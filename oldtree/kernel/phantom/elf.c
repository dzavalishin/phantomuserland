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
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>

#include <x86/phantom_page.h>
#include <i386/proc_reg.h>
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
//#include <unix/uufile.h>
#include <sys/fcntl.h>

// This structure is pointed by thread that is running it
// and describes loadable execution unit
struct exe_module
{
    const char * 	name; // debug only
    u_int32_t           start;
    u_int32_t           esp;

    // possibly here we have to have syscall service pointer
    //physaddr_t  	phys_cs;
    u_int16_t           cs_seg;
    linaddr_t           cs_linear;
    size_t  		cs_pages;

    //physaddr_t  	phys_ds;
    u_int16_t           ds_seg;
    linaddr_t           ds_linear;
    size_t  		ds_pages;

    void *              mem_start;
    void *              mem_end; // above last addr
};


static errno_t elf_check(struct Elf32_Ehdr *elf_header);
static errno_t elf_load_seg(Elf32_Phdr *ph, void *src, void *dst);

static void 	kernel_protected_module_starter( void * _em );





// Loads final relocated executable

errno_t load_elf( void *_elf, size_t elf_size, const char *name )
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

        //elf_load_segment(src, &program_header[i]);
    }

    //assert( minaddr >= 0 );
    if( minaddr != 0 )
        SHOW_ERROR( 0, "min addr not zero: 0x%X", minaddr );


    //int minpage = minaddr / PAGE_SIZE;
    //int maxpage = PAGE_ALIGN(maxaddr) / PAGE_SIZE;
    int maxpage = ((maxaddr-1) / PAGE_SIZE) + 1;

    // TODO implement stack somehow better with SS growing down
    int stack_pages = 64*1024/PAGE_SIZE;

    maxpage += stack_pages;

    int memsize = maxpage*PAGE_SIZE;

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
            // TODO free mem?
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

    // TODO Need some table of running modules?

    em->mem_start = va;
    em->mem_end = va+memsize;

    em->name = name;
    em->start = elf_header->e_entry;
    em->esp = memsize - sizeof(int); // Why -sizeof(int) ?

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

    SHOW_FLOW( 1, "cs 0x%X, ds 0x%X, entry 0x%x", em->cs_seg, em->ds_seg, em->start );

    hal_start_thread( kernel_protected_module_starter, em, 0 );

    return 0;
}



static void switch_to_user_mode_cs_ds(u_int32_t cs, u_int32_t ds, u_int32_t start, u_int32_t esp)
{

    /*

    Push:

    SS
    ESP
    EFLAGS
    CS
    EIP
    */



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



static void kernel_protected_module_starter( void * _em )
{
    struct exe_module *em = _em;

    SHOW_FLOW( 3, "Module %s thread started", em->name );

    hal_set_thread_name(em->name);

    uuprocess_t *u = uu_create_process(-1); // no parent PID?

    u->mem_start = em->mem_start;
    u->mem_end = em->mem_end;

    u->umask = 022;

    strncpy( u->cmd, em->name, MAX_UU_CMD );

#if 0
    u->tids[0] = GET_CURRENT_THREAD()->tid;
    GET_CURRENT_THREAD()->u = u;
#else
    uu_proc_add_thread( u, GET_CURRENT_THREAD()->tid );
#endif

    int err;
    usys_close( &err, u, 0 );
    usys_close( &err, u, 1 );
    usys_close( &err, u, 2 );

    if( usys_open( &err, u, "/dev/tty", O_RDONLY, 0 ) != 0 )
        SHOW_ERROR( 0, "can't open stdin, %d", err );

    if( usys_open( &err, u, "/dev/tty", O_WRONLY, 0 ) != 1 )
        SHOW_ERROR( 0, "can't open stdout, %d", err );

    if( usys_open( &err, u, "/dev/tty", O_WRONLY, 0 ) != 2 )
        SHOW_ERROR( 0, "can't open stderr, %d", err );



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

