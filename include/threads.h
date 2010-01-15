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


#endif // THREADS_H

