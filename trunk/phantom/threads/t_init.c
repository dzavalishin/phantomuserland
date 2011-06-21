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

#include "thread_private.h"
#include <kernel/init.h>


static volatile int have_idlest = 0;

//static phantom_thread_t *idlest;

// This is not an ideal solution. This one
// will prevent other idle threads from running
static void haltme( void *a )
{
    (void) a;
    //hal_set_thread_priority( GET_CURRENT_THREAD()->tid, THREAD_PRIO_IDLE );


    // Hack - this thread can't call malloc (it has sema and we can't wait), so set name manually
    //hal_set_thread_name("<Halt_Me>");
    GET_CURRENT_THREAD()->name = "<Halt_Me>";
    // BUG - next call to hal_set_thread_name will crash due to attempt to free static string

    have_idlest = 1;

    while(1)
    {
        /*
        hal_disable_preemption();
        if( t_thread_kill_request )
            t_do_some_kills();
        hal_enable_preemption();
        */
        hal_sti();
        //asm volatile("hlt" : : );
        hal_wait_for_interrupt();
    }
}

static void kill_thread_thread( void )
{
    //(void) a;
    hal_set_current_thread_priority( THREAD_PRIO_IDLE );

    hal_set_thread_name("Init");

    while(1)
    {
        hal_disable_preemption();
        if( t_thread_kill_request )
            t_do_some_kills();
        hal_enable_preemption();

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
    phantom_thread_init_killer();

    // Create thread entry for this control flow
    phantom_import_main_thread();

    // Create idle (idlest!;) thread

    GET_IDLEST_THREAD() = phantom_create_thread( haltme, 0, THREAD_FLAG_KERNEL );
    GET_IDLEST_THREAD()->thread_flags |= THREAD_FLAG_NOSCHEDULE;

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

