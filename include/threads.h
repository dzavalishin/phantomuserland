#ifndef THREADS_H
#define THREADS_H

// TODO remove it from here when everything is moved in.
#include <hal.h>

#include <kernel/smp.h>


struct phantom_thread;
typedef struct phantom_thread phantom_thread_t;




#define THREAD_PRIO_NORM        0x7
#define THREAD_PRIO_LOWEST      0x1
#define THREAD_PRIO_LOW         0x2
#define THREAD_PRIO_HIGH        0xC
#define THREAD_PRIO_HIGHEST     0xF

#define THREAD_PRIO_IDLE        0x0

// Or (|) with prio to get realtime thread
#define THREAD_PRIO_MOD_REALTIME     0x10





#define THREAD_FLAG_USER        0x0001 // runs in user mode - not imlp
#define THREAD_FLAG_VM          0x0002 // runs virtual machine thread, owner points to phantom thread object??? GC??!
#define THREAD_FLAG_JIT         0x0004 // JITted VM tread - not imlp
#define THREAD_FLAG_NATIVE      0x0008 // runs native code objetct - not imlp
#define THREAD_FLAG_KERNEL      0x0010 // runs some kernel private stuff

#define THREAD_FLAG_PREFORK     0x0020 // it to be forked
#define THREAD_FLAG_PARENT      0x0040 // after fork - parent
#define THREAD_FLAG_CHILD       0x0080 // after fork - child

#define THREAD_FLAG_TIMEDOUT    0x0100 // Cond (or something else) was timed out
#define THREAD_FLAG_UNDEAD      0x0200 // This thread can't be killed for some reason. Usually it's some special one like CPU idle thread.
#define THREAD_FLAG_NOSCHEDULE  0x0400 // Must not be selected by scheduler in usual way - per CPU 'parking' (idlest) thread



#define CREATION_POSSIBLE_FLAGS (THREAD_FLAG_USER|THREAD_FLAG_VM|THREAD_FLAG_JIT|THREAD_FLAG_NATIVE|THREAD_FLAG_KERNEL)



extern phantom_thread_t *   percpu_current_thread[];

#define GET_CURRENT_THREAD() get_current_thread()

phantom_thread_t * get_current_thread(void);
phantom_thread_t * get_thread(int tid);


/**
 *
 * Called when:
 *   1. thread wants to give a good chance for others to run
 *   2. thread is blocked and can't run anymore
 *
**/

void phantom_scheduler_yield( void );

/**
 *
 * Set handler to be called in case of thread death.
 *
**/

void hal_set_thread_death_handler(void (*handler)( phantom_thread_t * ));


int phantom_vm_threads_get_count(void);

void hal_set_thread_name(const char *name);



errno_t t_kill_thread( int tid );









/*
struct hal_sem
{
    struct phantom_sem_impl     *impl;
};
*/


struct wtty *get_thread_ctty( struct phantom_thread *t );


/**
 *
 * Called from timer interrupt 100 times per sec.
 * Does not switch context. Just updates thread state
 * and possibly requests reshedule + soft irq.
 *
**/

void phantom_scheduler_time_interrupt(void);


#endif // THREADS_H

