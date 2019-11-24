/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Hosted mutex impl
 *
 *
**/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

typedef int errno_t;

#include <bits/errno_t.h>
#include <kernel/mutex.h>
#include <kernel/atomic.h>

#include <pthread.h>
#include <assert.h>

#include "hst_private.h"



static void * gen_hal_mutex_init(const char *name)
{
    struct phantom_mutex_impl *impl;
    impl = calloc(1, sizeof(struct phantom_mutex_impl)+16); // to prevent corruption if kernel hal mutex func will be called
    if(impl == 0) return 0;

//    InitializeCriticalSection( &(impl->cs) );
    int rc = pthread_mutex_init( &(impl->m), 0 );
    if( rc )
    {
        perror("unix_hal_init pthread_mutex_init");
        exit(33);
    }

    impl->name = name;
    return impl;
}

static int gen_hal_mutex_lock(void *_m)
{
    struct phantom_mutex_impl *impl = _m;
    impl->lock++;
//    EnterCriticalSection( &(impl->cs) );
    int rc = pthread_mutex_lock(&(impl->m));
    if( rc )
        perror("gen_hal_mutex_lock");
    return 0;
}

static int gen_hal_mutex_unlock(void *_m)
{
    struct phantom_mutex_impl *impl = _m;
//    LeaveCriticalSection( &(impl->cs) );
    int rc = pthread_mutex_unlock(&(impl->m));
    if( rc )
        perror("gen_hal_mutex_unlock");
    impl->lock--;
    return 0;
}

static int gen_hal_mutex_is_locked(void *_m)
{
    struct phantom_mutex_impl *impl = _m;
    return impl->lock;
}








int hal_mutex_init(hal_mutex_t *m, const char *name)
{
    m->impl = gen_hal_mutex_init(name);
    assert( m->impl );
    return 0;
}

int hal_mutex_lock(hal_mutex_t *m)
{
    assert(m->impl);
    return gen_hal_mutex_lock(m->impl);
}

int hal_mutex_unlock(hal_mutex_t *m)
{
    assert(m->impl);
    return gen_hal_mutex_unlock(m->impl);
}


int hal_mutex_is_locked(hal_mutex_t *m)
{
    assert(m->impl);
    return gen_hal_mutex_is_locked(m->impl);
}


errno_t hal_mutex_destroy(hal_mutex_t *m)
{
    struct phantom_mutex_impl *mi = m->impl;

    //if(mi->owner != 0)        panic("locked mutex killed");
    free(mi);

    m->impl = 0;

    return 0;
}



