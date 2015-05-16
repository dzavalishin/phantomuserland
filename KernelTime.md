# Overview #

`#include <time.h>`

## Time in seconds ##

unix `time()` - just the same old true unix time(). not recommended, of course

## Time in microseconds ##

`bigtime_t hal_system_time(void);` - useconds from kernel boot

`bigtime_t hal_local_time(void);` - absolute time in useconds - **NB! Base?**


`void phantom_spinwait(int millis);` - wait some mseconds by spinning (not giving CPU away) - note that you have to disable interrupts (or, at least, preemption) to wait EXACTLY for given time.

## Timed calls (callouts) ##

```
void phantom_request_timed_call( timedcall_t *entry, u_int32_t flags );
void phantom_request_timed_func( timedcall_func_t f, void *arg, int msecLater, u_int32_t flags );
```

timedcall\_t is:

`timedcall_func_t 	f;       ` - func to call
`void                *arg;     ` - arg to pass to f
`long                msecLater;` - milliseconds to pass before call

flags are:

  * TIMEDCALL\_FLAG\_PERIODIC   - func will be called periodically
  * TIMEDCALL\_FLAG\_AUTOFREE   - entry will be freed automatically

**NB!** Call is done in timer intr! Be quick!

**NB!** It is possible for callout to happen BEFORE the return from this function.

`void phantom_request_cond_signal( int msec, hal_cond_t *cond);`
`void phantom_request_cond_broadcast( int msec, hal_cond_t *cond);`


**TODO**


`void phantom_request_timed_thread_wake( int tid, long timeMsec );`

Implement copy of these, but working on separate thread.

## Sleep ##

`void phantom_spinwait(int millis);` - spend some milliseconds consuming CPU