/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Initialization.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include <thread_private.h>
#include <kernel/init.h>
#include <kernel/profile.h>


int threads_inited = 0;

static volatile int have_idlest = 0;

//static phantom_thread_t *idlest;

// This is not an ideal solution. This one
// will prevent other idle threads from running
static void haltme( void *a )
{
    (void) a;
    //hal_set_thread_priority( GET_CURRENT_THREAD()->tid, THREAD_PRIO_IDLE );


    // Hack - this thread can't call malloc (it has sema and we can't wait), so set name manually
    //t_current_set_name("<Halt_Me>");
    GET_CURRENT_THREAD()->name = "<Halt_Me>";
    // BUG - next call to t_current_set_name will crash due to attempt to free static string

    have_idlest = 1;
    //int cpu = GET_CPU_ID();

    while(1)
    {
        hal_sti();

        //percpu_idle_status[cpu] = 1;
        hal_wait_for_interrupt();
        //percpu_idle_status[cpu] = 0; // TODO wrong - we have to do it in shceduler
    }
}

static void kill_thread_thread( void )
{
    //(void) a;
    //hal_set_current_thread_priority( THREAD_PRIO_IDLE );
    //hal_set_current_thread_priority( THREAD_PRIO_LOW );
    t_current_set_priority( THREAD_PRIO_LOW );
    t_current_set_name("Init");

    while(1)
    {
        //hal_disable_preemption();
        //if( t_thread_kill_request )
        t_do_some_kills();
        //hal_enable_preemption();

        hal_sti();
        hal_wait_for_interrupt();
    }
}


/**
 *
 * Init threads/scheduler.
 *
 * Requires: interrupts, softint, malloc.
 *
**/

void
phantom_threads_init()
{
    //hal_spin_init(&tidlock);


    phantom_scheduler_init();

    phantom_thread_init_conds();
    phantom_thread_init_mutexes();
    phantom_thread_init_sems();
    //phantom_thread_init_killer();

    threads_inited = 1; // phantom_import_main_thread needs it...

#if CONF_NEW_CTTY
    t_init_ctty_pool();
#endif


#if NEW_SNAP_SYNC
    phantom_thread_init_snapper_interlock();
#endif

    // Create thread entry for this control flow
    phantom_import_main_thread();

    // Create idle (idlest!;) thread

    GET_IDLEST_THREAD() = phantom_create_thread( haltme, 0, THREAD_FLAG_KERNEL );
    GET_IDLEST_THREAD()->thread_flags |= THREAD_FLAG_NOSCHEDULE;

    //threads_inited = 1;

    phantom_thread_init_killer();

    // BUG! Bad hack! Fix and use hal_set_thread_priority() here!
    // BUG! On SMP will be really wrong!
    hal_cli();
    t_dequeue_runq(GET_IDLEST_THREAD());
    GET_IDLEST_THREAD()->priority = THREAD_PRIO_IDLE;
    t_enqueue_runq(GET_IDLEST_THREAD());
    hal_sti();

    //while( !have_idlest )        hal_sleep_msec(10);

    hal_start_kernel_thread(kill_thread_thread);

}

