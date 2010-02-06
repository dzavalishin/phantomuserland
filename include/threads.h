#ifndef THREADS_H
#define THREADS_H

// TODO remove it from here when everything is moved in.
#include <hal.h>






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

#define THREAD_FLAG_PREFORK     0x0020 // is to be forked
#define THREAD_FLAG_PARENT      0x0040 // after fork - parent
#define THREAD_FLAG_CHILD       0x0080 // after fork - child


#define CREATION_POSSIBLE_FLAGS (THREAD_FLAG_USER|THREAD_FLAG_VM|THREAD_FLAG_JIT|THREAD_FLAG_NATIVE|THREAD_FLAG_KERNEL)






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
** /

void hal_set_thread_death_handler(void (*handler)( phantom_thread_t * ));

*/

int phantom_vm_threads_get_count();

void hal_set_thread_name(const char *name);


#endif // THREADS_H

