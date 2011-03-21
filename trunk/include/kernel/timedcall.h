/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Timed calls. NB! Interrupt-time!
 *
 *
**/

#ifndef TIMEDCALL_H
#define TIMEDCALL_H

#include <queue.h>
#include <phantom_types.h>
#include <hal.h>

typedef void (*timedcall_func_t)(  void * );

typedef struct timedcall
{
    timedcall_func_t 	f;
    void                *arg;
    long                msecLater;

    // Event execution will be postponed while this spinlock is locked
    // TIMEDCALL_FLAG_CHECKLOCK flag must be set
    //hal_spinlock_t	lock;
    hal_spinlock_t	*lockp;

    // ----- following is callee's workarea, caller don't touch

    long                msecMore; // diff with earlier entry
    queue_chain_t       chain;
    u_int32_t           flags;

} timedcall_t;


#define TIMEDCALL_FLAG_PERIODIC         (1 << 0)

/* TIMEDCALL_FLAG_AUTOFREE requires call to free from interrupt :(
#define TIMEDCALL_FLAG_AUTOFREE         (1 << 1)
*/

#define TIMEDCALL_FLAG_CHECKLOCK        (1 << 2)

//#define TIMEDCALL_FLAG_THREADWAKE       (1 << 8)
//#define TIMEDCALL_FLAG_CONDSIGNAL       (1 << 9)
//#define TIMEDCALL_FLAG_CONDBROADC       (1 << 10)

void phantom_process_timed_calls(void);

void phantom_request_timed_call( timedcall_t *entry, u_int32_t flags );

/* TIMEDCALL_FLAG_AUTOFREE requires call to free from interrupt :(
void phantom_request_timed_func( timedcall_func_t f, void *arg, int msecLater, u_int32_t flags );
*/


//void phantom_request_timed_thread_wake( int tid, long timeMsec );
//void phantom_request_cond_signal( int msec, hal_cond_t *cond);
//void phantom_request_cond_broadcast( int msec, hal_cond_t *cond);

// Un-request previously requested timed call
void phantom_undo_timed_call(timedcall_t *entry);


#endif // TIMEDCALL_H

