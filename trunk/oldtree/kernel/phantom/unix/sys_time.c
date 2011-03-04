/**
 *
 * Phantom OS Unix Box
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Time syscalls
 *
 *
**/

#if HAVE_UNIX


#define DEBUG_MSG_PREFIX "Unix/net"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <limits.h>
#include <errno.h>
#include <time.h>

#include <unix/uuprocess.h>

#define MAX_MSEC (LONG_MAX-1)

#define MAX_SEC (MAX_MSEC/1000)



static long interruptible_sleep( int *interrupted, long msec )
{
    // TODO signal!
    hal_sleep_msec(msec);
    *interrupted = 0;
    return 0;
}


// TODO check ret val * errno against POSIX
int usys_nanosleep( int *err, uuprocess_t *u,  const struct timespec *req_time, struct timespec *rest_time )
{
    (void) u;

    long rest;
    // The problem is that this sleep is signal interruptible

    int interrupted = 0;

    struct timespec run_time = *req_time;

    while( run_time.seconds > 0 ) //&& run_time.nanoseconds > 1000 )
    {
        int sec = run_time.seconds;

        if( sec > MAX_SEC )
            sec = MAX_SEC;

        if(sec > 0 )
        {
            rest = interruptible_sleep( &interrupted, run_time.seconds * 1000 );
            run_time.seconds -= rest/1000;
            if(interrupted) goto leave;
        }
    }

    rest = interruptible_sleep( &interrupted, run_time.nanoseconds / 1000 );
    run_time.seconds -= rest*1000;
    if(interrupted) goto leave;

    // last msec without sig check

    if( run_time.nanoseconds > 0 )
        hal_sleep_msec(1);

leave:
    *rest_time = run_time;
    if(interrupted) *err = EINTR;
    return interrupted ? -1 : 0;
}

#endif // UNIX
