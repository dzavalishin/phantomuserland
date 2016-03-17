/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Thread state dump.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include "thread_private.h"
#include <phantom_libc.h>





void dump_thread_info(phantom_thread_t *t)
{
    assert(threads_inited);

    printf("T%2d pri %02d blk %08X ",
           t->tid,
           t->priority,
           t->sleep_flags
          );

    if(t->sleep_flags & THREAD_SLEEP_MUTEX)
        printf("mutx %8p", t->waitmutex );

    if(t->sleep_flags & THREAD_SLEEP_COND)
        printf("cond %8p", t->waitcond );

    if(t->sleep_flags & THREAD_SLEEP_SEM)
        printf("sema %8p", t->waitsem );





    printf("\n");
}

void dump_thread_stacks()
{
    if(!threads_inited) return;

    int i;
    for( i = 0; i < MAX_THREADS; i++ )
    {
        if( phantom_kernel_threads[i] == 0 )
            continue;
        phantom_thread_t *t = phantom_kernel_threads[i];

        dump_thread_info(t);
        dump_thread_stack(t);
    }
}


void phantom_dump_threads()
{
    if(!threads_inited) return;

    int i;
    for( i = 0; i < MAX_THREADS; i++ )
    {
        if( phantom_kernel_threads[i] == 0 )
            continue;

        phantom_thread_t *t = phantom_kernel_threads[i];
        dump_thread_info(t);
    }
}








static int dump_thread_info_buf(char *buf, int len, phantom_thread_t *t)
{
    int rc, ret = 0;

    *buf = '\0';

    const char *slp = "?";
    const char *scol = "";

    switch(t->sleep_flags)
    {
    case 0:                     slp = " RUN "; scol = "\x1b[31m"; break;
    case THREAD_SLEEP_USER:     slp = "user"; break;
    case THREAD_SLEEP_SLEEP:    slp = "sleep"; scol = "\x1b[34m"; break;
    case THREAD_SLEEP_COND:     slp = "cond"; break;
    case THREAD_SLEEP_MUTEX:    slp = "mutex"; break;
    case THREAD_SLEEP_SEM:      slp = "sema"; break;
    case THREAD_SLEEP_LOCKED:   slp = "lock"; break;
    case THREAD_SLEEP_IO:       slp = "io"; scol = "\x1b[33m"; break;
    };

    rc = snprintf(buf, len, " %2d %02d %s%-5.5s %d %-11.11s \x1b[37m",
                  t->tid,
                  t->priority,

                  scol, slp,
                  t->cpu_id,
                  t->name ? t->name : "--"
          );

    len -= rc;
    ret += rc;
    buf += rc;

    if(t->sleep_flags & THREAD_SLEEP_MUTEX)
    {
        //rc = snprintf(buf, len, "%8p %s", t->waitmutex, t->waitmutex->impl->name );
        rc = snprintf(buf, len, "\x1b[35m%s\x1b[37m", t->waitmutex->impl->name );
        len -= rc;
        ret += rc;
        buf += rc;
    }

    if(t->sleep_flags & THREAD_SLEEP_COND)
    {
        //rc = snprintf(buf, len, "%8p %s", t->waitcond, t->waitcond->impl->name );
        rc = snprintf(buf, len, "%s", t->waitcond->impl->name );
        len -= rc;
        ret += rc;
        buf += rc;
    }

#if USE_NEW_SEMAS
    if(t->sleep_flags & THREAD_SLEEP_SEM)
    {
        //rc = snprintf(buf, len, "%8p %s", t->waitsem, t->waitsem->impl->name );
        rc = snprintf(buf, len, "%s", t->waitsem->impl->name );
        len -= rc;
        ret += rc;
        buf += rc;
    }
#endif

    if( len > 0 )
    {
        *buf++ = '\n';
        len--;
        ret++;
    }

    return ret;
}

void phantom_dump_threads_buf(char *buf, int len)
{
    int rc;
    rc = snprintf(buf, len, "\x1b[35m Id Pr State CPU Name       Locked\x1b[37m\n");
    buf += rc;
    len -= rc;


    int i;
    for( i = 0; i < MAX_THREADS; i++ )
    {
        // BUG! Races with thread kill code
        if( phantom_kernel_threads[i] == 0 )
            continue;

        phantom_thread_t t = *phantom_kernel_threads[i];

        if( t.sleep_flags & THREAD_SLEEP_ZOMBIE )
            continue;

        rc = dump_thread_info_buf(buf,len,&t);
        buf += rc;
        len -= rc;
    }

    if(len--)
        *buf++ = 0;
}



// -----------------------------------------------------------------------
//
// Dump thread list or one thread state to JSON
//
// -----------------------------------------------------------------------

#include <kernel/json.h>



static void json_encode_thread( json_output *jo, void *el )
{
    phantom_thread_t *tp = *(phantom_thread_t**)el;

    if( tp == 0 )
    {
        json_out_string( jo, "exist", "no" );
        return;
    }


    json_out_string( jo, "cpu", "-" );
    json_out_delimiter( jo );

    /** NB! Exactly first! Accessed from asm. */
    //cpu_state_save_t            cpu;

    //! on which CPU this thread is dispatched now
    json_out_int( jo, "cpu_id", tp->cpu_id );
    json_out_delimiter( jo );

    json_out_int( jo, "tid", tp->tid );
    json_out_delimiter( jo );

    //! phantom thread ref, etc
    json_out_int( jo, "owner", (natural_t)tp->owner );
    json_out_delimiter( jo );

    //! "controlling" tty
#if CONF_NEW_CTTY
    json_out_int( jo, "ctty", (natural_t)tp->ctty_h );
#else
    json_out_int( jo, "ctty", (natural_t)tp->ctty );
#endif
    json_out_delimiter( jo );


    json_out_int( jo, "pid", tp->pid );
    json_out_delimiter( jo );


    json_out_string( jo, "name", tp->name ? tp->name : "(?)" );
    json_out_delimiter( jo );

    //void *                      stack;
    //physaddr_t                  stack_pa;
    json_out_int( jo, "stack_size", tp->stack_size );
    json_out_delimiter( jo );


    // This is kernel/interrupt stack.
    // We patch it into the TSS on thread switch so that if 386 will
    // for some reason switch from ring 1-3 to ring 0, this stack is used.
    // It seems to be not needed for ring 0 only threads, but lets have it
    // this way now.
    //void *                      kstack;
    //physaddr_t                  kstack_pa;
    //void *                      kstack_top; // What to load to ESP
    json_out_int( jo, "kstack_size", tp->kstack_size );
    json_out_delimiter( jo );

    //void *                      start_func_arg;
    //void                        (*start_func)(void *);

    json_out_int( jo, "thread_flags", tp->thread_flags );
    json_out_delimiter( jo );

    /** if this field is zero, thread is ok to run. */
    json_out_int( jo, "sleep_flags", tp->sleep_flags );
    json_out_delimiter( jo );

    /**
     * If we sleep on cond, here is it.
     * NB! sleep_flags & THREAD_SLEEP_COND is set and reset out of lock.
     * If you need atomic check, check waitcond under waitlock!
     */
    //hal_cond_t *                waitcond;
    //hal_mutex_t *               waitmutex;
    //hal_mutex_t *               ownmutex;
    //hal_sem_t *                 waitsem;

    //hal_spinlock_t              waitlock;
    //json_out_int( jo, "twaitlock", tp->waitlock );
    //json_out_delimiter( jo );

    //queue_chain_t               chain; // used by mutex/cond code to chain waiting threads
    //queue_chain_t               kill_chain; // used kill code to chain threads to kill

    //* Used to wake with timer, see hal_sleep_msec
    //timedcall_t                 sleep_event;

    json_out_int( jo, "snap_lock", tp->snap_lock );
    json_out_delimiter( jo );

    json_out_int( jo, "preemption_disabled", tp->preemption_disabled );
    json_out_delimiter( jo );

    //int                         child_tid; // used by fork code


    //! void (*handler)( phantom_thread_t * )
    //void *                      death_handler; // func to call if thread is killed

    //! Func to call on trap (a la unix signals), returns nonzero if can't handle
    //int 			(*trap_handler)( int sig_no, struct trap_state *ts );
    // --------------------------------------------------------------
    // Scheduler part
    // --------------------------------------------------------------

    /** THREAD_PRIO_xxx */
    json_out_int( jo, "priority", tp->priority );
    json_out_delimiter( jo );

    /**
     *
     * How many (100HZ) ticks this thread can be on CPU before resched.
     * NB! Signed, so that underrun is not a problem.
     *
    **/
    json_out_int( jo, "ticks_left", tp->ticks_left );
    json_out_delimiter( jo );

    /** Used by runq only. Is not 0 if on runq. */
    //queue_chain_t		runq_chain;

    /** Will be unlocked just after this thread is switched off CPU */
    //hal_spinlock_t              *sw_unlock;

#if LATENCY_DEBUG
    //bigtime_t                   sleep_start;
    //bigtime_t                   max_sleep;
#endif

}



void json_dump_threads( json_output *jo )
{
    json_foreach( jo, "threads", phantom_kernel_threads, sizeof(phantom_thread_t *), MAX_THREADS, json_encode_thread );
}








