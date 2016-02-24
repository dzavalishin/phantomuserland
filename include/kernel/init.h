/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Boot time init junkyard. Must be cleaned up somehow.
 *
 *
**/

#ifndef INIT_H
#define INIT_H

#include <errno.h>

/**
 * \ingroup Init
 * \defgroup Init Init - kernel startup
 * @{
**/

extern char arch_name[];
extern char board_name[];


struct init_record
{
    struct init_record *next;

    void (*init_1)(void);
    int         flags_1;

    void (*init_2)(void);
    int         flags_2;

    void (*init_3)(void);
    int         flags_3;
};

void register_init_record( struct init_record *ir );

#define INIT_ME(__init1,__init2,__init3) \
    static struct init_record __init_record = { 0, __init1, 0, __init2, 0, __init3, 0 }; \
    static void __register_init(void) __attribute__ ((constructor)); \
    static void __register_init(void) \
    { \
    register_init_record( &__init_record ); \
    }

#define STOP_ME(__stop1,__stop2,__stop3) \
    static struct init_record __stop_record = { 0, __stop1, 0, __stop2, 0, __stop3, 0 }; \
    static void __register_stop(void) __attribute__ ((constructor)); \
    static void __register_stop(void) \
    { \
    register_stop_record( &__stop_record ); \
    }


/*
#define INIT_NEED_NET           (1<< 0)
#define INIT_NEED_DISK          (1<< 1)
#define INIT_NEED_UI            (1<< 2)
#define INIT_NEED_THREADS       (1<< 3)
#define INIT_NEED_PORTS         (1<< 4)
#define INIT_NEED_UNIX          (1<< 5)
#define INIT_NEED_OBJECTS       (1<< 6)
*/

// PREPARE - no threads are running yet, don't call others
// After prepare all public interfaces must be callable
#define INIT_LEVEL_PREPARE      1
// INIT - threads are running, most of other stuff is callable
#define INIT_LEVEL_INIT         2
// LATE - all stuff is running: all drivers, video, persistent memory, virtual machine
#define INIT_LEVEL_LATE         3

// ?
#define STOP_LEVEL_EARLY        1
// Stop requesting others
#define STOP_LEVEL_PREPARE      2
// Stop serving
#define STOP_LEVEL_STOP         3

volatile int  phantom_stop_level; // zero on noraml operation, 1-2-3 on stop

void run_init_functions( int level );
void run_stop_functions( int level );


int phantom_find_drivers( int stage );



// Tests


void run_test( const char *test_name, const char *test_parm );



// Startup/init                               

#include <sys/types.h>

void __phantom_run_constructors(void);

void hal_init_vm_map(void);
void phantom_timed_call_init(void);
void phantom_timed_call_init2(void);

void phantom_heap_init(void);
//void hal_init_physmem_alloc( physaddr_t start, size_t npages );
void hal_init_physmem_alloc(void);
void hal_init_physmem_alloc_thread(void);

void hal_physmem_add( physaddr_t start, size_t npages );
void hal_physmem_add_low( physaddr_t start, size_t npages );


void phantom_map_mem_equally(void);

void hal_init_vm_map(void);
void phantom_timed_call_init(void);

int detect_cpu(int curr_cpu);

void hal_cpu_reset_real(void) __attribute__((noreturn));

void phantom_threads_init(void);


void phantom_load_gdt(void); // TODO ia32

void arch_debug_console_init(void);

void phantom_init_descriptors(void); // TODO ia32
void phantom_fill_idt(void); // TODO ia32
void phantom_load_idt(void); // TODO ia32

void phantom_init_vm86(void); // TODO ia32
void phantom_init_vesa(void); // TODO ia32

void phantom_init_apic(void); // TODO ia32

void phantom_paging_init(void);

int phantom_timer_pit_init(int freq, void (*timer_intr)()); // TODO ia32

void init_irq_allocator(void);

void phantom_start_video_driver(void);
void init_main_event_q(void);

void phantom_trfs_init(void); // TODO INIT_ME?

void phantom_turn_off_pic_scheduler_timer(void); // TODO ia32


void phantom_unix_proc_init(void);


void load_classes_module(void); // vm bulk classes init

void stray(void); // check for stray pointers

void phantom_init_part_pool(void);

void usb_setup(void);

void drv_video_init_windows(void);

void init_wins(u_int32_t ip_addr); // TODO INIT_ME


void arch_init_early(void);
//void board_init_early(void);
void arch_float_init(void);
void arch_threads_init(); // Do all that needed to prepare for thread switches

void heap_init_mutex( void );

void identify_cpu(void);
void identify_hypervisor(void);

struct multiboot_info;
void setSymtabBootParameters(struct multiboot_info *bpp);

void phantom_parse_cmd_line(const char* cmdline);
void phantom_process_boot_options(void);
void phantom_start_boot_modules(void);


errno_t vesa3_bootstrap(void);

errno_t InitializeFullAcpi(void);

void start_tcp_echo_server(void);


// -----------------------------------------------------------------------
// Finita
// -----------------------------------------------------------------------

#define SHUTDOWN_FLAG_REBOOT    (1<<0)
#define SHUTDOWN_FLAG_NOSYNC    (1<<1)

void phantom_shutdown(int flags);

#ifdef ARCH_ia32
void ia32_intel_poweroff(void);
#endif


#endif // INIT_H

