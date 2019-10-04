/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel main
 *
 *
**/

#define DEBUG_MSG_PREFIX "boot"
#include <debug_ext.h>
#define debug_level_flow 4
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>
#include <kernel/board.h>
#include <kernel/snap_sync.h>
#include <kernel/acpi.h>

#include "svn_version.h"

#include <phantom_libc.h>
#include <phantom_time.h>

#include <kernel/boot.h>
#include <kernel/init.h>
#include <kernel/debug.h>
#include <kernel/trap.h>


#include <newos/port.h>


#include <hal.h>
#include "paging_device.h"
#include "vm_map.h"

#include <vm/root.h>
//#include "video.h"
#include "misc.h"
#include <kernel/net.h>

#include <kernel/timedcall.h>

#include <sys/utsname.h>
#include <sys/syslog.h>
#include <stdlib.h>

#include <threads.h>

// pvm_memcheck
#include <vm/alloc.h>

// multiboot info
#include <multiboot.h>

// phys addr
#include <kernel/vm.h>




//#define N_OBJMEM_PAGES ((1024L*1024*128)/4096)
#define N_OBJMEM_PAGES ((1024L*1024*32)/4096)
#define CHECKPAGES 1000

















void pressEnter(char *text)
{
    printf("%s\n", text);

    if(debug_boot_pause)
    {
        printf("press Enter...\n...");
#if 0
        while( getchar() >= ' ' )
            ;
#else
        hal_sleep_msec(10000);
#endif
    }
}

//static void pause() { pressEnter("pause"); printf("\n"); }


#if !PAGING_PARTITION
static paging_device pdev;
#endif

void start_phantom()
{
    //pressEnter("will start Phantom");
    SHOW_FLOW0( 2, "Will init snap interlock... ");
    phantom_snap_threads_interlock_init();

    //pressEnter("will init paging dev");

    SHOW_FLOW0( 5, "Will init paging dev... ");
#if !PAGING_PARTITION
    // TODO size?
    init_paging_device( &pdev, "wd1", 1024*20); //4096 );

    //pressEnter("will init pager");
    pager_init(&pdev);
#else
    partition_pager_init(select_phantom_partition());
#endif

    SHOW_FLOW0( 5, "Will init vm map... ");
    //pressEnter("will init VM map");
    // TODO BUG this +1 is due to allocator error:
    // allocator insists on trying to access the byte
    // just after the memory arena. Fix that and remove this +1 after.
    vm_map_init( N_OBJMEM_PAGES+1 );
}






static void net_stack_init();













#if !DRIVE_SCHED_FROM_RTC
static timedcall_t sched_timer =
{
    (void *)phantom_scheduler_time_interrupt,
    0, 20,
    0, 0, { 0, 0 }, 0
};

void phantom_turn_off_pic_scheduler_timer(void)
{
    phantom_undo_timed_call( &sched_timer );
}
#endif

/**
 *
 * Phantom kernel main. Called with (what?) initialized.
 *
**/


int main(int argc, char **argv, char **envp)
{
    (void) envp;

    snprintf( phantom_uname.machine, sizeof(phantom_uname.machine), "%s/%s", arch_name, board_name );

    run_init_functions( INIT_LEVEL_PREPARE );

    init_irq_allocator();

    //hal_init(             (void *)PHANTOM_AMAP_START_VM_POOL,             N_OBJMEM_PAGES*4096L);

    // moved pre main()
#if 0
    // TODO we have 2 kinds of ia32 cpuid code
    detect_cpu(0);
    //identify_cpu();
    identify_hypervisor();
#endif

    phantom_paging_init();

#ifdef ARCH_arm
    //test_swi();
#endif

    board_init_kernel_timer();
    phantom_timed_call_init(); // Too late? Move up?

#if defined(ARCH_mips) && 0
    SHOW_FLOW0( 0, "test intr reg overflow" );
    mips_test_interrupts_integrity();
    SHOW_FLOW0( 0, "intr reg overflow test PASSED" );
#endif

    hal_init((void *)PHANTOM_AMAP_START_VM_POOL, N_OBJMEM_PAGES*4096L);

    // Threads startup
    {
    arch_threads_init();
    phantom_threads_init();
    heap_init_mutex(); // After threads
    pvm_alloc_threaded_init(); // After threads

#if !DRIVE_SCHED_FROM_RTC // run from int 8 - rtc timer
    phantom_request_timed_call( &sched_timer, TIMEDCALL_FLAG_PERIODIC );
#endif
    hal_set_softirq_handler( SOFT_IRQ_THREADS, (void *)phantom_scheduler_soft_interrupt, 0 );
    }

    //SHOW_FLOW0( 0, "Will sleep" );
    //hal_sleep_msec( 120000 );

    net_timer_init();


    phantom_init_part_pool();

    // Driver stage is:
    //   0 - very early in the boot - interrupts can be used only
    //   1 - boot, most of kernel infrastructure is there
    //   2 - disks which Phantom will live in must be found here
    //   3 - late and optional and slow junk

    phantom_find_drivers( 0 );

    board_start_smp();

    {
        extern const char* SVN_Version;
        extern struct utsname phantom_uname;
        strncpy( phantom_uname.release, SVN_Version, sizeof(phantom_uname.release) );
    }

    //pressEnter("will run DPC");
    dpc_init();



    printf("\n\x1b[33m\x1b[44mPhantom " PHANTOM_VERSION_STR " (SVN rev %s) @ %s starting\x1b[0m\n\n", svn_version(), phantom_uname.machine );
    phantom_process_boot_options();

#if defined(ARCH_arm) && 0
    SHOW_FLOW0( 0, "test intr reg overflow" );
    arm_test_interrupts_integrity();
    SHOW_FLOW0( 0, "intr reg overflow test PASSED" );
#endif


    dbg_init(); // Kernel command line debugger

    // Used to refill list used to allocate physmem in interrupts
    hal_init_physmem_alloc_thread();

    port_init();
    //pressEnter("will start net");
    net_stack_init();

    //pressEnter("will look for drv stage 1");
    phantom_find_drivers( 1 );

    // here it kills all by calling windowing funcs sometimes
    //init_main_event_q();
    run_init_functions( INIT_LEVEL_INIT );

    //vesa3_bootstrap();

#ifdef ARCH_ia32
    SHOW_FLOW( 5, "ACPI: %d" , InitializeFullAcpi() );
#endif

#ifdef ARCH_ia32
#if HAVE_VESA
    //pressEnter("will init vm86");
    phantom_init_vm86();
    //pressEnter("will init VESA");
    if(!bootflag_no_vesa) phantom_init_vesa();
#endif
#endif
    //pressEnter("will init graphics");
    phantom_start_video_driver();

    //SHOW_FLOW0( 0, "Will sleep" );
    //hal_sleep_msec( 120000 );

    // vm86 and VESA die without page 0 mapped
#if 1
	// unmap page 0, catch zero ptr access
	hal_page_control( 0, 0, page_unmap, page_noaccess );
#endif

    init_main_event_q();

    //pressEnter("will look for drv stage 2");
    phantom_find_drivers( 2 );


#if HAVE_UNIX
    // Before boot modules init Unix-like environment
    // as modules have access to it
    phantom_unix_fs_init();
    phantom_unix_proc_init();

    // Start modules bootloader brought us
    phantom_start_boot_modules();
#endif // HAVE_UNIX



#if HAVE_USB
    usb_setup();
#endif // HAVE_USB

    //pressEnter("will run phantom_timed_call_init2");
    //phantom_timed_call_init2();

    // -----------------------------------------------------------------------
    // If this is test run, switch to test code
    // -----------------------------------------------------------------------
#if !defined(ARCH_arm)
    if( argc >= 3 && (0 == strcmp( argv[1], "-test" )) )
    {
        SHOW_FLOW0( 0, "Sleep before tests to settle down boot activities" );
        hal_sleep_msec( 2000 );
        SHOW_FLOW( 0, "Will run '%s' test", argv[2] );
        run_test( argv[2], argv[3] );
	// CI: this message is being watched by CI scripts (ci-runtest.sh)
        SHOW_FLOW0( 0, "Test done, reboot");
        exit(0);
    }
#else
    {
        (void) argv;
        (void) argc;
        SHOW_FLOW0( 0, "Will run all tests" );
        run_test( "all", "" );
	// CI: this message is being watched by CI scripts (ci-runtest.sh)
        SHOW_FLOW0( 0, "Test done, reboot");
        exit(0);
    }
#endif

#ifdef ARCH_ia32
    connect_ide_io();
#endif

    // -----------------------------------------------------------------------
    // Now starting object world infrastructure
    // -----------------------------------------------------------------------


    //pressEnter("will start phantom");
    start_phantom();

#ifdef ARCH_ia32
    phantom_check_disk_check_virtmem( (void *)hal_object_space_address(), CHECKPAGES );
#endif

    SHOW_FLOW0( 2, "Will load classes module... ");
    load_classes_module();

    phantom_find_drivers( 3 );


#if HAVE_NET
    phantom_tcpip_active = 1; // Tell them we finished initing network
    syslog(LOG_DEBUG|LOG_KERN, "Phantom " PHANTOM_VERSION_STR " (SVN rev %s) @ %s starting", svn_version(), phantom_uname.machine );
#endif




    SHOW_FLOW0( 2, "Will init phantom root... ");
    // Start virtual machine in special startup (single thread) mode
    pvm_root_init();

    // just test
    //phantom_smp_send_broadcast_ici();

    //pressEnter("will run vm threads");
    SHOW_FLOW0( 2, "Will run phantom threads... ");
    // Virtual machine will be run now in normal mode
    activate_all_threads();

    vm_enable_regular_snaps();

    //pressEnter("will look for drv stage 4");
    phantom_find_drivers( 4 );

//trfs_testrq();

    run_init_functions( INIT_LEVEL_LATE );


    //init_wins(u_int32_t ip_addr);

    printf("\n\x1b[33m\x1b[44mPhantom " PHANTOM_VERSION_STR " (SVN rev %s) @ %s started\x1b[0m\n\n", svn_version(), phantom_uname.machine );

#if 1
    {
        hal_sleep_msec(60000*13);
        printf("\nWILL CRASH ON PURPOSE\n\n" );
        hal_sleep_msec(20000);
        hal_cpu_reset_real();
    }

    while(1)
        hal_sleep_msec(20000);

#else
    kernel_debugger();
#endif

    phantom_shutdown(0);
    //pressEnter("will reboot");

    return 0;
}


void phantom_save_vmem(void)
{
#ifdef ARCH_ia32
    phantom_check_disk_save_virtmem( (void *)hal_object_space_address(), CHECKPAGES );
#endif
}





// -----------------------------------------------------------------------
// TCP/IP stack
// -----------------------------------------------------------------------


static void net_stack_init()
{
    cbuf_init();

#if HAVE_NET
    SHOW_FLOW0( 1, "Init TCP/IP stack" );
    // used elsewhere, init earlier
    //net_timer_init();
    if_init();
    ethernet_init();
    arp_init();
    ipv4_init();
    loopback_init();
    udp_init();
    tcp_init();
    //socket_init();
    //net_control_init();

    resolver_init();

    phantom_trfs_init();

    start_tcp_echo_server();

#endif // HAVE_NET
}



void _exit(int code)
{
    (void) code;

    hal_cpu_reset_real();
}


// -----------------------------------------------------------------------
// Kill world
// -----------------------------------------------------------------------

void phantom_shutdown(int flags)
{
    static int reenter = 0;

    if( reenter++ )
    {
        while(1)
            hal_sleep_msec(1000);
    }

    SHOW_FLOW0( 0, "shutdown in 5 seconds" );
    hal_sleep_msec(5000);

    t_migrate_to_boot_CPU(); // Make sure other CPUs are stopped

    phantom_finish_all_threads(); // TODO all VM threads?

    run_stop_functions( STOP_LEVEL_EARLY );

    if( !( flags & SHUTDOWN_FLAG_NOSYNC ) )
    {
#ifdef ARCH_ia32
        phantom_check_disk_save_virtmem( (void *)hal_object_space_address(), CHECKPAGES );
#endif

        //pressEnter("finishing vm");
        vm_map_finish(); // Actually does snapshot
        vm_map_wait_for_finish();

        //pressEnter("will do a snap");
        run_stop_functions( STOP_LEVEL_PREPARE );
        run_stop_functions( STOP_LEVEL_STOP );


        //pressEnter("finishing pager");
        pager_finish();

        dpc_finish();
    }

#ifdef ARCH_ia32
    if(flags & SHUTDOWN_FLAG_REBOOT)
        acpi_reboot();
    else
    {
        acpi_powerdown();
        ia32_intel_poweroff(); // if failed - try hack
    }
#endif


    hal_sleep_msec(2000);
    SHOW_ERROR0( 0, "Can't shoutdown, will attempt reset" );

    hal_cpu_reset_real();
}


