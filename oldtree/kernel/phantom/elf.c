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

//#include <unix/uufile.h>
//#include <sys/fcntl.h>



static errno_t 	elf_check(struct Elf32_Ehdr *elf_header);
static errno_t 	elf_load_seg(Elf32_Phdr *ph, void *src, void *dst);

static void 	kernel_protected_module_starter( void * _u );

static errno_t 	load_elf( struct exe_module **emo, void *_elf, size_t elf_size );




errno_t uu_run_binary( int pid, void *_elf, size_t elf_size )
{
//return EIO;
    struct exe_module *em;
    errno_t e = load_elf( &em, _elf, elf_size );
    if( e ) return e;

    if( (e = uu_proc_set_exec( pid, em ) ) )
    {
        SHOW_ERROR( 0, "Can't set em for pid %d", pid );
        return e;
    }

    hal_start_thread( kernel_protected_module_starter, (void *)pid, 0 );

    return 0;
}


errno_t uu_run_file( int pid, const char *fname )
{
    void *odata;
    int osize;
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


#if defined(ARCH_ia32)

static void switch_to_user_mode_cs_ds(u_int32_t cs, u_int32_t ds, u_int32_t start, u_int32_t esp)
{
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

    // todo align?
    esp -= size;
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

    uu_proc_add_thread( pid, GET_CURRENT_THREAD()->tid );

    int prev_esp = em->esp;

    int ava, enva, tmp;

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
    // Check if we need to load this segment
    if(seg->p_type != PT_LOAD)
        return 0; // Not loadable

    // Get pointer to source
    void *src_base = src + seg->p_offset;

    // Get pointer to destination
    void *dest_base = dst + seg->p_vaddr;
    //size_t size = ((u32int) dest_base + seg->p_memsz + 0x1000) & ~0xFFF;

    // Load!
    memcpy(dest_base, src_base, seg->p_memsz);

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




#endif // HAVE_UNIX

