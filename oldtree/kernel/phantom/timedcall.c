/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Call function from timer interrupt.
 * Delay is in msec.
 *
 *
**/

#define DEBUG_MSG_PREFIX "timer"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/init.h>
#include <kernel/timedcall.h>
#include <kernel/debug.h>

#include "misc.h"

#include <spinlock.h>
#include <assert.h>
#include <phantom_libc.h>


void dump_timed_call_queue();


static hal_spinlock_t 	timedcall_lock;
static queue_head_t    	tcEventQ;
static int              inited = 0;



void phantom_timed_call_init(void)
{
    hal_spin_init( &timedcall_lock );
    queue_init( &tcEventQ );

    inited = 1;

}

static void do_request_timed_call( timedcall_t *newEntry, u_int32_t flags )
{
    assert(!hal_is_sti());

    // Still call it later - for any case :)
    if( newEntry->msecLater < 0 ) newEntry->msecLater = 0;
    newEntry->msecMore = 0;
    newEntry->flags = flags;

    /* TIMEDCALL_FLAG_AUTOFREE requires call to free from interrupt :(
    if(
       (newEntry->flags & TIMEDCALL_FLAG_PERIODIC) &&
       (newEntry->flags & TIMEDCALL_FLAG_AUTOFREE)
      )
        panic("periodic autofree timed call requested");
    */

    //if( newEntry.next != 0)        panic("next != null in phantom_request_timed_call");

    if(queue_empty(&tcEventQ))
    {
        SHOW_FLOW0( 2, "put: empty");
        newEntry->msecMore = newEntry->msecLater;
        queue_enter_first( &tcEventQ, newEntry, timedcall_t *, chain);
        goto unlock;
    }

    {
        timedcall_t *first = (timedcall_t *)queue_first(&tcEventQ);
        if(newEntry->msecLater < first->msecMore)
        {
            SHOW_FLOW0( 2, "put: first");
            newEntry->msecMore = newEntry->msecLater;
            first->msecMore -= newEntry->msecMore;
            queue_enter_first( &tcEventQ, newEntry, timedcall_t *, chain);
            goto unlock;
        }
    }

    timedcall_t *ep;
    long msecSum = 0;
    queue_iterate( &tcEventQ, ep, timedcall_t *, chain)
    {
        if( newEntry->msecLater >= msecSum && newEntry->msecLater < msecSum + ep->msecMore )
        {
            //newEntry->msecMore = newEntry->msecLater - (msecSum + ep->msecMore); // wrong, gives negative times
            newEntry->msecMore = newEntry->msecLater - msecSum;
            ep->msecMore -= newEntry->msecMore;

            SHOW_FLOW( 2, "put: before %ld (%ld total)", ep->msecLater, msecSum );
            queue_enter_before( &tcEventQ, ep, newEntry, timedcall_t *, chain);
            //queue_enter_after( &tcEventQ, ep, newEntry, timedcall_t *, chain);
            goto unlock;
        }

        msecSum += ep->msecMore;
    }

    newEntry->msecMore = newEntry->msecLater - msecSum;
    queue_enter( &tcEventQ, newEntry, timedcall_t *, chain);
    SHOW_FLOW0( 2, "put: last");

unlock:
    ;
}


/**
 *  Call f( arg ) about msecLater miliseconds later.
 *  Func guaranteed to be called, and it will be called
 *  no earlier than asked.
**/

void phantom_request_timed_call( timedcall_t *entry, u_int32_t flags )
{
    int ie = hal_save_cli();
    hal_spin_lock( &timedcall_lock );

    do_request_timed_call( entry, flags );

    hal_spin_unlock( &timedcall_lock );
    if(ie) hal_sti();
}

static void local_request_timed_call( timedcall_t *entry, u_int32_t flags )
{
    int ie = hal_save_cli();
    do_request_timed_call( entry, flags );
    if(ie) hal_sti();
}



/*
static void signal_cond(void *arg)
{
    hal_cond_signal((hal_cond_t *)arg);
}

static void broadcast_cond(void *arg)
{
    hal_cond_broadcast((hal_cond_t *)arg);
}


void phantom_request_cond_signal( int msec, hal_cond_t *cond)
{
    phantom_request_timed_func( signal_cond, cond, msec, 0 );
}

void phantom_request_cond_broadcast( int msec, hal_cond_t *cond)
{
    phantom_request_timed_func( broadcast_cond, cond, msec, 0 );
}
*/



/* TIMEDCALL_FLAG_AUTOFREE requires call to free from interrupt :(

void phantom_request_timed_func( timedcall_func_t f, void *arg, int msecLater, u_int32_t flags )
{
    timedcall_t *req = calloc(1, sizeof(timedcall_t) );
    req->f = f;
    req->arg = arg;
    req->msecLater = msecLater;

    phantom_request_timed_call( req, flags|TIMEDCALL_FLAG_AUTOFREE );
}

*/


static void dputc(char c)
{
#if 0 || debug_level_flow > 2
    putchar(c);
#else
    (void) c;
#endif
}






// Called once in msec
void phantom_process_timed_calls()
{
    if(!inited) return;

    int ie = hal_save_cli();
    hal_spin_lock( &timedcall_lock );
    dputc('.');


    if(queue_empty(&tcEventQ))
    {
        dputc('-');
        goto unlock;
    }
    dputc('%');

    timedcall_t *ep = (timedcall_t *)queue_first(&tcEventQ);

    ep->msecMore--;

again:
    if( TIMEDCALL_FLAG_CHECKLOCK & ep->flags && ep->lockp->lock )
    {
        if(ep->msecMore < -500)
            panic("timedcall engine stall due to spinlock");
        //return;
        goto unlock;
    }

    if( ep->msecMore <= 0 )
    {
        dputc('!');

        // detach
        queue_remove(&tcEventQ, ep, timedcall_t *, chain);

        if( (ep->msecMore < 0) && !queue_empty(&tcEventQ) )
        {
            // It can happen if execution was deferred due to the lock
            timedcall_t *next_ep = (timedcall_t *)queue_first(&tcEventQ);
            // ep->msecMore is negative. Propagate extra time to next
            // event
            next_ep->msecMore += ep->msecMore;
        }

        // It is not supposed that callee will sleep, but for any case
        // unlock the spinlock
    	hal_spin_unlock( &timedcall_lock );
        //if(ie) hal_sti();
        // call
        if(ep->f) ep->f(ep->arg);
        //ie =
        hal_save_cli(); // Just cli!
    	hal_spin_lock( &timedcall_lock );

        // reattach? free?
        if( ep->flags & TIMEDCALL_FLAG_PERIODIC )
        {
            assert(ep->msecLater > 0);
            local_request_timed_call( ep, ep->flags );
        }
        /* TIMEDCALL_FLAG_AUTOFREE requires call to free from interrupt :(
        else if( ep->flags & TIMEDCALL_FLAG_AUTOFREE )
            free( ep );
        */

        if(!queue_empty(&tcEventQ))
        {
            ep = (timedcall_t *)queue_first(&tcEventQ);
            goto again;
        }
    }

unlock:
    hal_spin_unlock( &timedcall_lock );
    if(ie) hal_sti();

    dputc('_');
}



static int is_on_q(timedcall_t *entry)
{
    timedcall_t *next;

    queue_iterate( &tcEventQ, next, timedcall_t *, chain)
        if(next == entry)
            return 1;

    return 0;
}



void phantom_undo_timed_call(timedcall_t *entry)
{
    if(!inited) return;

    if( (entry->chain.next == 0) || (entry->chain.prev == 0) )
        return;

    int ie = hal_save_cli();
    hal_spin_lock( &timedcall_lock );
    //void *save_f = entry->f; // save
    //entry->f = 0; // make sure it won't fire in any case

    // TIMEDCALL_FLAG_AUTOFREE requires call to free from interrupt :(
    //assert( ! (entry->flags & TIMEDCALL_FLAG_AUTOFREE) );

    if( !is_on_q(entry) )
        goto unlock;

    long more = entry->msecMore;

    timedcall_t *tnext = (timedcall_t *)queue_next(&(entry->chain));

    queue_remove(&tcEventQ, entry, timedcall_t *, chain);

    if( !queue_end( &tcEventQ, (void *)tnext) )
        tnext->msecMore += more;

unlock:
    //entry->f = save_f; // put func back!
    hal_spin_unlock( &timedcall_lock );
    if(ie) hal_sti();
}






extern void wake_sleeping_thread( void *arg );


void dump_timed_call_queue(void)
{
    timedcall_t *entry;
    int index = 0;
    queue_iterate( &tcEventQ, entry, timedcall_t *, chain)
    {
        printf("%2d ", index++ );

        if( entry->f == wake_sleeping_thread )
            printf("WakeTh %3d ", (int)entry->arg );
        else
            printf(" @%8X ", entry );


        printf("time %5ld left %5ld, flags %b%s\n",
               entry->msecLater, entry->msecMore,
               entry->flags,
               "\20\1Periodic\2AutoFree\3CheckLock",
               entry->lockp ? (entry->lockp->lock ? ", Locked" : ", Unlocked") : ""
              );
    }
}


static void dump_timed_call_queue_cmd(int ac, char **av)
{
    (void) ac;
    (void) av;

    dump_timed_call_queue();
}

void phantom_timed_call_init2(void)
{
    dbg_add_command(&dump_timed_call_queue_cmd, "timedcalls", "List all of the timed call entries");
}

