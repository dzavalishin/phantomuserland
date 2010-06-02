/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Sycn: conds.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include <queue.h>
#include <hal.h>
#include <malloc.h>
#include <errno.h>

#include "thread_private.h"




// pseudocode yet

static hal_spinlock_t init_lock;

void phantom_thread_init_conds(void)
{
    hal_spin_init(&init_lock);
}


errno_t hal_cond_init(hal_cond_t *c, const char *name )
{
    c->impl = calloc(1, sizeof(struct phantom_cond_impl));
    if(c->impl == 0) return ENOMEM;

    struct phantom_cond_impl *ci = c->impl;

    ci->name = name;
    hal_spin_init(&(ci->lock));
    queue_init(&(ci->waiting_threads));
    return 0;
}

static void checkinit(hal_cond_t *c)
{
    // in spinlock!

    hal_spin_lock(&init_lock);

    struct phantom_cond_impl *ci = c->impl;
    if(ci != 0)
    {
        hal_spin_unlock(&init_lock);
        return;
    }

    //ci = calloc(1, sizeof(struct phantom_cond_impl));
    //c->impl = ci;

    hal_cond_init(c,"?Static");

    hal_spin_unlock(&init_lock);
}




errno_t hal_cond_wait(hal_cond_t *c, hal_mutex_t *m)
{
    if(c->impl == 0) checkinit(c);
    struct phantom_cond_impl *ci = c->impl;

    // save & dis preemtion
    int ie = hal_save_cli();
    hal_disable_preemption();
    hal_spin_lock(&(ci->lock));


    phantom_thread_t *t = GET_CURRENT_THREAD();

    t_queue_check(&(ci->waiting_threads), t);
    queue_enter(&(ci->waiting_threads), t, phantom_thread_t *, chain);

    hal_mutex_unlock(m);


    hal_spin_lock(&t->waitlock);
    hal_spin_unlock(&(ci->lock));

    t->waitcond   = c;

    thread_block( THREAD_SLEEP_COND, &t->waitlock );

    //hal_spin_unlock(&t->waitlock);

//ret:
    //hal_spin_unlock(&(ci->lock));
    hal_enable_preemption();
    if(ie) hal_sti();

    hal_mutex_lock(m);

    return 0;
}


// Called from timer to wake thread on timeout
static void wake_cond_thread( void *arg )
{
    phantom_thread_t *t = get_thread( (int)arg ); // arg is tid
    struct phantom_cond_impl *ci = t->waitcond->impl;

    queue_remove(&(ci->waiting_threads), t, phantom_thread_t *, chain);

    t->thread_flags |= THREAD_FLAG_TIMEDOUT;
    thread_unblock( t, THREAD_SLEEP_COND );
}



errno_t hal_cond_timedwait( hal_cond_t *c, hal_mutex_t *m, long msecTimeout )
{
    if(c->impl == 0) checkinit(c);
    struct phantom_cond_impl *ci = c->impl;

    // save & dis preemtion
    int ie = hal_save_cli();
    hal_disable_preemption();
    hal_spin_lock(&(ci->lock));


    phantom_thread_t *t = GET_CURRENT_THREAD();

    t_queue_check(&(ci->waiting_threads), t);
    queue_enter(&(ci->waiting_threads), t, phantom_thread_t *, chain);

    hal_mutex_unlock(m);


    hal_spin_lock(&t->waitlock);
    hal_spin_unlock(&(ci->lock));

    t->waitcond   = c;

    // Now prepare timer
#warning if thread is killed this must be undone!
    t->sleep_event.lockp = &t->waitlock;
    t->sleep_event.msecLater = msecTimeout;
    t->sleep_event.f = wake_cond_thread;
    t->sleep_event.arg = (void *)t->tid;

    t->thread_flags &= ~THREAD_FLAG_TIMEDOUT;


    phantom_request_timed_call( &t->sleep_event, TIMEDCALL_FLAG_CHECKLOCK );


    thread_block( THREAD_SLEEP_COND, &t->waitlock );

    int retcode = (t->thread_flags & THREAD_FLAG_TIMEDOUT) != 0;
    t->thread_flags &= ~THREAD_FLAG_TIMEDOUT;


    hal_enable_preemption();
    if(ie) hal_sti();

    hal_mutex_lock(m);

    return retcode;
}









errno_t hal_cond_signal(hal_cond_t *c)
{

    if(c->impl == 0) checkinit(c);
    struct phantom_cond_impl *ci = c->impl;

    // save & dis preemtion
    int ie = hal_save_cli();
    hal_disable_preemption();
    hal_spin_lock(&(ci->lock));

    if (queue_empty(&(ci->waiting_threads)))
        goto ret;

    phantom_thread_t *next_waiter = t_dequeue_highest_prio(&(ci->waiting_threads));
    hal_spin_unlock(&(ci->lock));

    assert(next_waiter != 0);

    //hal_spin_lock(&(t->waitlock));
    next_waiter->waitcond = 0;
    //hal_spin_unlock(&(t->waitlock));

    thread_unblock( next_waiter, THREAD_SLEEP_COND );

    goto ena;


ret:
    hal_spin_unlock(&(ci->lock));
ena:
    hal_enable_preemption();
    if(ie) hal_sti();

    return 0;
}


errno_t hal_cond_broadcast(hal_cond_t *c)
{
    if(c->impl == 0) checkinit(c);
    struct phantom_cond_impl *ci = c->impl;

    // save & dis preemtion
    int ie = hal_save_cli();
    hal_disable_preemption();
    hal_spin_lock(&(ci->lock));

    if (queue_empty(&(ci->waiting_threads)))
        goto ret;


    phantom_thread_t *nextt;
    queue_iterate(&(ci->waiting_threads), nextt, phantom_thread_t *, chain)
    {
        if(nextt == GET_CURRENT_THREAD())
            panic("cond: wake up self?");

        nextt->waitcond = 0;
        thread_unblock( nextt, THREAD_SLEEP_COND );
    }

    queue_init(&(ci->waiting_threads));

ret:
    hal_spin_unlock(&(ci->lock));
//ena:
    hal_enable_preemption();
    if(ie) hal_sti();

    return 0;
}


errno_t hal_cond_destroy(hal_cond_t *c)
{
    //if(m->impl.owner != 0)        panic("locked mutex killed");
    free(c->impl);
    c->impl=0;

    return 0;
}
