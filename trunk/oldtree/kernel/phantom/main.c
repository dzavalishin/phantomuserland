/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel main
 *
 *
 **/

#define DEBUG_MSG_PREFIX "boot"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include "config.h"

#include "svn_version.h"

#include <i386/proc_reg.h>
#include <i386/trap.h>
#include <i386/pio.h>

#include <phantom_libc.h>
#include <phantom_time.h>

#include <kernel/init.h>

#include "hal.h"
#include "paging_device.h"
#include "vm_map.h"
#include "snap_sync.h"

#include "vm/root.h"
#include "video.h"
#include "misc.h"
#include "net.h"

#include "timedcall.h"

#include <sys/utsname.h>
#include <stdlib.h>

// pvm_bulk_init
#include <vm/bulk.h>

// pvm_memcheck
#include <vm/alloc.h>

// multiboot info
#include <multiboot.h>

// phys addr
#include <kernel/vm.h>



//#define N_OBJMEM_PAGES ((1024L*1024*128)/4096)
#define N_OBJMEM_PAGES ((1024L*1024*32)/4096)
#define CHECKPAGES 1000

















static void pressEnter(char *text)
{
    printf("%s\npress Enter...\n...", text);
    while( getchar() >= ' ' )
        ;
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
    pressEnter("finishing vm");
    vm_map_finish();
    vm_map_wait_for_finish();

    //pressEnter("finishing pager");
    pager_finish();

    dpc_finish();
}




static void load_classes_module();
static void net_stack_init();









#include "threads/thread_private.h"
#include "i386/interrupts.h"

void
phantom_scheduler_request_soft_irq()
{
    hal_request_softirq(SOFT_IRQ_THREADS);
    __asm __volatile("int $15");
}

void
phantom_scheduler_schedule_soft_irq()
{
    hal_request_softirq(SOFT_IRQ_THREADS);
}



static int ignore_handler(struct trap_state *ts)
{
    (void) ts;

    hal_sti(); // It works in open interrupts
    phantom_scheduler_soft_interrupt();
    // it returns with soft irqs disabled
    hal_enable_softirq();

    return 0;
}


static timedcall_t sched_timer =
{
    (void *)phantom_scheduler_time_interrupt,
    0, 20
};


/**
 *
 * Phantom kernel main. Called with (what?) initialized.
 *
**/


int main(int argc, char **argv, char **envp)
{
    (void) envp;

    init_irq_allocator();

    // BUG - called second time?
    phantom_init_descriptors();

    init_multiboot_symbols();

    hal_init(
             (void *)PHANTOM_AMAP_START_VM_POOL,
             N_OBJMEM_PAGES*4096L);

    // Stage is:
    //   0 - very early in the boot - interrupts can be used only
    //   1 - boot, most of kernel infrastructure is there
    //   2 - disks which Phantom will live in must be found here
    //   3 - late and optional and slow junk

    phantom_pci_find_drivers( 0 );

    phantom_timer_pit_init(100,0);

    phantom_paging_init();

    phantom_timed_call_init(); // Too late? Move up?


    // Threads startup
    {
    // HACK!!! Used to give away CPU in thread switch
    // Replace with?
    phantom_trap_handlers[15] = ignore_handler;
    phantom_threads_init();
    phantom_request_timed_call( &sched_timer, TIMEDCALL_FLAG_PERIODIC );
    hal_set_softirq_handler( SOFT_IRQ_THREADS, (void *)phantom_scheduler_soft_interrupt, 0 );
    }

    set_cr0( get_cr0() | CR0_WP );

    {
        extern const char* SVN_Version;
        extern struct utsname phantom_uname;
        strncpy( phantom_uname.release, SVN_Version, sizeof(phantom_uname.release) );

    }

    printf("\nPhantom " PHANTOM_VERSION_STR " (SVN ver %s) starting\n\n", svn_version() );
    phantom_process_boot_options();

    detect_cpu(0);

    // Used to refill list used to allocate physmem in interrupts
    hal_init_physmem_alloc_thread();

    net_stack_init();

    phantom_pci_find_drivers( 1 );

    init_main_event_q();

    phantom_init_vm86();
    phantom_init_vesa();
    phantom_start_video_driver();


    phantom_init_apic();
    //pressEnter("after APIC");


    phantom_pci_find_drivers( 2 );

#if HAVE_UNIX
    // Before boot modules init Unix-like environment
    // as modules have access to it
    phantom_unix_fs_init();


    // Start modules bootloader brought us
    phantom_start_boot_modules();
#endif // HAVE_UNIX

    //arch_get_rtc_delta(); // Read PC clock
    //getchar();
    dpc_init();

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

    phantom_check_disk_check_virtmem( (void *)hal_object_space_address(), CHECKPAGES );

    load_classes_module();

    phantom_pci_find_drivers( 3 );

#if HAVE_NET
    phantom_tcpip_active = 1; // Tell them we finished initing network
    net_test();
#endif

    // Start virtual machine in special startup (single thread) mode
    pvm_root_init();


    SHOW_FLOW0( 2, "Will run phantom threads... ");
    // Virtual machine will be run now in normal mode
    activate_all_threads();
    vm_enable_regular_snaps();

    phantom_pci_find_drivers( 4 );

    printf("PRESS Q TO STOP PHANTOM");
    while(getchar() != 'Q')
        ;

    phantom_finish_all_threads();

    phantom_check_disk_save_virtmem( (void *)hal_object_space_address(), CHECKPAGES );

    pressEnter("will do a snap");

    stop_phantom();

    pressEnter("will reboot");

    return 0;
}


void phantom_save_vmem(void)
{
    phantom_check_disk_save_virtmem( (void *)hal_object_space_address(), CHECKPAGES );
}




// -----------------------------------------------------------------------
// Boot module classloader support
// -----------------------------------------------------------------------



static void *bulk_code;
static unsigned int bulk_size;
static void *bulk_read_pos;

int bulk_seek_f( int pos )
{
    bulk_read_pos = bulk_code + pos;
    return bulk_read_pos >= bulk_code + bulk_size;
}

int bulk_read_f( int count, void *data )
{
    if( count < 0 )
        return -1;

    int left = (bulk_code + bulk_size) - bulk_read_pos;

    if( count > left )
        count = left;

    memcpy( data, bulk_read_pos, count );

    bulk_read_pos += count;

    return count;
}

static void load_classes_module()
{
    // In fact we need this only if boot classloader is called,
    // and it is called only if completely fresh system is set up
    struct multiboot_module *classes_module = phantom_multiboot_find("classes");

    SHOW_FLOW( 2, "Classes boot module is %sfound\n", classes_module ? "" : "not " );

    bulk_read_pos = bulk_code;
    bulk_size = 0;

    if(classes_module != 0)
    {
        bulk_code = (void *)phystokv(classes_module->mod_start);
        bulk_size = classes_module->mod_end - classes_module->mod_start;
    }
    else
        panic("no boot classes module found");

    pvm_bulk_init( bulk_seek_f, bulk_read_f );
}



// -----------------------------------------------------------------------
// TCP/IP stack
// -----------------------------------------------------------------------


static void net_stack_init()
{
#if HAVE_NET
    SHOW_FLOW0( 1, "Init TCP/IP stack" );
    cbuf_init();
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

    //beep(); // TODO test!
#endif // HAVE_NET
}



void _exit(int code)
{
    (void) code;

    hal_cpu_reset_real();
}
