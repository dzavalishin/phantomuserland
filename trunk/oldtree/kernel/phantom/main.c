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
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>
#include <kernel/board.h>

#include "svn_version.h"

//#include <i386/proc_reg.h>
//#include <i386/trap.h>
//#include <i386/pio.h>

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
#include "snap_sync.h"

#include "vm/root.h"
#include "video.h"
#include "misc.h"
#include "net.h"

#include <kernel/timedcall.h>

#include <sys/utsname.h>
#include <stdlib.h>

// pvm_bulk_init
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


static paging_device pdev;


void start_phantom()
{
    //pressEnter("will start Phantom");
    phantom_snap_threads_interlock_init();

    //pressEnter("will init paging dev");

    // TODO size?
    init_paging_device( &pdev, "wd1", 1024*20); //4096 );

    //pressEnter("will init pager");
    pager_init(&pdev);


    //pressEnter("will init VM map");
    // TODO BUG this +1 is due to allocator error:
    // allocator insists on trying to access the byte
    // just after the memory arena. Fix that and remove this +1 after.
    vm_map_init( N_OBJMEM_PAGES+1 );
}



static void
stop_phantom()
{
    //pressEnter("finishing vm");
    vm_map_finish();
    vm_map_wait_for_finish();

    //pressEnter("finishing pager");
    pager_finish();

    dpc_finish();
}




static void net_stack_init();












static int ignore_handler(struct trap_state *ts)
{
    (void) ts;

    //hal_sti(); // It works in open interrupts - NOOO! We carry user spinlock here, so we have to be in closed interrupts up to unlock!
    phantom_scheduler_soft_interrupt();
    // it returns with soft irqs disabled
    hal_enable_softirq();

    return 0;
}

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

#ifdef STRAY_CATCH_SIZE
    // check for stray ptrs here
    stray();
#endif

    init_irq_allocator();

    init_multiboot_symbols();

    hal_init(
             (void *)PHANTOM_AMAP_START_VM_POOL,
             N_OBJMEM_PAGES*4096L);

    detect_cpu(0);
    phantom_paging_init();

    phantom_init_stat_counters();

    board_init_kernel_timer();
    phantom_timed_call_init(); // Too late? Move up?

    //init_buses();

    // Stage is:
    //   0 - very early in the boot - interrupts can be used only
    //   1 - boot, most of kernel infrastructure is there
    //   2 - disks which Phantom will live in must be found here
    //   3 - late and optional and slow junk

    phantom_pci_find_drivers( 0 );

    // Threads startup
    {
    // HACK!!! Used to give away CPU in thread switch
    // Replace with?
    phantom_trap_handlers[15] = ignore_handler;
    phantom_threads_init();
#if !DRIVE_SCHED_FROM_RTC // run from int 8 - rtc timer
    phantom_request_timed_call( &sched_timer, TIMEDCALL_FLAG_PERIODIC );
#endif
    hal_set_softirq_handler( SOFT_IRQ_THREADS, (void *)phantom_scheduler_soft_interrupt, 0 );
    }

    board_start_smp();

    {
        extern const char* SVN_Version;
        extern struct utsname phantom_uname;
        strncpy( phantom_uname.release, SVN_Version, sizeof(phantom_uname.release) );
    }

    printf("\nPhantom " PHANTOM_VERSION_STR " (SVN ver %s) starting\n\n", svn_version() );
    phantom_process_boot_options();

    dbg_init(); // Kernel command line debugger

    // Used to refill list used to allocate physmem in interrupts
    hal_init_physmem_alloc_thread();

    port_init();
    //pressEnter("will start net");
    net_stack_init();

    //pressEnter("will look for drv stage 1");
    phantom_pci_find_drivers( 1 );

    init_main_event_q();

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

    pressEnter("will look for drv stage 2");
    phantom_pci_find_drivers( 2 );

#if HAVE_UNIX
    // Before boot modules init Unix-like environment
    // as modules have access to it
    phantom_unix_fs_init();
    phantom_unix_proc_init();

    // Start modules bootloader brought us
    phantom_start_boot_modules();
#endif // HAVE_UNIX

    //arch_get_rtc_delta(); // Read PC clock
    pressEnter("will run DPC");
    dpc_init();

#ifdef STRAY_CATCH_SIZE
    init_stray_checker();
#endif


    pressEnter("will run phantom_timed_call_init2");
    //phantom_timed_call_init2();
    pressEnter("will run phantom_init_stat_counters2");
    //phantom_init_stat_counters2();

    // -----------------------------------------------------------------------
    // If this is test run, switch to test code
    // -----------------------------------------------------------------------

    if( argc >= 3 && (0 == strcmp( argv[1], "-test" )) )
    {
        SHOW_FLOW0( 0, "Sleep before tests to settle down boot activities" );
        hal_sleep_msec( 2000 );
        SHOW_FLOW( 0, "Will run '%s' test", argv[2] );
        run_test( argv[2], argv[3] );
        SHOW_FLOW0( 0, "Test done, reboot");
        exit(0);
    }

    // -----------------------------------------------------------------------
    // Now starting object world infrastructure
    // -----------------------------------------------------------------------


    //pressEnter("will start phantom");
    start_phantom();

#ifdef ARCH_ia32
    phantom_check_disk_check_virtmem( (void *)hal_object_space_address(), CHECKPAGES );
#endif

    load_classes_module();

    phantom_pci_find_drivers( 3 );

#if HAVE_NET
    phantom_tcpip_active = 1; // Tell them we finished initing network
    net_test();
#endif



#ifdef ARCH_ia32
connect_ide_io();
#endif

    // Start virtual machine in special startup (single thread) mode
    pvm_root_init();

    // just test
    //phantom_smp_send_broadcast_ici();
//init_tetris();

    //pressEnter("will run vm threads");
    SHOW_FLOW0( 2, "Will run phantom threads... ");
    // Virtual machine will be run now in normal mode
    activate_all_threads();
    vm_enable_regular_snaps();

    //pressEnter("will look for drv stage 4");
    phantom_pci_find_drivers( 4 );

//trfs_testrq();

    init_buses();

    // pool.ntp.org
    //init_sntp( IPV4_DOTADDR_TO_ADDR(85,21,78,91), 10000 );
    //init_sntp( IPV4_DOTADDR_TO_ADDR(192,168,1,1), 10000 );


#if 1
    /*
    printf("PRESS Q TO STOP PHANTOM");
    while(getchar() != 'Q')
    ;
    */

    while(1)
        hal_sleep_msec(20000);

#else
    kernel_debugger();
#endif

    phantom_finish_all_threads();

#ifdef ARCH_ia32
    phantom_check_disk_save_virtmem( (void *)hal_object_space_address(), CHECKPAGES );
#endif

    //pressEnter("will do a snap");

    stop_phantom();

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
    net_timer_init();
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


    //beep(); // TODO test!
#endif // HAVE_NET
}



void _exit(int code)
{
    (void) code;

    hal_cpu_reset_real();
}
