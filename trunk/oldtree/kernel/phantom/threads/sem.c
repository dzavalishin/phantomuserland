/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Sync: semas.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include <queue.h>
#include <hal.h>
#include <malloc.h>
#include <errno.h>

#include "thread_private.h"

#if USE_NEW_SEMAS



static hal_spinlock_t init_lock;

void phantom_thread_init_sems(void)
{
    hal_spin_init(&init_lock);
}


errno_t hal_sem_init(hal_sem_t *c, const char *name )
{
    c->impl = calloc(1, sizeof(struct phantom_sem_impl));
    if(c->impl == 0) return ENOMEM;

    struct phantom_sem_impl *ci = c->impl;

    ci->name = name;
    hal_spin_init(&(ci->lock));
    queue_init(&(ci->waiting_threads));
    ci->value = 0;

    return 0;
}



static void checkinit(hal_sem_t *c)
{
    // in spinlock!

    hal_spin_lock(&init_lock);

    struct phantom_sem_impl *ci = c->impl;
    if(ci != 0)
    {
        hal_spin_unlock(&init_lock);
        return;
    }

    //ci = calloc(1, sizeof(struct phantom_sem_impl));
    //c->impl = ci;

    hal_sem_init(c,"?Static");

    hal_spin_unlock(&init_lock);
}




errno_t hal_sem_acquire(hal_sem_t *c)
{
    if(c->impl == 0) checkinit(c);
    struct phantom_sem_impl *ci = c->impl;

    // save & dis preemtion
    int ie = hal_save_cli();
    hal_disable_preemption();
    hal_spin_lock(&(ci->lock));

    phantom_thread_t *t = GET_CURRENT_THREAD();

    while( ci->value <= 0 )
    {
        t_queue_check(&(ci->waiting_threads), t);
        queue_enter(&(ci->waiting_threads), t, phantom_thread_t *, chain);


        hal_spin_lock(&t->waitlock);
        hal_spin_unlock(&(ci->lock));

        t->waitsem   = c;

        thread_block( THREAD_SLEEP_SEM, &t->waitlock );
        hal_spin_lock(&(ci->lock));
    }

    ci->value--;
    hal_spin_unlock(&(ci->lock));
    //hal_spin_unlock(&(ci->lock));
    hal_enable_preemption();
    if(ie) hal_sti();

    return 0;
}

// Called from timer to wake thread on timeout
static void wake_sem_thread( void *arg )
{
    phantom_thread_t *t = get_thread( (int)arg ); // arg is tid
    struct phantom_sem_impl *ci = t->waitsem->impl;

    //printf("wake sem\n");

    queue_remove(&(ci->waiting_threads), t, phantom_thread_t *, chain);

    t->thread_flags |= THREAD_FLAG_TIMEDOUT;
    thread_unblock( t, THREAD_SLEEP_SEM );
}



errno_t
hal_sem_acquire_etc( hal_sem_t *s, int val, int flags, long uSec )
//errno_t hal_sem_timedwait( hal_sem_t *c, hal_mutex_t *m, long msecTimeout )
{
    int retcode = 0;

    if(s->impl == 0) checkinit(s);
    struct phantom_sem_impl *ci = s->impl;

    long msec = ((uSec-1)/1000)+1;
    if( msec == 0 && uSec != 0 ) msec = 1; // at least 1

    // save & dis preemtion
    int ie = hal_save_cli();
    hal_disable_preemption();
    hal_spin_lock(&(ci->lock));

    phantom_thread_t *t = GET_CURRENT_THREAD();

    while( ci->value <= 0 )
    {
        t_queue_check(&(ci->waiting_threads), t);
        queue_enter(&(ci->waiting_threads), t, phantom_thread_t *, chain);

        hal_spin_lock(&t->waitlock);
        hal_spin_unlock(&(ci->lock));

        t->waitsem   = s;

        if(flags & SEM_FLAG_TIMEOUT )
        {
            // Now prepare timer
            t->sleep_event.lockp = &t->waitlock;
            t->sleep_event.msecLater = msec;
            t->sleep_event.f = wake_sem_thread;
            t->sleep_event.arg = (void *)t->tid;

            t->thread_flags &= ~THREAD_FLAG_TIMEDOUT;

            phantom_request_timed_call( &t->sleep_event, TIMEDCALL_FLAG_CHECKLOCK );
        }

        thread_block( THREAD_SLEEP_SEM, &t->waitlock );

        retcode = (t->thread_flags & THREAD_FLAG_TIMEDOUT) != 0;
        t->thread_flags &= ~THREAD_FLAG_TIMEDOUT;

        if(retcode) goto exit_on_timeout;

        hal_spin_lock(&(ci->lock));
    }

    ci->value -= val;
    hal_spin_unlock(&(ci->lock));
    //hal_spin_unlock(&(ci->lock));
exit_on_timeout:
    hal_enable_preemption();
    if(ie) hal_sti();

    return retcode ? ETIMEDOUT : 0;
}








void hal_sem_release(hal_sem_t *c)
{

    if(c->impl == 0) checkinit(c);
    struct phantom_sem_impl *ci = c->impl;

    // save & dis preemtion
    int ie = hal_save_cli();
    hal_disable_preemption();
    hal_spin_lock(&(ci->lock));

    ci->value++;
    //if( ci->value < 0 )         goto ret;

    if (queue_empty(&(ci->waiting_threads)))
    {
        hal_spin_unlock(&(ci->lock));
        goto ena;
    }

    phantom_thread_t *next_waiter = t_dequeue_highest_prio(&(ci->waiting_threads));
    hal_spin_unlock(&(ci->lock));

    assert(next_waiter != 0);

    //hal_spin_lock(&(t->waitlock));
    next_waiter->waitsem = 0;
    //hal_spin_unlock(&(t->waitlock));

    phantom_undo_timed_call( &next_waiter->sleep_event );
    thread_unblock( next_waiter, THREAD_SLEEP_SEM );

    goto ena;


ena:
    hal_enable_preemption();
    if(ie) hal_sti();
}

/*
errno_t hal_sem_broadcast(hal_sem_t *c)
{
    if(c->impl == 0) checkinit(c);
    struct phantom_sem_impl *ci = c->impl;

    // save & dis preemtion
    int ie = hal_save_cli();
    int pr = hal_disable_preemption_r();
    hal_spin_lock(&(ci->lock));

    if (queue_empty(&(ci->waiting_threads)))
        goto ret;


    phantom_thread_t *nextt;
    queue_iterate(&(ci->waiting_threads), nextt, phantom_thread_t *, chain)
    {
        if(nextt == GET_CURRENT_THREAD())
            panic("sem: wake up self?");

        nextt->waitsem = 0;
        thread_unblock( nextt, THREAD_SLEEP_COND );
    }

    queue_init(&(ci->waiting_threads));

ret:
    hal_spin_unlock(&(ci->lock));
//ena:
    if(pr) hal_enable_preemption();
    if(ie) hal_sti();

    return 0;
}
*/

errno_t sem_get_count(hal_sem_t *s, int *count)
{
    if(s->impl == 0 || count == 0)
        return EINVAL;

    //errno_t ret = 0;
    struct phantom_sem_impl *ci = s->impl;

    // save & dis preemtion
    int ie = hal_save_cli();
    hal_disable_preemption();
    hal_spin_lock(&(ci->lock));


    *count = ci->value;

//exit:
    hal_spin_unlock(&(ci->lock));
    hal_enable_preemption();
    if(ie) hal_sti();

    return 0;
}



// BUG! Races! 
void hal_sem_destroy(hal_sem_t *c)
{
    // BUG! Must unlock and signal killed sema! newos code relies on that
    //if(m->impl.owner != 0)        panic("locked mutex killed");
    free(c->impl);
    c->impl=0;
}

#endif // USE_NEW_SEMAS
