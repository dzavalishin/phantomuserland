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


int get_current_tid(void)
{
    assert(threads_inited);
    return get_current_thread()->tid;
}

static phantom_thread_t * get_n_thread(int tid)
{
    assert(threads_inited);
    assert(tid >=0 && tid <= MAX_THREADS);
    //assert(phantom_kernel_threads[tid] != 0);
    return phantom_kernel_threads[tid];
}


#define GETT() \
    phantom_thread_t *t = get_n_thread(tid); \
    if( t == 0 )                             \
    {                                        \
        ret = ESRCH;                         \
        goto err;                            \
    }


#define PRE()    \
    assert(threads_inited); \
    int ret = 0; \
    TA_LOCK();   \
    GETT()

#define POST()   \
    err:         \
    TA_UNLOCK(); \
    return ret;

errno_t t_set_owner( tid_t tid, void *owner )
{
    PRE()

    t->owner = owner;

    POST()
}

errno_t t_get_owner( tid_t tid, void **owner )
{
    assert(owner);
    PRE()
    *owner = t->owner;
    POST()
}



errno_t t_set_ctty( tid_t tid, struct wtty *ct )
{
    PRE()
    t->ctty = ct;
    POST()
}

errno_t t_get_ctty( tid_t tid, struct wtty **ct )
{
    PRE()
    *ct = t->ctty;
    POST()
}


errno_t t_set_pid( tid_t tid, pid_t pid )
{
    PRE()
    t->pid = pid;
    POST()
}

errno_t t_get_pid( tid_t tid, pid_t *pid )
{
    assert(pid);
    PRE()
    *pid = t->pid;
    POST()
}


// -----------------------------------------------------------
// old - to rewrite & kill



phantom_thread_t * get_current_thread(void)
{
    static phantom_thread_t dummy[MAX_CPUS];
    int ncpu = GET_CPU_ID();
    return percpu_current_thread[ncpu] ? percpu_current_thread[ncpu] : dummy + ncpu;
}



phantom_thread_t * get_thread(int tid)
{
    assert(threads_inited);
    assert(tid >=0 && tid <= MAX_THREADS);
    assert(phantom_kernel_threads[tid] != 0);
    return phantom_kernel_threads[tid];
}


/**
 *
 * Set given thread's priority.
 *
**/

errno_t
hal_set_thread_priority( int tid, int prio )
{
    assert(threads_inited);
    assert( prio >= 0 && prio <= (THREAD_PRIO_HIGHEST|THREAD_PRIO_MOD_REALTIME) );
    phantom_thread_t * t = get_thread(tid);
    assert(t != 0);

    if( t != GET_CURRENT_THREAD() )
    {
        printf("Warning! Not self in hal_set_thread_priority()\n");
        // wrong - thread possibly is on wrong run q!
        /*
         t_dequeue_runq(t);
         t_enqueue_runq(t);
         */
        return EINVAL;
    }

    t->priority = prio;

    return 0;
}

errno_t hal_set_current_thread_priority( int prio )
{
    assert(threads_inited);
    assert( prio >= 0 && prio <= (THREAD_PRIO_HIGHEST|THREAD_PRIO_MOD_REALTIME) );
    phantom_thread_t * t = GET_CURRENT_THREAD();
    t->priority = prio;
	return 0;
}

int hal_get_current_thread_priority()
{
    assert(threads_inited);
    phantom_thread_t * t = GET_CURRENT_THREAD();
    return t->priority;
}


/**
 *
 * Get thread's priority.
 *
**/


int
hal_get_thread_priority( int tid )
{
    assert(threads_inited);
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
    assert(threads_inited);
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
    assert(threads_inited);
    const char *old = GET_CURRENT_THREAD()->name;
    GET_CURRENT_THREAD()->name = name ? strdup( name ) : 0;
    if( old ) free((char *)old);
}


#if 0

struct wtty *get_thread_ctty( struct phantom_thread *t )
{
    assert(threads_inited);
    return t->ctty;
}

/*

void *get_thread_owner( struct phantom_thread *t )
{
    assert(threads_inited);
    return t->owner;
}


int get_thread_flags( struct phantom_thread *t )
{
    assert(threads_inited);
	return t->thread_flags;
}

*/

#endif

