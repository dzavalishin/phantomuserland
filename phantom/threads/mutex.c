/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Sync: mutexes.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#define DEBUG_MSG_PREFIX "threads"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10


#include <phantom_libc.h>
#include <queue.h>
#include <malloc.h>
#include <hal.h>
#include <errno.h>

#include <thread_private.h>

#define VERIFY_DEADLOCK 0


static hal_spinlock_t init_lock;

static void checkinit(hal_mutex_t *m);

#if VERIFY_DEADLOCK
static void verify_mutex_deadlock(phantom_thread_t *t);
#endif

// ------------------------------------------------------------------
// Main (write) lock/unlock. Only one thread is allowed inside.
// ------------------------------------------------------------------


errno_t hal_mutex_lock(hal_mutex_t *m)
{
    assert_not_interrupt();

    if(m->impl == 0) checkinit(m);

    struct phantom_mutex_impl* mi = m->impl;

    // save & dis preemtion
    int ie = hal_save_cli();
    hal_disable_preemption();
    hal_spin_lock(&(mi->lock));

    if(mi->owner == 0)
    {
        mi->owner = GET_CURRENT_THREAD();
        GET_CURRENT_THREAD()->ownmutex = m; // to release on thread death
        goto ret;
    }

    if(mi->owner == GET_CURRENT_THREAD())
        panic("recursive mutex lock");

    t_queue_check(&(mi->waiting_threads), GET_CURRENT_THREAD());

    // TODO pri inherit here

    queue_enter(&(mi->waiting_threads), GET_CURRENT_THREAD(), phantom_thread_t *, chain);

    GET_CURRENT_THREAD()->waitmutex = m; // just for debug

#if VERIFY_DEADLOCK
    verify_mutex_deadlock(GET_CURRENT_THREAD());
#endif
    thread_block( THREAD_SLEEP_MUTEX, &(mi->lock) ); //-V112
    // returns on unblock
    goto nounlock;

ret:
    hal_spin_unlock(&(mi->lock));
nounlock:
    GET_CURRENT_THREAD()->waitmutex = 0; // just for debug
    hal_enable_preemption();
    if(ie) hal_sti();

    return 0;
}

errno_t hal_mutex_unlock(hal_mutex_t *m)
{
    if(m->impl == 0) panic("unlock of uninited mutex");

    struct phantom_mutex_impl* mi = m->impl;

    // save & dis preemtion
    int ie = hal_save_cli();
    hal_disable_preemption();
    hal_spin_lock(&(mi->lock));

    if(mi->owner != GET_CURRENT_THREAD())
        panic("mutex - not owner");


    if (queue_empty(&(mi->waiting_threads))) {
        mi->owner = 0;
        goto ret;
	}

    phantom_thread_t *next_owner = t_dequeue_highest_prio(&(mi->waiting_threads));

    mi->owner = next_owner;
    next_owner->ownmutex = m; // to release on thread death
    thread_unblock( next_owner, THREAD_SLEEP_MUTEX );

ret:
    hal_spin_unlock(&(mi->lock));
    hal_enable_preemption();
    if(ie) hal_sti();

    return 0;
}

void hal_mutex_unlock_if_owner(void)
{
    hal_mutex_t *m = GET_CURRENT_THREAD()->ownmutex;
    if( !m ) return;

    if(m->impl == 0) panic("unlock_if_owner of uninited mutex");

    struct phantom_mutex_impl* mi = m->impl;
    if( mi->owner != GET_CURRENT_THREAD())
        return;

    errno_t rc = hal_mutex_unlock(m);
    if( rc )
        SHOW_ERROR( 0, "failed to unlock mutex on thread death: %d", rc );
}

int
hal_mutex_is_locked(hal_mutex_t *m)
{
    if(m->impl == 0)
        return 0;

    struct phantom_mutex_impl* mi = m->impl;

    return mi->owner != 0;
}


#if 0

// ------------------------------------------------------------------
// Read (concurrent) locks - UNFINISHED, UNTESTED
//
// Any number of readers are allowed if no main (write) lock is not done.
// Main (write) lock will block until all the readers are finished. No
// new readers will be allowed to pass if write lock is waiting to be
// locked or locked.
//
// ------------------------------------------------------------------


errno_t hal_mutex_read_lock(hal_mutex_t *m)
{
    assert_not_interrupt();

    if(m->impl == 0) checkinit(m);

    struct phantom_mutex_impl* mi = m->impl;

    // save & dis preemtion
    int ie = hal_save_cli();
    hal_disable_preemption();
    hal_spin_lock(&(mi->lock));

    if(mi->owner != 0)
    {
        //mi->owner = GET_CURRENT_THREAD();
        mi->readers++;
        goto ret;
    }

    if(mi->owner == GET_CURRENT_THREAD())
        panic("recursive rmutex lock");

    t_queue_check(&(mi->waiting_threads), GET_CURRENT_THREAD());

    // TODO pri inherit here

    queue_enter(&(mi->waiting_threads), GET_CURRENT_THREAD(), phantom_thread_t *, chain);

    GET_CURRENT_THREAD()->waitmutex = m; // just for debug

#if VERIFY_DEADLOCK
    verify_mutex_deadlock(GET_CURRENT_THREAD());
#endif
    thread_block( THREAD_SLEEP_MUTEX, &(mi->lock) ); //-V112
    // returns on unblock
    goto nounlock;

ret:
    hal_spin_unlock(&(mi->lock));
nounlock:
    GET_CURRENT_THREAD()->waitmutex = 0; // just for debug
    hal_enable_preemption();
    if(ie) hal_sti();

    return 0;
}


#endif

// ------------------------------------------------------------------
// Create and destroy
// ------------------------------------------------------------------


void phantom_thread_init_mutexes(void)
{
    hal_spin_init(&init_lock);
}

errno_t hal_mutex_init(hal_mutex_t *m, const char *name )
{
    assert(threads_inited);
    assert(m->impl == 0);
    m->impl = calloc(1, sizeof(struct phantom_mutex_impl));
    if(m->impl == 0) return ENOMEM;

    struct phantom_mutex_impl *mi = m->impl;

    hal_spin_init(&(mi->lock));
    mi->owner = 0;
    mi->name = name;
    queue_init(&(mi->waiting_threads));
    return 0;
}


// BUG This is wrong - can't call malloc under spinlock (blocks)
static void checkinit(hal_mutex_t *m)
{
#if 1
    panic("init mutex!");
#else
    // in spinlock!

    int ie = hal_save_cli();
    hal_spin_lock(&init_lock);

    struct phantom_mutex_impl *mi = m->impl;
    if(mi != 0)
    {
        hal_spin_unlock(&init_lock);
        if(ie) hal_sti();
        return;
    }

    hal_mutex_init(m,"?Static");
    hal_spin_unlock(&init_lock);
    if(ie) hal_sti();
#endif
}


errno_t hal_mutex_destroy(hal_mutex_t *m)
{
    assert_not_interrupt();

    struct phantom_mutex_impl *mi = m->impl;

    if(mi->owner != 0)
        panic("locked mutex killed");
    free(mi);

    m->impl = 0;

    return 0;
}

// ------------------------------------------------------------------
// Verify
// ------------------------------------------------------------------


#if VERIFY_DEADLOCK
static void verify_mutex_deadlock(phantom_thread_t *t)
{
    hal_mutex_t *m = t->waitmutex;

    while (m)
    {
        struct phantom_mutex_impl* mi = m->impl;
        phantom_thread_t *o = mi->owner;
        assert(o != t);
        if (o->waitmutex == m)
            break;
        m = o->waitmutex;
    }
}
#endif

