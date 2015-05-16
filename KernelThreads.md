# Overview #

```
#include <threads.h>
#include <hal.h>
```

Phantom kernel is preemptively multithreaded. Threads sync API (see KernelSync) is more or less the same as in pthreads, but not exactly.

Current scheduler is very simple and supports 2 priority classes and exactly one idle priority level.

`THREAD_PRIO_IDLE` - select idle priority. Idle threads are run round robin in absence of other threads.


`THREAD_PRIO_LOWEST, THREAD_PRIO_LOW, THREAD_PRIO_NORM, THREAD_PRIO_HIGH, THREAD_PRIO_HIGHEST` - select normal priority. Run more or less RR when no realtime threads are ready to run.

`THREAD_PRIO_MOD_REALTIME|THREAD_PRIO_LOWEST...THREAD_PRIO_MOD_REALTIME|THREAD_PRIO_HIGHEST` - realtime priority. If any realtime priority thread exists, only realtime thread with maximum priority will run.

# API #

## Thread start and finish ##

`void hal_start_kernel_thread(void (*thread)(void));` - just starts thread. panics if thread returns.
`void hal_exit_kernel_thread(void);` - called by thread to finish well.

`int hal_start_kernel_thread_arg(void (*thread)(void *arg), void *arg);` - returns thread id, passes arg, does not check for thread return.


## Execution control ##

`void phantom_scheduler_yield( void );` - give CPU away. May return instantly.

`void hal_disable_preemption(void);` - make sure this thread won't be preempted. Not for a frequent use. Really. Not to be used as semaphore/mutex.

`void hal_enable_preemption(void);` - let thread to be preempted again.


`void hal_set_thread_priority( int tid, int prio );` - Set prio, see above.


`void hal_sleep_msec( int miliseconds );` - sleep a bit. Can actually sleep for a longer time, than requested!