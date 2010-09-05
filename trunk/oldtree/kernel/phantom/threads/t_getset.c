/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Thread attributes access.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include "thread_private.h"
#include <hal.h>
#include <threads.h>
#include <phantom_libc.h>


/**
 *
 * Set given thread's priority.
 *
**/

void
hal_set_thread_priority( int tid, int prio )
{
    assert( prio >= 0 && prio <= (THREAD_PRIO_HIGHEST|THREAD_PRIO_MOD_REALTIME) );
    phantom_thread_t * t = get_thread(tid);
    assert(t != 0);
    t->priority = prio;

    if( t != GET_CURRENT_THREAD() )
        printf("Warning! Not self in hal_set_thread_priority()\n");

#warning wrong - thread possibly is on wrong run q!
    /*
    t_dequeue_runq(t);
    t_enqueue_runq(t);
    */
}

/**
 *
 * Get thread's priority.
 *
**/


int
hal_get_thread_priority( int tid )
{
    phantom_thread_t * t = get_thread(tid);
    assert(t != 0);
    return t->priority;
}


/**
 *
 * Set (for current thread) a handler, which will be
 * called if thread is killed in some way.
 *
**/


void
hal_set_thread_death_handler(void (*handler)( phantom_thread_t * ))
{
    GET_CURRENT_THREAD()->death_handler = (void *)handler;
}




/**
 *
 * Set (for current thread) thread name.
 *
**/


void
hal_set_thread_name(const char *name)
{
    GET_CURRENT_THREAD()->name = name;
}




struct wtty *get_thread_ctty( struct phantom_thread *t )
{
    return t->ctty;
}


