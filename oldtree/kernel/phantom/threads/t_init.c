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

//static phantom_thread_t *idlest;

// This is not an ideal solution. This one
// will prevent other idle threads from running
static void haltme( void *a )
{
    (void) a;
    //hal_set_thread_priority( GET_CURRENT_THREAD()->tid, THREAD_PRIO_IDLE );

    hal_set_thread_name("<Halt_Me>");

    while(1)
    {
        //osenv_softintr_enable();
        hal_sti();
        asm volatile("hlt" : : );
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
    phantom_scheduler_init();

    phantom_thread_init_conds();
    phantom_thread_init_mutexes();

    // Create thread entry for this control flow
    phantom_import_main_thread();

    // Create idle (idlest!;) thread

    GET_IDLEST_THREAD() = phantom_create_thread( haltme, 0, THREAD_FLAG_KERNEL );

    // BUG! Bad hack! Fix and use hal_set_thread_priority() here!
    // BUG! On SMP will be really wrong!
    hal_cli();
    t_dequeue_runq(GET_IDLEST_THREAD());
    GET_IDLEST_THREAD()->priority = THREAD_PRIO_IDLE;
    t_enqueue_runq(GET_IDLEST_THREAD());
    hal_sti();

}

