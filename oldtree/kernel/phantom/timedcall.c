/**
 *
 *
 * 
 *
**/

#define DEBUG_MSG_PREFIX "timer"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10


#include "timedcall.h"
#include "misc.h"
#include <spinlock.h>
#include <assert.h>
#include <phantom_libc.h>

#define TEST 0
//#define DEBUG 0


static hal_spinlock_t 	timedcall_lock;
static queue_head_t    	head;
static int              inited = 0;


#if TEST
static void phantom_timed_call_test();
#endif


void phantom_timed_call_init(void)
{
    hal_spin_init( &timedcall_lock );
    queue_init( &head );

    inited = 1;

#if TEST
    phantom_timed_call_test();
#endif
}

static void do_request_timed_call( timedcall_t *entry, u_int32_t flags )
{

    // Still call it later - for any case :)
    if( entry->msecLater < 0 ) entry->msecLater = 0;
    entry->msecMore = 0;
    entry->flags = flags;

    if(
       (entry->flags & TIMEDCALL_FLAG_PERIODIC) &&
       (entry->flags & TIMEDCALL_FLAG_AUTOFREE)
      )
        panic("periodic autofree timed call requested");

    //if( entry.next != 0)        panic("next != null in phantom_request_timed_call");

    if(queue_empty(&head))
    {
        SHOW_FLOW0( 2, "put: empty");
        entry->msecMore = entry->msecLater;
        queue_enter_first( &head, entry, timedcall_t *, chain);
        goto unlock;
    }

    {
        timedcall_t *first = (timedcall_t *)queue_first(&head);
        if(entry->msecLater < first->msecMore)
        {
            SHOW_FLOW0( 2, "put: first");
            entry->msecMore = entry->msecLater;
            first->msecMore -= entry->msecMore;
            queue_enter_first( &head, entry, timedcall_t *, chain);
            goto unlock;
        }
    }

    timedcall_t *ep;
    long msecSumm = 0;
    queue_iterate( &head, ep, timedcall_t *, chain)
    {
        if( entry->msecLater >= msecSumm && entry->msecLater < msecSumm + ep->msecMore )
        {
            entry->msecMore = entry->msecLater - msecSumm;
            ep->msecMore -= entry->msecMore;

            SHOW_FLOW( 2, "put: before %ld", ep->msecLater );
            queue_enter_before( &head, ep, entry, timedcall_t *, chain);
            goto unlock;
        }

        msecSumm += ep->msecMore;
    }

    entry->msecMore = entry->msecLater - msecSumm;
    queue_enter( &head, entry, timedcall_t *, chain);
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
static void wakethread( void *arg )
{
    //phantom_thread_t *t = get_thread( (int)arg ); // arg is tid
    //t_unblock(t, ?? );
}

// TODO impl
void phantom_request_timed_thread_wake( int tid, long timeMsec )
{
    panic("not impl");

    phantom_request_timed_func( wakethread, (void *)tid, timeMsec, 0 );
}
*/

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






void phantom_request_timed_func( timedcall_func_t f, void *arg, int msecLater, u_int32_t flags )
{
    timedcall_t *req = calloc(1, sizeof(timedcall_t) );
    req->f = f;
    req->arg = arg;
    req->msecLater = msecLater;

    phantom_request_timed_call( req, flags|TIMEDCALL_FLAG_AUTOFREE );
}











// Called once in msec
void phantom_process_timed_calls()
{
    if(!inited) return;

    hal_spin_lock( &timedcall_lock );
#if debug_level_flow > 2
    putchar('.');
#endif


    if(queue_empty(&head))
    {
#if debug_level_flow > 2
        putchar('-');
#endif
        goto unlock;
    }
#if debug_level_flow > 2
    putchar('%');
#endif

    timedcall_t *ep = (timedcall_t *)queue_first(&head);

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
#if debug_level_flow > 2
    putchar('!');
#endif
        // detach
        queue_remove(&head, ep, timedcall_t *, chain);

        if( (ep->msecMore < 0) && !queue_empty(&head) )
        {
            // It can happen if execution was deferred due to the lock
            timedcall_t *next_ep = (timedcall_t *)queue_first(&head);
            // ep->msecMore is negative. Propagate extra time to next
            // event
            next_ep->msecMore += ep->msecMore;
        }

		// It is not supposed that callee will sleep, but for any case
		// unlock the spinlock
    	hal_spin_unlock( &timedcall_lock );
	    // call
        ep->f(ep->arg);
    	hal_spin_lock( &timedcall_lock );

        // reattach? free?
        if( ep->flags & TIMEDCALL_FLAG_PERIODIC )
            local_request_timed_call( ep, ep->flags );
        else if( ep->flags & TIMEDCALL_FLAG_AUTOFREE )
            free( ep );


        if(!queue_empty(&head))
        {
            ep = (timedcall_t *)queue_first(&head);
            goto again;
        }
    }

unlock:
    hal_spin_unlock( &timedcall_lock );
}


#if TEST


static void echo(  void *_a )
{
    printf("Echo: '%s'\n", _a);
}

timedcall_t     t1 = { echo, "hello 1", 1 };
timedcall_t     t2 = { echo, "hello 100", 100 };
timedcall_t     t3 = { echo, "hello 2000", 2000 };
timedcall_t     t4 = { echo, "hello 10 000", 10000 };

static char *msg = "timed func";

static void phantom_timed_call_test()
{
    //printf("Testing timed calls, reset time:\n");
    //getchar();
    phantom_request_timed_call( &t1, 0 );
    phantom_request_timed_call( &t2, 0 );
    phantom_request_timed_call( &t3, 0 );
    phantom_request_timed_call( &t4, 0 );

    phantom_request_timed_func( echo, msg, 5000, 0 );


    printf("Testing timed calls, wait for echoes:\n");
    getchar();
}


#endif // TEST


void dump_timed_call_queue()
{
    timedcall_t *entry;
    int index = 0;
    queue_iterate( &head, entry, timedcall_t *, chain)
    {
        printf("%2d @%8X time %5ld left %5ld, flags %b%s\n",
               index++, entry, entry->msecLater, entry->msecMore,
               entry->flags,
               "\20\0Periodic\1AutoFree\2CheckLock",
               entry->lockp->lock ? ", locked" : ", unl"
              );
    }
}


