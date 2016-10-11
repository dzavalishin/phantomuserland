/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * OS interface for Unix/Cygwin.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"
#include "cpfs_defs.h"

#include "cpfs_test.h"

#include <stdio.h>

#ifndef __POK_LIBC_STDIO_H__

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdarg.h>

#include <pthread.h>


// ----------------------------------------------------------------------------
//
// Debugging
//
// ----------------------------------------------------------------------------

#ifdef __CYGWIN__
    //#include <sys/cygwin.h>
//void cygwin_stackdump(void);
#endif


void
cpfs_panic( const char *fmt, ... )
{
    sleep(1); // let other thread to finish or die
    printf( "\n\nPanic: " );

    va_list ptr;
    va_start(ptr, fmt);
    vprintf(fmt, ptr);
    va_end(ptr);

    printf( "\nGlobal errno = %s\n\n", strerror(errno) );
#ifdef __CYGWIN__
    //cygwin_stackdump();
#endif
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
    return time(0);
}



// ----------------------------------------------------------------------------
//
// Mutex
//
// ----------------------------------------------------------------------------



#define USE_PTHREAD_MUTEX 1

#if !USE_PTHREAD_MUTEX
#  define MUTEX_TEST_VAL ((void *)0x3443ABBA)
#endif

void cpfs_mutex_lock( cpfs_mutex m)
{
/*
    if( TRACE ) trace(1, "%*s > cpfs_mutex_lock. mutex=%0x\n", TRACE, " ", m);  
*/
#if USE_PTHREAD_MUTEX
    pthread_mutex_t *pm = (void *)m;

    int rc = pthread_mutex_lock( pm );
    cpfs_assert( rc == 0 );
#else
    cpfs_assert( m == MUTEX_TEST_VAL );
#endif
}


void cpfs_mutex_unlock( cpfs_mutex m)
{
/*
    if( TRACE ) trace(-1, "%*s < cpfs_mutex_unlock. mutex=%0x\n", TRACE-TRACE_TAB, " ", m);   
*/
#if USE_PTHREAD_MUTEX
    pthread_mutex_t *pm = (void *)m;

    int rc = pthread_mutex_unlock( pm );
    cpfs_assert( rc == 0 );
#else
    cpfs_assert( m == MUTEX_TEST_VAL );
#endif
}

void cpfs_mutex_init( cpfs_mutex *m)
{

#if USE_PTHREAD_MUTEX
    pthread_mutex_t *pm = calloc( 1, sizeof( pthread_mutex_t ) );
    cpfs_assert( pm != 0 );
    //int rc = pthread_mutex_init( pm, 0 );
    //cpfs_assert( rc == 0 );

    pthread_mutex_t tmp = PTHREAD_MUTEX_INITIALIZER;

    *pm = tmp;
    *m = (void *)pm;
#else
    *m = MUTEX_TEST_VAL;
#endif
}



void
cpfs_mutex_stop( cpfs_mutex m )
{
/*
if( TRACE ) trace(0, "cpfs_mutex_stop %0x\n",  m); 
*/

#if USE_PTHREAD_MUTEX
    pthread_mutex_t *pm = (void *)m;

    int rc = pthread_mutex_destroy ( pm );
    cpfs_assert( rc == 0 );
#else
    cpfs_assert( m == MUTEX_TEST_VAL );
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
    int rc;

    pthread_attr_t a;
    pthread_t pt;

    pthread_attr_init( &a );

    // TODO idle prio!

    rc = pthread_create( &pt, &a, func_p, arg);

    pthread_attr_destroy( &a );

    return (rc == 0) ? 0 : ENOMEM;
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




#endif // POK/JetOS

