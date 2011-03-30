/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Multiboot support.
 *
**/

#define DEBUG_MSG_PREFIX "boot"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_types.h>
#include <phantom_libc.h>
#include <string.h>
#include <kernel/vm.h>
#include <kernel/init.h>
#include <kernel/boot.h>
#include <kernel/page.h>

#include <x86/phantom_pmap.h>
#include <hal.h>
#include <multiboot.h>
#include <elf.h>
#include <unix/uuprocess.h>



#include "misc.h"
#include <kernel/config.h>
#include <kernel/board.h>


static int file_inited = 0;

static void __file_init_func(void) __attribute__ ((constructor));

static void __file_init_func(void)
{
	file_inited = 1;
}


#include <kernel/amap.h>

static amap_t ram_map;

//#define MEM_SIZE (((long)~(int)0)+1)
#define MEM_SIZE 0x100000000LL
#define SET_MEM(st,len,type) e(amap_modify( &ram_map, st, len, type))

static void e( errno_t err )
{
    if(err) panic("mem map init err = %d", err );
}



#ifdef ARCH_ia32
struct multiboot_info bootParameters;
#endif
static void make_mem_map(void);
static void process_mem_region( amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags, void *arg );

void
phantom_multiboot_main(physaddr_t multibootboot_info_pa, int cookie)
{
#ifdef ARCH_ia32
    if( cookie == 0x36d76289 )
    {
        printf("Not multiboot2 ready!");
        while(1)
            ;
    }

    bootParameters = *(struct multiboot_info*)phystokv(multibootboot_info_pa);
#endif
    board_init_early();

#if 0
    // TODO Enable superpage support if we have it.
    if (cpu.feature_flags & CPUF_4MB_PAGES)
    {
        set_cr4(get_cr4() | CR4_PSE);
    }
#endif

    board_init_cpu_management(); // idt/gdt/ldt

    board_init_interrupts();

    //phantom_init_descriptors();
    //phantom_fill_idt();
    //phantom_load_idt();

    arch_debug_console_init();

    // setup the floating point unit
    arch_float_init();

    // malloc will start allocating from fixed pool.
    phantom_heap_init();

    // Initialize the memory allocator and find all available memory.
    amap_init(&ram_map, 0, MEM_SIZE, MEM_MAP_UNKNOWN );


#ifdef ARCH_ia32
    make_mem_map();
#else
    board_fill_memory_map(&ram_map);
#endif

    // Suppose it to be machindep. Override if board didn't mention the kernel :)
    extern char _start_of_kernel[], end[];
    SET_MEM(kvtophys(_start_of_kernel), end-_start_of_kernel, MEM_MAP_KERNEL);

    hal_init_physmem_alloc();
    amap_iterate_all( &ram_map, process_mem_region, 0 );

    // Time to enable interrupts
    hal_sti();


    // Call constructors.
    __phantom_run_constructors();

    if(!file_inited)
    	SHOW_ERROR0(0, "FAIL: Constructors failed!");
    else
    	SHOW_FLOW0( 7, "Constructors OK!");


#ifdef ARCH_ia32
    printf("mb1 %s video: mode %d\n",
           ((bootParameters.flags & MULTIBOOT_VIDEO_INFO) ? "have" : "no"),
           bootParameters.vbe_mode
          );
#endif    

    phantom_parse_cmd_line();

    // Now time for kernel main

    main(main_argc, main_argv, main_env);
    panic("returned from main");
}





static char *nameTab[] =
{
    "Nothing?",
    "unknown",
    "low RAM",
    "hi RAM",
    "dev mem",
    "BIOS DA",
    "kernel",
    "module",
    "mod name",
    "mod tab",
    "args",
    "ELF header",
    "ELF section",
};


#define START_UPPER 0x100000




static void process_mem_region( amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags, void *arg )
{
    (void) arg;

    if(flags == MEM_MAP_HI_RAM && (n_elem >= PAGE_SIZE*10) )
        hal_physmem_add((from + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1),
                ((n_elem - (from & (PAGE_SIZE - 1))) & ~(PAGE_SIZE - 1)) / PAGE_SIZE );

    if(flags == MEM_MAP_LOW_RAM && (n_elem >= PAGE_SIZE))
        hal_physmem_add_low((from + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1),
                ((n_elem - (from & (PAGE_SIZE - 1))) & ~(PAGE_SIZE - 1)) / PAGE_SIZE );


    int kmb = (int)(n_elem/1024);
    char *kms = "K";

    if( n_elem < 4096 )
    {
        kmb = (int)n_elem;
        kms = "b";
    }
    else if( kmb > 4096 )
    {
        kmb /= 1024;
        kms = "M";
    }

    const char *name = nameTab[flags];

    SHOW_FLOW( 11,
           "[0x%09qX - 0x%09qX[ (%4d %s) - %s",
           from, from+n_elem,
           kmb, kms, name
          );

}


#ifdef ARCH_ia32

static void make_mem_map(void)
{

    if (bootParameters.flags & MULTIBOOT_MEMORY)
    {
        SET_MEM(0, bootParameters.mem_lower * 1024, MEM_MAP_LOW_RAM );
        SET_MEM(START_UPPER, bootParameters.mem_upper * 1024, MEM_MAP_HI_RAM );
    }

    //amap_dump( &ram_map );
    //printf("------------\n");

    SET_MEM(0, 0x500, MEM_MAP_BIOS_DA );

    // TODO add VGA/MDA videomem?
    // TODO somehow exclude VESA videomem?
    // TODO skip known CPU memmapped devs? APIC?

    //extern char _start_of_kernel[], end[];
    //SET_MEM(kvtophys(_start_of_kernel), end-_start_of_kernel, MEM_MAP_KERNEL);


    if(bootParameters.flags & MULTIBOOT_CMDLINE)
    {
        SET_MEM(bootParameters.cmdline,
                     strlen((char*)phystokv(bootParameters.cmdline)) + 1,
                     MEM_MAP_ARGS);
    }

    if ((bootParameters.flags & MULTIBOOT_MODS)
        && (bootParameters.mods_count > 0))
    {
        struct multiboot_module *m = (struct multiboot_module*)
            phystokv(bootParameters.mods_addr);


        SET_MEM(bootParameters.mods_addr,
                     bootParameters.mods_count * sizeof(*m),
                     MEM_MAP_MODTAB);

        unsigned int i;
        for (i = 0; i < bootParameters.mods_count; i++)
        {
            if (m[i].string != 0)
            {
                char *s = (char*)phystokv(m[i].string);
                unsigned len = strlen(s);

                SET_MEM(m[i].string, len, MEM_MAP_MOD_NAME);
            }

            SET_MEM(m[i].mod_start, m[i].mod_end-m[i].mod_start, MEM_MAP_MODULE );
        }
    }

    if(bootParameters.flags & MULTIBOOT_ELF_SHDR)
    {
        SET_MEM(
                bootParameters.syms.e.addr,
                bootParameters.syms.e.size * bootParameters.syms.e.num,
                MEM_MAP_ELF_HDR
               );

        Elf32_Shdr *header = (void *)bootParameters.syms.e.addr;
        int num = bootParameters.syms.e.num;

        int i;
        // 0th element is zero filled
        for( i = 1; i < num; i++ )
        {
            SET_MEM(
                    header[i].sh_addr,
                    header[i].sh_size,
                    MEM_MAP_ELF_SEC
               );

        }

    }
    //amap_dump( &ram_map );



    //asm volatile ("hlt");
}

#endif


static void map_eq( amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags, void *arg )
{
    (void) arg;

    int off = from & INTEL_OFFMASK;
    unsigned pageaddr = (unsigned)(from & ~INTEL_OFFMASK); // Just page start addr
    n_elem += off; // We moved start addr back, now fix len

    int n_pages = 1+((n_elem-1)/PAGE_SIZE);

    switch(flags)
    {
    case  MEM_MAP_KERNEL:
    case  MEM_MAP_MODULE:
    case  MEM_MAP_MOD_NAME:
    case  MEM_MAP_MODTAB:
    case  MEM_MAP_ARGS:
    case  MEM_MAP_ELF_HDR:
    case  MEM_MAP_ELF_SEC:
        //hal_physmem_add( from, n_elem/PAGE_SIZE ); // WHYY!!??
        hal_pages_control( pageaddr, (void *)pageaddr, n_pages,
                           page_map, page_ro );
        break;

    case  MEM_MAP_DEV_MEM:
    case  MEM_MAP_BIOS_DA:
        hal_pages_control( pageaddr, (void *)pageaddr, n_pages,
                           page_map, page_rw );
        break;

    default:
        break;
    }
    //if(flags == MEM_MAP_LOW_RAM)
}


void phantom_map_mem_equally()
{
    // Low RAM
    hal_pages_control( 0, 0, 0x100000/PAGE_SIZE, page_map, page_rw );
    amap_iterate_all( &ram_map, map_eq, 0 );

    // kernel was mapped RO. Now remap data seg r/w

    //extern char start_of_data[];
    extern char _data_start__[];
    extern char _bss_end__[];
    extern char _end__[];

    int ds_start = (int) &_data_start__;

    if( !PAGE_ALIGNED(ds_start) )
        SHOW_ERROR0( 0, "WARNING: Data seg is not on page boundary!");

    ds_start &= ~(PAGE_SIZE-1); // Bump down to page boundary.

    //int ds_bytes = ((int)&_bss_end__) - ds_start;
    int ds_bytes = ((int)&_end__) - ds_start;

    int ds_pages = PAGE_ALIGN(ds_bytes) / PAGE_SIZE;

    hal_pages_control( ds_start, (void *)ds_start, ds_pages, page_map, page_rw );

    SHOW_FLOW( 7, "Data seg 0x%X-%X, %d bytes, %d pages", ds_start, ((int)&_bss_end__), ds_bytes, ds_pages );
}


#ifdef ARCH_ia32

static const char *skip_path(const char *mn)
{
    char *p;

    p = rindex(mn, '/');
    if(p) mn = p+1;

    p = rindex(mn, ':');
    if(p) mn = p+1;

    p = rindex(mn, ')');
    if(p) mn = p+1;

    return mn;
}

#endif

/**
 *
 * Find multiboot module with specific string in name.
 *
 **/

struct multiboot_module *phantom_multiboot_find(const char *string)
{
#if defined(ARCH_ia32)
    struct multiboot_module *m = (struct multiboot_module*)
        phystokv(bootParameters.mods_addr);
    unsigned i;

    if (!(bootParameters.flags & MULTIBOOT_MODS))
        return 0;

    for (i = 0; i < bootParameters.mods_count; i++)
    {
        if (!(m[i].string) )
            continue;
        const char *mn = (char*)phystokv(m[i].string);

        mn = skip_path(mn);

        if( strcmp(mn, (char *)string) == 0)
            return &m[i];
    }
#else
    (void) string;
#endif
    return 0;
}



// -----------------------------------------------------------------------
// Start modules bootloader brought us
// -----------------------------------------------------------------------

#define PMOD_PREFIX "pmod_"

void phantom_start_boot_modules()
{
#if HAVE_UNIX && defined(ARCH_ia32)
    struct multiboot_module *m = (struct multiboot_module*)
        phystokv(bootParameters.mods_addr);
    unsigned i;

    if (!(bootParameters.flags & MULTIBOOT_MODS))
        return;

    for (i = 0; i < bootParameters.mods_count; i++)
    {
        if( !(m[i].string) )  continue;
        const char *mn = skip_path((char*)phystokv(m[i].string));

        if( strncmp(mn, PMOD_PREFIX, sizeof(PMOD_PREFIX)-1 ) != 0)
            continue;

        struct multiboot_module *module = m+i;

        void *code = (void *)phystokv(module->mod_start);
        size_t size = module->mod_end - module->mod_start;

        int pid = uu_create_process(-1);
        const char* av[] = { mn, 0 };
        uu_proc_setargs( pid, av, 0 );

        errno_t err = uu_run_binary( pid, code, size );
        if(err)
        {
            char errbuf[1024];
            errbuf[0] = 0;
            //strerror_r( err, errbuf, 1024);

            SHOW_ERROR( 0, "module start error %d (%s)", err, errbuf );
        }
    }

#endif // HAVE_UNIX
}

