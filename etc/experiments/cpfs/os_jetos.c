/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * OS interface for JetOS.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"
#include "cpfs_defs.h"

#include "cpfs_test.h"

#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>

//#include <stdarg.h>

//#include <pthread.h>

#warning need real headers
struct msection;


// ----------------------------------------------------------------------------
//
// Debugging
//
// ----------------------------------------------------------------------------



void
cpfs_panic( const char *fmt, ... )
{
//    sleep(1); // let other thread to finish or die
    printf( "\n\nPanic: " );

    va_list ptr;
    va_start(ptr, fmt);
    vprintf(fmt, ptr);
    va_end(ptr);

    printf( "\nGlobal errno = %d\n\n", errno );
#warning implement me

    exit(33);
}


void
cpfs_log_error( const char *fmt, ... )
{
    printf( "Error: ");

    va_list ptr;
    va_start(ptr, fmt);
    vprintf(fmt, ptr);
    va_end(ptr);
#warning implement me

    printf( "\n");
}







// ----------------------------------------------------------------------------
//
// Time/date
//
// ----------------------------------------------------------------------------


cpfs_time_t
cpfs_get_current_time(void)
{
#warning implement me
    return 0;
}



// ----------------------------------------------------------------------------
//
// Mutex
//
// ----------------------------------------------------------------------------



#define USE_PTHREAD_MUTEX 0


void cpfs_mutex_lock( cpfs_mutex m)
{
#if USE_PTHREAD_MUTEX
    pthread_mutex_t *pm = (void *)m;

    int rc = pthread_mutex_lock( pm );
    cpfs_assert( rc == 0 );
#else

#warning implement me
//    struct msection *ms = (void *)m;
//    msection_enter(ms); // TODO check rc
#endif
}


void cpfs_mutex_unlock( cpfs_mutex m)
{
#if USE_PTHREAD_MUTEX
    pthread_mutex_t *pm = (void *)m;

    int rc = pthread_mutex_unlock( pm );
    cpfs_assert( rc == 0 );
#else
#warning implement me
//    struct msection *ms = (void *)m;
//    msection_leave(ms); // TODO check rc
#endif
}

void cpfs_mutex_init( cpfs_mutex *m)
{

#if USE_PTHREAD_MUTEX
    pthread_mutex_t *pm = calloc( 1, sizeof( pthread_mutex_t ) );
    cpfs_assert( pm != 0 );

    pthread_mutex_t tmp = PTHREAD_MUTEX_INITIALIZER;

    *pm = tmp;
    *m = (void *)pm;
#else
/*
#warning implement me
    struct msection *ms = calloc( 1, sizeof( struct msection ) );
    cpfs_assert( ms != 0 );
    msection_init( ms ); // TODO check rc
    *m = (void *)ms;
*/
#endif
}



void
cpfs_mutex_stop( cpfs_mutex m )
{

#if USE_PTHREAD_MUTEX
    pthread_mutex_t *pm = (void *)m;

    int rc = pthread_mutex_destroy ( pm );
    cpfs_assert( rc == 0 );
#else

#warning implement me

#endif
}



// ----------------------------------------------------------------------------
//
// Threads
//
// ----------------------------------------------------------------------------




errno_t
cpfs_os_run_idle_thread( void* (*func_p)(void *arg), void *arg ) // Request OS to start thread
{
/*
    int rc;

    pthread_attr_t a;
    pthread_t pt;

    pthread_attr_init( &a );

    // TODO idle prio!

#warning check me


    rc = pthread_create( &pt, &a, func_p, arg);

    pthread_attr_destroy( &a );

    return (rc == 0) ? 0 : ENOMEM;
*/
    return ENOMEM;
}


// ----------------------------------------------------------------------------
//
// Rigths check stub
//
// ----------------------------------------------------------------------------



errno_t
cpfs_os_access_rights_check( struct cpfs_fs *fs, cpfs_right_t t, void *user_id_data, const char *fname )
{
    (void) fs;
    (void) t;
    (void) user_id_data;
    (void) fname;

    return 0; // can do anything
}



void exit(int rc) { STOP_SELF(); }


char *
strchr
(const char *p, int ch)
{
    char c;

    c = ch;
    for (;; ++p) {
	if (*p == c)
	    return ((char *)p);
	if (*p == '\0')
	    return 0;
    }
    /* NOTREACHED */
}


