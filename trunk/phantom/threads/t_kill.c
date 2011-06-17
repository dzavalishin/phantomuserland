/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Thread kill.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/


#define DEBUG_MSG_PREFIX "thread"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10



//#include "../config.h"
#include "thread_private.h"

#include <malloc.h>
#include <hal.h>
#include <phantom_libc.h>



static hal_mutex_t      kill_mutex;
static hal_cond_t       kill_request;

int     t_thread_kill_request = 0;

/** threads to kill */
static queue_head_t	threads4kill = {0,0};


// TODO run special thread kill thread? It is possible that
// idlest thread won't run for a long time and we'll have
// backlog of non-killed threads...

// Alt'ly add some prio to idlest thread here on kill request?

void phantom_thread_init_killer(void)
{
    queue_init( &threads4kill );

    hal_cond_init( &kill_request, "threadKill" );
    hal_mutex_init( &kill_mutex, "threadKill" );
}

static errno_t t_do_kill_thread( phantom_thread_t * t );



void
hal_exit_kernel_thread(void)
{
    //panic("can't kill some kernel thread");
    t_kill_thread( GET_CURRENT_THREAD()->tid );
}

hal_spinlock_t dummy_lock;

errno_t t_kill_thread( int tid )
{
    phantom_thread_t * t = get_thread(tid);
    phantom_thread_t * curr = GET_CURRENT_THREAD();


    if( t != curr )
    {
        // not current thread - mark t as non-runnable by setting flag
        t->sleep_flags |= THREAD_SLEEP_ZOMBIE;
        return t_do_kill_thread( t );
    }
    else
    {
        hal_disable_preemption();

        // that's current thread - add it to kill queue and then mark as not runnable
        hal_mutex_lock( &kill_mutex );

        queue_enter( &threads4kill, t, phantom_thread_t *, kill_chain);
        //t->sleep_flags |= THREAD_SLEEP_ZOMBIE;

        hal_cond_signal( &kill_request );

        t_thread_kill_request++;

        hal_mutex_unlock( &kill_mutex );

        // TODO is it OK?
        hal_cli();
        hal_spin_lock(&dummy_lock);

        thread_block( THREAD_SLEEP_ZOMBIE, &dummy_lock );

        // Never comes here.
        panic("undead thread");
        //if(pe) hal_enable_preemption();
        //return 0;
    }

}


// SMP BUG - kill interlock works on one CPU only.
// on SMP it is possible for t_do_some_kills to run
// before thread that must be killed is really blocked.

void t_do_some_kills(void)
{
    phantom_thread_t * t;

    while(t_thread_kill_request)
    {
        hal_mutex_lock( &kill_mutex );

        assert( !queue_empty( &threads4kill ) );
        queue_remove_first( &threads4kill, t, phantom_thread_t *, kill_chain);

        // If not - just put it to the q end and retry later
        assert( t->sleep_flags & THREAD_SLEEP_ZOMBIE );

        t_thread_kill_request--;
        hal_mutex_unlock( &kill_mutex );

        t_do_kill_thread( t );
    }
}


// We are guaranteed that t is not curr thread and t to kill has

static errno_t t_do_kill_thread( phantom_thread_t * t )
{
    SHOW_FLOW( 2, "Kill thread tid %d", t->tid );

    assert( !(t->thread_flags & THREAD_FLAG_UNDEAD) );
    assert( t->sleep_flags & THREAD_SLEEP_ZOMBIE );
    assert( t != GET_CURRENT_THREAD() );

    if(t->death_handler)
    {
        void (*handler)( phantom_thread_t * ) = t->death_handler;
        handler(t);
    }


    // Cleanup all the stuff

    // If any timed call is scheduled - kill it
    phantom_undo_timed_call( &t->sleep_event );

    // Cond?
    if(t->waitcond)
    {
    }


    // Keeps mutexes? CAN'T CHECK!

    /*
    // TODO Sem unlock?!
    if(t->waitsem)
    {
        SHOW_ERROR0( 0, "Killing thread w sem?!" );
    }
    */


    if(t->tid == 1)
    {
        SHOW_ERROR0( 0, "Thread number one exits??" );
    }
    else
    {
        // Free stack
        // free( t->stack );
        hal_pv_free( t->stack_pa, t->stack, t->stack_size );
        hal_pv_free( t->kstack_pa, t->kstack, t->kstack_size );
    }

    // Remove from thread array
    phantom_kernel_threads[t->tid] = 0;


    if( t->name )
        free((char *)t->name);

    // Free threead struct
    free( t );
    return 0;
}


