/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel threads. Move to kernel/.
 *
**/

#ifndef THREADS_H
#define THREADS_H

#include <phantom_types.h>
#include <errno.h>
#include <sys/cdefs.h>


/**
 * \ingroup Threads
 * \defgroup Threads Threads and syncronization
 * @{
**/

#define NO_PID 0

// -----------------------------------------------------------------------
// Types
// -----------------------------------------------------------------------

struct phantom_thread;
typedef struct phantom_thread phantom_thread_t;

// -----------------------------------------------------------------------
// Good clean tid based threads interface
// -----------------------------------------------------------------------


tid_t           get_current_tid(void);
errno_t         t_kill_thread( tid_t tid );

errno_t         t_set_owner( tid_t tid, void *owner );
errno_t         t_get_owner( tid_t tid, void **owner );

struct wtty;
#if CONF_NEW_CTTY
errno_t		t_new_ctty( tid_t tid );
#else
errno_t         t_set_ctty( tid_t tid, struct wtty * );
#endif
errno_t         t_get_ctty( tid_t tid, struct wtty ** );

errno_t         t_set_pid( tid_t tid, pid_t pid );
errno_t         t_get_pid( tid_t tid, pid_t *pid );

errno_t         t_set_priority( tid_t tid, int prio );
errno_t         t_get_priority( tid_t tid, int *prio );

errno_t         t_add_flags( tid_t tid, u_int32_t set_flags );
errno_t         t_remove_flags( tid_t tid, u_int32_t reset_flags );
errno_t         t_get_flags( tid_t tid, u_int32_t *flags );



// -----------------------------------------------------------------------
// Current thread only
// -----------------------------------------------------------------------

errno_t         t_current_set_priority( int prio );
errno_t         t_current_get_priority( int *prio);

errno_t         t_current_set_name(const char *name);


errno_t         t_current_set_death_handler(void (*handler)( phantom_thread_t * ));

// -----------------------------------------------------------------------
// Misc
// -----------------------------------------------------------------------

// Switch on/off CPUs other than boot
void            t_smp_enable(int yn);


// Make sure we return executing on boot CPU
void            t_migrate_to_boot_CPU(void);

errno_t         t_set_snapper_flag(void); // mark myself as snapper thread


// -----------------------------------------------------------------------
// Priorities
// -----------------------------------------------------------------------


#define THREAD_PRIO_NORM        0x7
#define THREAD_PRIO_LOWEST      0x1
#define THREAD_PRIO_LOW         0x2
#define THREAD_PRIO_HIGH        0xC
#define THREAD_PRIO_HIGHEST     0xF

#define THREAD_PRIO_IDLE        0x0

// Or (|) with prio to get realtime thread
#define THREAD_PRIO_MOD_REALTIME     0x10




// -----------------------------------------------------------------------
// Flags
// -----------------------------------------------------------------------


#define THREAD_FLAG_USER        0x0001 // runs in user mode - not imlp
#define THREAD_FLAG_VM          0x0002 // runs virtual machine thread, owner points to phantom thread object?? GC?!
#define THREAD_FLAG_JIT         0x0004 // JITted VM tread - not imlp
#define THREAD_FLAG_NATIVE      0x0008 // runs VM native code object - not imlp
#define THREAD_FLAG_KERNEL      0x0010 // runs some kernel private stuff

#define THREAD_FLAG_PREFORK     0x0020 // it to be forked
#define THREAD_FLAG_PARENT      0x0040 // after fork - parent
#define THREAD_FLAG_CHILD       0x0080 // after fork - child

#define THREAD_FLAG_TIMEDOUT    0x0100 // Cond (or something else) was timed out
#define THREAD_FLAG_UNDEAD      0x0200 // This thread can't be killed for some reason. Usually it's some special one like CPU idle thread.
#define THREAD_FLAG_NOSCHEDULE  0x0400 // Must not be selected by scheduler in usual way - per CPU 'parking' (idlest) thread

#define THREAD_FLAG_SNAPPER     0x1000 // I am is a snapper thread
//#define THREAD_FLAG_SNAP_OK     0x2000 // I am paused so snapper can set pages writeonly

#define THREAD_FLAG_HAS_PORT   0x10000 // This thread (possibly) owns port (see newos/ports)

// Can be passed to thread creation function
#define CREATION_POSSIBLE_FLAGS (THREAD_FLAG_USER|THREAD_FLAG_VM|THREAD_FLAG_JIT|THREAD_FLAG_NATIVE|THREAD_FLAG_KERNEL)

// Can be set with t_add_flags/t_remove_flags
#define SET_POSSIBLE_FLAGS (THREAD_FLAG_HAS_PORT)


















// -----------------------------------------------------------------------
// Outdated
// -----------------------------------------------------------------------


// returns thread id, does not check for thread death
tid_t           hal_start_kernel_thread_arg(void (*thread)(void *arg), void *arg);
tid_t           hal_start_thread(void (*thread)(void *arg), void *arg, int flags);


//errno_t         hal_set_current_thread_priority( int prio );
//int             hal_get_current_thread_priority();

#define hal_set_current_thread_name hal_set_thread_name
void            hal_set_thread_name(const char *name);


void            hal_disable_preemption(void);
void            hal_enable_preemption(void);





// TODO remove it from here when everything is moved in.
#include <hal.h>

#include <kernel/smp.h>














extern phantom_thread_t *   percpu_current_thread[];

#define GET_CURRENT_THREAD() get_current_thread()

phantom_thread_t * get_current_thread(void) __attribute__((no_instrument_function));
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














struct wtty *get_thread_ctty( struct phantom_thread *t )  __attribute__((deprecated));


//! Returns 'owner' field, which is, for VM threads, is used to keep pointer to VM thread object
//void *get_thread_owner( struct phantom_thread *t )  __attribute__((deprecated));

//int get_thread_flags( struct phantom_thread *t ) __attribute__((deprecated));

/**
 *
 * Called from timer interrupt 100 times per sec.
 * Does not switch context. Just updates thread state
 * and possibly requests reshedule + soft irq.
 *
**/

void phantom_scheduler_time_interrupt(void);


/**
 *
 * Called from interrupt finalizer (when interrupt state
 * is actually exited, but iret not executed.
 * Used to switch context, if needed.
 *
**/

void phantom_scheduler_soft_interrupt(void);




//void*   hal_start_kernel_thread(void (*thread)(void));
void    hal_start_kernel_thread(void (*thread)(void));
void    hal_exit_kernel_thread(void) __dead2;



//! Internal kernel debug print only
void phantom_dump_threads_buf(char *buf, int len);



#endif // THREADS_H

