/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Local header.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#ifndef THREAD_PRIVATE_H
#define THREAD_PRIVATE_H

#include <kernel/timedcall.h>
#include <kernel/trap.h>
#include <kernel/smp.h>

#include <cpu_state.h>

#include <phantom_types.h>
#include <queue.h>
#include <hal.h>
#include <wtty.h>
#include <threads.h>

/**
 * \ingroup Threads
 * \defgroup Threads Threads and syncronization
 * @{
**/


extern int threads_inited;

// No. Killed.
#define USE_FORK_LUKE 0

#define LATENCY_DEBUG 0

#define MAX_THREADS 1024

// Thread struct/array access lock - to prevent accessing killed thread
// TODO - take in kill!
#define TA_LOCK() int ___sie = hal_save_cli(); hal_spin_lock( &schedlock )
#define TA_UNLOCK() hal_spin_unlock( &schedlock ); if(___sie) hal_sti()

#define MAX_THREAD_NAME_LEN 32

struct uuprocess;

struct phantom_thread
{
    /** NB! Exactly first! Accessed from asm. */
    cpu_state_save_t            cpu;

    //! on which CPU this thread is dispatched now
    int                         cpu_id; 

    int                         tid;

    //! phantom thread ref, etc
    void *                      owner;

    //! "controlling" tty
    wtty_t *                    ctty; 

    //! if this thread runs Unix simulation process - here is it
    struct uuprocess *          u;

    const char *                name;
    //const char                  name[MAX_THREAD_NAME_LEN+1];

    void *                      stack;
    physaddr_t                  stack_pa;
    size_t                      stack_size;

    // This is kernel/interrupt stack.
    // We patch it into the TSS on thread switch so that if 386 will
    // for some reason switch from ring 1-3 to ring 0, this stack is used.
    // It seems to be not needed for ring 0 only threads, but lets have it
    // this way now.
    void *                      kstack;
    physaddr_t                  kstack_pa;
    size_t                      kstack_size;
    void *                      kstack_top; // What to load to ESP

    void *                      start_func_arg;
    void                        (*start_func)(void *);

    u_int32_t                   thread_flags; // THREAD_FLAG_xxx

    /** if this field is zero, thread is ok to run. */
    u_int32_t                   sleep_flags; //THREAD_SLEEP_xxx

    /**
     * If we sleep on cond, here is it.
     * NB! sleep_flags & THREAD_SLEEP_COND is set and reset out of lock.
     * If you need atomic check, check waitcond under waitlock!
     */
    hal_cond_t *                waitcond;
    hal_mutex_t *               waitmutex;
    hal_mutex_t *               ownmutex;
    hal_sem_t *                 waitsem;

    hal_spinlock_t              waitlock;

    queue_chain_t               chain; // used by mutex/cond code to chain waiting threads
    queue_chain_t               kill_chain; // used kill code to chain threads to kill

    //* Used to wake with timer, see hal_sleep_msec
    timedcall_t                 sleep_event; 


    int                         preemption_disabled;

    int                         child_tid; // used by fork code


    //! void (*handler)( phantom_thread_t * )
    void *                      death_handler; // func to call if thread is killed

    //! Func to call on trap (a la unix signals), returns nonzero if can't handle
    int 			(*trap_handler)( int sig_no, struct trap_state *ts );
    // --------------------------------------------------------------
    // Scheduler part
    // --------------------------------------------------------------

    /** THREAD_PRIO_xxx */
    u_int32_t                   priority;

    /**
     *
     * How many (100HZ) ticks this thread can be on CPU before resched.
     * NB! Signed, so that underrun is not a problem.
     *
    **/
    int32_t                   	ticks_left;

    /** Used by runq only. Is not 0 if on runq. */
    queue_chain_t		runq_chain;

    /** Will be unlocked just after this thread is switched off CPU */
    hal_spinlock_t              *sw_unlock;

#if LATENCY_DEBUG
    bigtime_t                   sleep_start;
    bigtime_t                   max_sleep;
#endif
};




#define THREAD_SLEEP_USER       0x0001 // just paused by user's call - must be unpaused manually
#define THREAD_SLEEP_SLEEP      0x0002 // waits for time to pass
#define THREAD_SLEEP_COND       0x0004 // waits for cond to be signalled
#define THREAD_SLEEP_MUTEX      0x0008 // waits for mutex to be freed
#define THREAD_SLEEP_SEM        0x0010 // waits for sema
#define THREAD_SLEEP_LOCKED     0x0020 // thread state is incomplete (on creation)
#define THREAD_SLEEP_IO     	0x0040 // Waits for synchronous IO to complete

#define THREAD_SLEEP_ZOMBIE    	0x0100 // Waits for synchronous IO to complete



//typedef struct phantom_thread phantom_thread_t;


/**
 *
 * Init Phantom threads engine.
 *
**/
void phantom_threads_init(void);


//* Scan through threads
tid_t get_next_tid(tid_t tid, phantom_thread_t *out);



/**
 *
 * Create thread structure for main thread - the one which is calling
 * this function and the one which initializes kernel.
 *
**/
void phantom_import_main_thread(void);



void phantom_thread_state_init(phantom_thread_t *t);


/**
 *
 * Will save context to from, load from to and atomically
 * clear unlock.
 */
void phantom_switch_context(
                            phantom_thread_t *from,
                            phantom_thread_t *to,
                            hal_spinlock_t *unlock );





#define SET_CURRENT_THREAD(t) ({ percpu_current_thread[GET_CPU_ID()] = (t); (void)0;})
 


extern phantom_thread_t *   percpu_idlest_thread[];

#define GET_IDLEST_THREAD() percpu_idlest_thread[GET_CPU_ID()]





extern phantom_thread_t *phantom_kernel_threads[];



extern hal_spinlock_t schedlock;

// lock for thread array manipulation, taken to be
// sure thread is not killed till we do smthing.
// USE shedlock!
//extern hal_spinlock_t tidlock;




/**
 *
 * Adds flag to sleep flags of thread, removes thread from
 * runnable set, unlocks the lock given, switches off to some other thread.
 */
void thread_block( int sleep_flag, hal_spinlock_t *lock_to_be_unlocked );
/**
 *
 * Checks if thread really has this sleep flag, removes it, unblocks
 * thread if no sleep flags left.
 *
 */
void thread_unblock( phantom_thread_t *t, int sleep_flag );



/**
 *
 * Extract thread with a highest priority from queue.
 * Current dumb impl returns just a first thread.
 *
 * Threads must be chaied by chain field.
 *
 */
phantom_thread_t * t_dequeue_highest_prio(queue_head_t *queue);

/**
 *
 * Test that queue does not contain given thread.
 *
**/
void t_queue_check(queue_head_t *queue, phantom_thread_t *test);

// Just remove from and put to runq
void t_dequeue_runq(phantom_thread_t *);
void t_enqueue_runq(phantom_thread_t *);


void hal_disable_preemption(void);
void hal_enable_preemption(void);
int hal_is_preemption_disabled(void);


void phantom_thread_init_conds(void);
void phantom_thread_init_mutexes(void);
void phantom_thread_init_sems(void);

void phantom_thread_init_killer(void);

//! Used in thread kill
void hal_mutex_unlock_if_owner(void);


// ----------------------------------------------------------------
// Killer
// ----------------------------------------------------------------


extern int     t_thread_kill_request;
void t_do_some_kills(void);



// ----------------------------------------------------------------
// Scheduler
// ----------------------------------------------------------------


void phantom_scheduler_init(void);

/**
 *
 * Ask scheduler to take note that something is changed and it is
 * possible that other thread is ready to run now. Returns immediately.
 * Actual reschedule happens on next (usually - timer) interrupt.
 *
**/

void phantom_scheduler_request_reschedule( /* reason? */ void );


/**
 *
 * Ask scheduler to chech if it is time to remove current thread
 * from CPU for some reason.
 *
**/

int phantom_scheduler_is_reschedule_needed(void);



/**
 *
 * Ask scheduler to select thread to be run next.
 *
**/

phantom_thread_t *phantom_scheduler_select_thread_to_run(void);


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
 * Same, but has to unlock given spinlock after thread switch.
 *
**/
void phantom_scheduler_yield_locked(hal_spinlock_t *lock_to_be_unlocked);






void phantom_scheduler_schedule_soft_irq(void); // do not enforce, just make it come in some near future


// TODO in fact, possibly, it is ok just to call phantom_scheduler_soft_interrupt() from it?
void phantom_scheduler_request_soft_irq(void); // enforce soft irq

/**
 *
 * Switch to other thread. Calls scheduler to
 * find a thread to switch. Can return without
 * switching.
 *
**/

void phantom_thread_switch(void);

// Do what is required (arch specific) after switching to a new thread
void arch_adjust_after_thread_switch(phantom_thread_t *t);





void i386_fxsave(void *fpu_state);
void i386_fxrstor(void *fpu_state);


// Internal one. See hal_start_kernel_thread_arg(void (*thread)(void *arg), void *arg)
phantom_thread_t *phantom_create_thread( void (*func)(void *), void *arg, int flags );



//void phantom_thread_in_interrupt_fork(void);

void dump_thread_stack(phantom_thread_t *t);
void dump_thread_stacks(void);


// ToDO move to more adequate header

//void hal_set_thread_death_handler(void (*handler)( phantom_thread_t * ));
//void hal_set_thread_name(const char *name);


// --------------------------------------------------------------
// Cond/mutex impl
// --------------------------------------------------------------

struct phantom_cond_impl
{
    hal_spinlock_t      lock;
    const char*         name;
    queue_head_t	waiting_threads;
};

struct phantom_mutex_impl
{
    hal_spinlock_t              lock;
    const char*         	name;
    phantom_thread_t *          owner;
    queue_head_t		waiting_threads;
    int                         readers; // N of readers in lock
};


struct phantom_sem_impl
{
    hal_spinlock_t      lock;
    int                 value;
    const char*         name;
    queue_head_t	waiting_threads;
};


/** @} */

#endif // THREAD_PRIVATE_H

