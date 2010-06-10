/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Sleep.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#define DEBUG_MSG_PREFIX "threads"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <hal.h>
#include <phantom_libc.h>

#include "thread_private.h"
#include "../timedcall.h"

#include <phantom_libc.h>


void wake_sleeping_thread( void *arg )
{
    phantom_thread_t *t = get_thread( (int)arg ); // arg is tid

    if( t->sleep_event.lockp->lock )
    {
        SHOW_ERROR0( 0, "t->sleep_event.lockp.lock still locked");
    }

    thread_unblock( t, THREAD_SLEEP_SLEEP );
}

/**
 *
 * Blocks thread and registers callout to unblock it
 * required time later.
 *
**/


void
hal_sleep_msec( int timeMsec )
{
    phantom_thread_t *t = GET_CURRENT_THREAD();

    // save & dis preemtion
    int ie = hal_save_cli();
    hal_disable_preemption();

    hal_spin_lock(&t->waitlock);
    t->sleep_event.lockp = &t->waitlock;
    //hal_spin_lock(&t->sleep_event.lock);

    t->sleep_event.msecLater = timeMsec;
    t->sleep_event.f = wake_sleeping_thread;
    t->sleep_event.arg = (void *)t->tid;

    phantom_request_timed_call( &t->sleep_event, TIMEDCALL_FLAG_CHECKLOCK );

    //phantom_request_timed_func( wake_sleeping_thread, (void *)t->tid, timeMsec, 0 );

    thread_block( THREAD_SLEEP_SLEEP, t->sleep_event.lockp );

    hal_enable_preemption();
    if(ie) hal_sti();
}



