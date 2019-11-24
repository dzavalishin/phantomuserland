/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Hosted cond impl
 *
 *
**/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

typedef int errno_t;

#include <bits/errno_t.h>
#include <kernel/cond.h>
#include <kernel/atomic.h>

#include <pthread.h>
#include <assert.h>
#include <time.h>


#include "hst_private.h"


struct phantom_cond_impl
{
    pthread_cond_t c;
    const char *name;
    int lock;
};


static void *gen_hal_cond_init(const char * name)
{
    struct phantom_cond_impl *impl;
    impl = calloc(1, sizeof(struct phantom_cond_impl)+16); // to prevent corruption if kernel hal mutex func will be called
    if(impl == 0) return 0;

    int rc = pthread_cond_init( &(impl->c), 0 );
    if( rc )
    {
        perror("pthread_cond_init");
        exit(33);
    }

    impl->name = name;
    return impl;
}



static void gen_hal_cond_wait( void *c_impl, void *m_impl )
{
    struct phantom_cond_impl *cond = c_impl;
    struct phantom_mutex_impl *mutex = m_impl;

    int rc = pthread_cond_wait(&(cond->c), &(mutex->m));
    if( rc ) perror("pthread_cond_wait");
}


static void gen_hal_cond_twait( void *c_impl, void *m_impl, long msecTimeout )
{
    struct phantom_cond_impl *cond = c_impl;
    struct phantom_mutex_impl *mutex = m_impl;

    struct timespec stime;

    // TODO do we need to move seconds from tv_nsec to tv_sec
    stime.tv_sec = time(NULL);
    stime.tv_nsec = msecTimeout * 1000L;

    int rc = pthread_cond_timedwait(&(cond->c), &(mutex->m), &stime);
    if( rc ) perror("pthread_cond_wait");
}



static void gen_hal_cond_signal( void *c_impl )
{
    struct phantom_cond_impl *cond = c_impl;
    int rc = pthread_cond_signal(&(cond->c));
    if( rc ) perror("pthread_cond_wait");
}



static void gen_hal_cond_broadcast( void *c_impl )
{
    struct phantom_cond_impl *cond = c_impl;
    int rc = pthread_cond_broadcast(&(cond->c));
    if( rc ) perror("pthread_cond_wait");
}

static void gen_hal_cond_destroy(void *c_impl)
{
    struct phantom_cond_impl *cond = c_impl;
}
















int hal_cond_init( hal_cond_t *c, const char *name )
{
    c->impl = calloc(1, sizeof(struct phantom_cond_impl)+16); // to prevent corruption if kernel hal mutex func will be called
    //InitializeConditionVariable( &(c->impl.cv) );
    c->impl->name = name;
    return 0;
}


errno_t hal_cond_wait( hal_cond_t *c, hal_mutex_t *m )
{
    assert(c->impl);
hal_mutex_unlock(m);
    hal_sleep_msec(100);
    //SleepConditionVariableCS( &(c->impl.cv), &(m->impl->cs), 0 );
hal_mutex_lock(m);
    return 0;
}

errno_t hal_cond_timedwait( hal_cond_t *c, hal_mutex_t *m, long msecTimeout )
{
    assert(c->impl);
    hal_sleep_msec(msecTimeout);
    //SleepConditionVariableCS( &(c->impl.cv), &(m->impl->cs), msecTimeout );
    return 0;
}


errno_t hal_cond_signal( hal_cond_t *c )
{
    assert(c->impl);
    //WakeConditionVariable( &(c->impl.cv) );
    return 0;
}

errno_t hal_cond_broadcast( hal_cond_t *c )
{
    assert(c->impl);
    //WakeAllConditionVariable( &(c->impl->cv) );
    return 0;
}

errno_t hal_cond_destroy(hal_cond_t *c)
{

    //if(m->impl.owner != 0)        panic("locked mutex killed");
    free(c->impl);
    c->impl=0;

    return 0;
}











