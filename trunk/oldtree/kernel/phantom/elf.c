/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * ELF binary code loader. Usermode process starter - temp, rewrite, move out.
 *
**/

#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "elf"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>
#include <kernel/page.h>

#include <phantom_libc.h>
#include <phantom_types.h>
#include <string.h>
#include <threads.h>
#include <thread_private.h>
#include <errno.h>

#include "misc.h"
#include <threads.h>


#include <elf.h>
#include <kernel/unix.h>
#include <unix/uuprocess.h>
#include <kunix.h>

#ifdef ARCH_ia32
#include <ia32/private.h>
#include <compat/kolibri.h>
#include <lzma.h>
#endif

//#include <unix/uufile.h>
//#include <sys/fcntl.h>



static errno_t 	elf_check(struct Elf32_Ehdr *elf_header);
static errno_t 	elf_load_seg(Elf32_Phdr *ph, void *src, void *dst);

static void 	kernel_protected_module_starter( void * _u );

static errno_t 	load_elf( struct exe_module **emo, void *_elf, size_t elf_size );

#if HAVE_KOLIBRI
static errno_t 	load_kolibri( struct exe_module **emo, void *_elf, size_t elf_size );
#endif



errno_t uu_run_binary( int pid, void *_elf, size_t elf_size )
{
    struct exe_module *em;
    errno_t e;

#if HAVE_KOLIBRI
    if( !is_not_kolibri_exe( _elf ) )
        e = load_kolibri( &em, _elf, elf_size );
    else
#endif
        e = load_elf( &em, _elf, elf_size );

    if( e ) return e;

    if( (e = uu_proc_set_exec( pid, em ) ) )
    {
        SHOW_ERROR( 0, "Can't set em for pid %d", pid );
        return e;
    }

    hal_start_thread( kernel_protected_module_starter, (void *)pid, THREAD_FLAG_USER );

    return 0;
}


errno_t uu_run_file( int pid, const char *fname )
{
    void *odata;
    size_t osize;
    SHOW_FLOW( 3, "loading %s", fname );

    errno_t ke = k_load_file( &odata, &osize, fname );

    if( !ke )
    {
        SHOW_FLOW( 2, "running %s", fname );
        ke = uu_run_binary( pid, odata, osize );
        free( odata );
        return ke;
    }

    SHOW_ERROR( 0, "%s read error %d", fname, ke );
    return ke;
}

#ifdef ARCH_ia32

#if HAVE_KOLIBRI

errno_t load_kolibri( struct exe_module **emo, void *_exe, size_t exe_size )
{
    errno_t rc;

    SHOW_FLOW( 7, "Attempt to run Kolibri exe, size %d", exe_size );

    if( exe_size < sizeof(kolibri_exe_hdr_t) )
        return ENOEXEC;

    kolibri_exe_hdr_t *hdr = _exe;

    rc = is_not_kolibri_exe( hdr );
    if( rc ) return rc;


    struct kolibri_pkck_hdr *kpck = _exe;
    int kpacked = ( 0 == strncmp( "KPCK", kpck->ident, 4 ) );

    if(kpacked)
    {
        SHOW_FLOW( 6, "Packed (PKCK) Kolibri exe, uncomp size %d flags 0x%x", kpck->unpacked_size, kpck->flags );
        if( kpck->unpacked_size > MAX_COLIBRI_EXE_SIZE )
            return ENOEXEC; // too big?

        void *unp = calloc( 1, kpck->unpacked_size );
        if( unp == 0 )
            return ENOMEM;

        size_t dest_len = kpck->unpacked_size;
        size_t src_len = exe_size - sizeof(struct kolibri_pkck_hdr);
        void *src = _exe + sizeof(struct kolibri_pkck_hdr);

        rc = plain_lzma_decode( unp, &dest_len, src, &src_len, 0 );
        if( rc )
        {
            SHOW_ERROR( 0, "KPCK unpack err %d", rc );
            free(unp);
            return ENOEXEC; 
        }

        rc = is_not_kolibri_exe( unp );
        if( rc )
        {
            SHOW_ERROR( 0, "KPCK unpack - result is not Kolibri exe, err %d", rc );
            free(unp);
            return rc;
        }

        _exe = unp;
        hdr = unp;
        exe_size = dest_len;

        //SHOW_ERROR0( 0, "Packed (PKCK) Kolibri exe not impl" );
        //return ENXIO;
    }


    int maxpage = BYTES_TO_PAGES(hdr->stack_end);

    {
        int dpages = BYTES_TO_PAGES(hdr->data_end);
        if( dpages > maxpage )
            maxpage = dpages;
    }

    // TODO stack guard page
    maxpage += 4; // Phantom puts stack at top of memory in any case, give stack some place to live

    size_t memsize = maxpage*PAGE_SIZE;

    size_t data_end = hdr->data_end;

    /*
    if( data_end > exe_size )
    {
        SHOW_ERROR( 0, "Kolibri EXE data_end %d > exe_size %d", data_end, exe_size );
        return ENOEXEC;
    }
    */

    if( hdr->start >= hdr->code_end )
    {
        SHOW_ERROR( 0, "Kolibri EXE hdr->start %d >= hdr->code_end %d", hdr->start, hdr->code_end );
        if(kpacked) free(_exe);
        return ENOEXEC;
    }

    void *va;
    physaddr_t pa;
    hal_pv_alloc( &pa, &va, memsize );

    memset( va, 0, memsize ); // Clear BSS! :)
    memcpy( va, _exe, data_end );

    // For kernel protected modlue allocata LDT, set segments, set LDT for thread,
    // start module in LDT.
    struct exe_module *em = calloc( sizeof(struct exe_module), 1 );
    em->refcount++;

    em->flags = EM_FLAG_KOLIBRI;
    em->kolibri_cmdline_addr = hdr->params;
    em->kolibri_cmdline_size = KOLIBRI_CMD_LINE_MAX;
    em->kolibri_exename_addr = hdr->exe_name;
    em->kolibri_exename_size = KOLIBRI_CMD_PATH_MAX;

    if( em->kolibri_cmdline_addr > memsize ) // Insane?
        em->kolibri_cmdline_size = 0;

    if( em->kolibri_exename_addr > memsize ) // Insane?
        em->kolibri_exename_size = 0;

    // TODO Need some table of running modules?

    em->mem_start = va;
    em->mem_end = va+memsize;

    em->mem_size = memsize;
    em->pa = pa;

    em->start = hdr->start;
    em->esp = memsize - sizeof(int); // Why -sizeof(int) ?

    em->stack_bottom = hdr->data_end; // TODO sanity check
    // TODO DS must not intersect with CS
    // TODO CS size is wrong (includes DS)

    em->cs_pages = maxpage;
    em->ds_pages = maxpage;

    if(kpacked) free(_exe);

    if( get_uldt_cs_ds(
                       (int)va, &(em->cs_seg), memsize,
                       (int)va, &(em->ds_seg), memsize
                      ) )
    {
        SHOW_ERROR0( 0, "Can't allocate CS/DS in LDT" );
        // TODO free mem?
        return ENOMEM;
    }

    SHOW_FLOW( 7, "Kolibri exe cs 0x%X, ds 0x%X, entry 0x%x", em->cs_seg, em->ds_seg, em->start );

    *emo = em;

    return 0;

}

#endif // HAVE_KOLIBRI


// Loads final relocated executable
errno_t load_elf( struct exe_module **emo, void *_elf, size_t elf_size )
{

    // TODO check that we do not access data out of elf image
    (void)elf_size;

    struct Elf32_Ehdr *elf_header = (struct Elf32_Ehdr *)_elf;

    if( elf_check(elf_header) )
        return ENOEXEC;

    Elf32_Phdr *program_header = (Elf32_Phdr*) (_elf + elf_header->e_phoff);

    int nseg = elf_header->e_phnum; // Number of segments;

    if( nseg == 0 )
        return ENOEXEC;

    // TODO determine code seg size and store it to limit
    // sbrk from below

    unsigned minaddr = ~0;
    unsigned maxaddr = 0;
    int i;
    for (i = 0; i < nseg; i++)
    {
        Elf32_Phdr *ph = &program_header[i];

        unsigned int bot = ph->p_vaddr;
        unsigned int top = ph->p_vaddr + ph->p_memsz;

        if( bot < minaddr ) minaddr = bot;
        if( top > maxaddr ) maxaddr = top;
    }

    if( minaddr != 0 )
        SHOW_ERROR( 0, "min addr not zero: 0x%X", minaddr );


    //int minpage = minaddr / PAGE_SIZE;
    //int maxpage = PAGE_ALIGN(maxaddr) / PAGE_SIZE;
    int maxpage = ((maxaddr-1) / PAGE_SIZE) + 1;

    // TODO implement stack somehow better with SS growing down
    int stack_size = 64*1024;
    int stack_pages = stack_size/PAGE_SIZE;

    maxpage += stack_pages;

    int memsize = maxpage*PAGE_SIZE;

    // TODO alloc virt mem in persistent space!
    // * Allocate physical RAM
    // * Map it temporarily
    void *va;
    physaddr_t pa;
    hal_pv_alloc( &pa, &va, memsize );

    memset( va, 0, memsize ); // Clear BSS! :)

    // * Copy required sections
    for(i = 0; i < nseg; i++)
    {
        Elf32_Phdr *ph = program_header+i;
        if( elf_load_seg(ph, _elf, va) )
        {
            hal_pv_free( pa, va, memsize );
            return ENXIO;
        }

        // * set actual pages permission
        //hal_set_mapping_mode(); // RW, RO/EXEC
    }
    // * TODO link to kernel funcs

    // For Unix object here we need to allocate page-aligned phantom objects
    // for CS/DS, copy CS/DS there, map 'em and setup/set segments


#if 0
    // For kernel module just start it now
    void (*entry)(char **av, int ac, char **env) = (void *)va + elf_header->e_entry;

    start_kernel_thread( kernel_module_starter, entry );
#endif

    // For kernel protected modlue allocata LDT, set segments, set LDT for thread,
    // start module in LDT.
    struct exe_module *em = calloc( sizeof(struct exe_module), 1 );
    em->refcount++;

    // TODO Need some table of running modules?

    em->mem_start = va;
    em->mem_end = va+memsize;


    em->mem_size = memsize;
    em->pa = pa;

    em->start = elf_header->e_entry;
    em->esp = memsize - sizeof(int); // Why -sizeof(int) ?

    em->stack_bottom = memsize-stack_size;
    // TODO DS must not intersect with CS
    // TODO CS size is wrong (includes DS)

    em->cs_pages = maxpage;
    em->ds_pages = maxpage;

    if( get_uldt_cs_ds(
                       (int)va, &(em->cs_seg), memsize,
                       (int)va, &(em->ds_seg), memsize
                      ) )
    {
        SHOW_ERROR0( 0, "Can't allocate CS/DS in LDT" );
        // TODO free mem?
        return ENOMEM;
    }

    SHOW_FLOW( 7, "cs 0x%X, ds 0x%X, entry 0x%x", em->cs_seg, em->ds_seg, em->start );

    *emo = em;

    return 0;
}

#else // ARCH_ia32
// Loads final relocated executable

errno_t load_elf( struct exe_module **emo, void *_elf, size_t elf_size )
{
    return ENOSYS;
}

errno_t load_kolibri( struct exe_module **emo, void *_exe, size_t exe_size )
{
    return ENOSYS;
}

#endif // ARCH_ia32




#if defined(ARCH_ia32)

static void switch_to_user_mode_cs_ds(u_int32_t cs, u_int32_t ds, u_int32_t start, u_int32_t esp)
{
    //cs |= 7; // cpl = low 2 bits = 3, bit 2 = LDT = 1
    //ds |= 7;
    // Push sequence: SS ESP EFLAGS CS EIP
    // Set up a stack structure for switching to user mode.
    asm volatile("  \
                 sti; \
                 movl %3, %%edi; \
                 movl %2, %%esi; \
                 movl %1, %%eax; \
                 movl %0, %%ebx; \
                 mov %%ax, %%ds; \
                 mov %%ax, %%es; \
                 mov %%ax, %%fs; \
                 mov %%ax, %%gs; \
                 \
                 pushl %%eax; \
                 pushl %%edi; \
                 pushf; \
                 pushl %%ebx; \
                 push %%esi; \
                 xor %%ebp, %%ebp; \
                 iret; \
                 "
                 : : "m" (cs), "m" (ds), "m" (start), "m" (esp)
                );
}

#else
static void switch_to_user_mode_cs_ds(u_int32_t cs, u_int32_t ds, u_int32_t start, u_int32_t esp)
{
    (void) cs;
    (void) ds;
    (void) start;
    (void) esp;
    panic("no user mode yet on ARM");
}
#endif

// _esp - user esp value (in user addr space)
// ouseraddr - resulting user address of pushed object

static errno_t user_push( struct exe_module *em, int *ouseraddr, const void *data, int size )
{
    addr_t min_esp = em->stack_bottom+(32*1024); // Leave him at least 32K?
    addr_t esp = em->esp;

    esp -= size;
    esp &= ~3; // align downwards to 4 bytes boundary

    if( esp < min_esp )
        return ENOMEM;

    void *dest = (void *) (((addr_t)em->mem_start) + esp);
    memcpy( dest, data, size );

    if(ouseraddr) *ouseraddr = esp;
    em->esp = esp;
    return 0;
}

static errno_t user_push_args( struct exe_module *em, const char **av, int *ouseraddr )
{
    const char **avp;
    int ac = 0;

    if( av != 0 )
    {
        for( avp = av; *avp; avp++ )
            ac++;
    }

    const char *uav[ac+1];

    int i;
    for( i = 0; i < ac; i++ )
    {
        int oaddr;

        if( user_push( em, &oaddr, av[i], strlen(av[i])+1 ) )
            return ENOMEM;

        uav[i] = (void *)oaddr;
    }

    uav[ac] = 0;

    //hexdump( uav, sizeof(const char *) * (ac+1), 0, 0 );

    return user_push( em, ouseraddr, uav, sizeof(const char *) * (ac+1) );
}

static void kernel_protected_module_starter( void * _pid )
{
    int pid = (int) _pid;
    uuprocess_t *u = proc_by_pid(pid);
    assert(u);
    struct exe_module *em = u->em;
    assert(em);

    const char *name = u->cmd;

    SHOW_FLOW( 3, "Module %s thread started", name );

    hal_set_thread_name(name);

    uu_proc_add_thread( pid, get_current_tid() );

    int prev_esp = em->esp;

    int ava, enva, tmp;

#if HAVE_KOLIBRI
    if( em->flags & EM_FLAG_KOLIBRI )
    {
        if(em->kolibri_cmdline_size)
            strlcpy( em->mem_start+em->kolibri_cmdline_addr, name, em->kolibri_cmdline_size );

        if(em->kolibri_cmdline_size)
        {
            char *dest = em->mem_start+em->kolibri_cmdline_addr;
            *dest = 0;

            const char **avp = u->argv;
            while(*avp++)
                strlcat( dest, *avp, em->kolibri_cmdline_size );
        }
    }
#endif // HAVE_KOLIBRI


#if 1
    // todo check success
    user_push_args( em, u->envp, &enva );
    user_push_args( em, u->argv, &ava );

    tmp = 0; // put zero arg after usual ac/av/envp
    user_push( em, 0, &tmp, sizeof(tmp) );

    user_push( em, 0, &(enva), sizeof(enva) );
    user_push( em, 0, &(ava), sizeof(ava) );
    user_push( em, 0, &(u->argc), sizeof(u->argc) );
#endif
    tmp = ~0; // __start retaddr - make it invalid
    user_push( em, 0, &tmp, sizeof(tmp) );

    SHOW_FLOW( 4, "pushed %d bytes", prev_esp - em->esp );

    // This sets
    //arch_adjust_after_thread_switch(GET_CURRENT_THREAD());

    // Dies on stack access fault on real hw!
//return;

#ifdef ARCH_ia32
    SHOW_FLOW( 4, "go user, start 0x%x sp 0x%x, memsize 0x%x, ds limit 0x%x", em->start, em->esp, em->mem_size, ia32_lsl(em->ds_seg) );
#else
    SHOW_FLOW( 4, "go user, start 0x%x sp 0x%x, memsize 0x%x", em->start, em->esp, em->mem_size );
#endif
    switch_to_user_mode_cs_ds( em->cs_seg, em->ds_seg, em->start, em->esp );
}




static int elf_check(struct Elf32_Ehdr *elf_header)
{
    if( elf_header->e_ident[0] != 0x7F)		return ENOEXEC;
    if( elf_header->e_ident[1] != 'E') 		return ENOEXEC;
    if( elf_header->e_ident[2] != 'L') 		return ENOEXEC;
    if( elf_header->e_ident[3] != 'F') 		return ENOEXEC;

    if( elf_header->e_type     != ET_EXEC) 	return ENOEXEC;
    if( elf_header->e_machine  != EM_386) 	return ENOEXEC;

    if( elf_header->e_version  == 0) 		return ENOEXEC;

    return 0;
}



static errno_t elf_load_seg(Elf32_Phdr *seg, void *src, void *dst)
{
    SHOW_INFO( 5, "Elf32_Phdr type %d offset %d vaddr %d filesz %d memsz %d flags %x",
               seg->p_type,
               seg->p_offset, seg->p_vaddr,
               seg->p_filesz, seg->p_memsz,
               seg->p_flags
             );

    // Check if we need to load this segment
    if(seg->p_type != PT_LOAD)
        return 0; // Not loadable

    // Get pointer to source
    void *src_base = src + seg->p_offset;

    // Get pointer to destination
    void *dest_base = dst + seg->p_vaddr;
    //size_t size = ((u32int) dest_base + seg->p_memsz + 0x1000) & ~0xFFF;

    // Loadable image less than mem - clear
    if(seg->p_filesz < seg->p_memsz)
        bzero( dest_base, seg->p_memsz );

    if(seg->p_filesz > seg->p_memsz)
    SHOW_ERROR( 1, "Elf32_Phdr filesz %d > memsz %d", seg->p_filesz, seg->p_memsz );

    // Load!
    //memcpy(dest_base, src_base, seg->p_memsz);
    memcpy(dest_base, src_base, seg->p_filesz);

#if 0
    // Set proper flags (i.e. remove write flag if needed)
    if (seg->p_flags & PF_W)
    {
        i = ((u32int) dest_base) & ~0xFFF;
        for (; i < dest_limit; i+= 0x1000)
            page_set(&t->map, i, page_fmt(page_get(&t->map, i), PF_USER | PF_PRES));
    }
#endif

    return 0;
}

#if HAVE_KOLIBRI
// Untested
void kolibri_thread_starter( void * arg )
{
    struct kolibri_thread_start_parm *sp = arg;
    assert(arg);

    addr_t eip = sp->eip;
    addr_t esp = sp->esp;

    free(sp);

    tid_t tid = get_current_tid();
    pid_t pid;
    assert( !t_get_pid( tid, &pid ));

    uuprocess_t *u = proc_by_pid(pid);
    assert(u);

    struct exe_module *em = u->em;
    assert(em);

    const char *name = u->cmd;

    SHOW_FLOW( 3, "Kolibri %s thread started", name );

    hal_set_thread_name(name);

    uu_proc_add_thread( pid, tid );

    switch_to_user_mode_cs_ds( em->cs_seg, em->ds_seg, eip, esp );

}
#endif // HAVE_KOLIBRI



#endif // HAVE_UNIX

