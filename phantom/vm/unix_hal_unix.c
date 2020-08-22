/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * HAL bindings for unix, compiled with Unix headers.
 *
**/


#include <stdio.h>
#include <stdlib.h>
//#include <errno.h> // travis Ci fail
#include <pthread.h>
#include <unistd.h>

#include "unixhal.h"


static pthread_mutex_t default_critical;

void unix_hal_init( void )
{
    //    InitializeCriticalSection(&default_critical);
    int rc = pthread_mutex_init( &default_critical, 0 );
    if( rc )
    {
        perror("unix_hal_init pthread_mutex_init");
        exit(33);
    }

    //int rc =
    //CreateThread( 0, 0, (void *) &winhal_debug_srv_thread, 0, 0, 0);
    //if( rc) printf("Win32 can't run debugger thread\n");

    unix_hal_start_thread( (void *)&winhal_debug_srv_thread, 0 );
}

void unix_hal_disable_preemption(void)
{
    //    EnterCriticalSection(&default_critical);
    int rc = pthread_mutex_lock(&default_critical);
    if( rc )
        perror("unix_hal_disable_preemption");
}

void unix_hal_enable_preemption(void)
{
//    LeaveCriticalSection(&default_critical);
    int rc = pthread_mutex_unlock(&default_critical);
    if( rc )
        perror("unix_hal_enable_preemption");
}



void unix_hal_sleep_msec( int miliseconds )
{
    usleep(1000L*miliseconds);
    //sleep( ((miliseconds-1)/1000)+1 );
}




unsigned long unix_hal_start_thread( void (*thread)(void *arg), void *arg )
{
    unsigned long tid;
    //if( 0 == CreateThread( 0, 0, (void *)thread, arg, 0, &tid ) )
    //    tid = 0;

    const pthread_attr_t *attr = 0;

    int rc = pthread_create((pthread_t *) &tid, attr, (void *)thread, arg );
    if( rc )                                           
    {
        perror("unix_hal_start_thread");
        return 0;
    }

    return tid;
}




// -----------------------------------------------------------------------
// TODO - implement mutex/sema code for unix sim environment


struct phantom_mutex_impl
{
    pthread_mutex_t m;
    const char *name;
    int lock;
};



void * unix_hal_mutex_init(const char *name)
{
    struct phantom_mutex_impl *impl;
    impl = calloc(1, sizeof(struct phantom_mutex_impl)+16); // to prevent corruption if kernel hal mutex func will be called
    if(impl == 0) return 0;

//    InitializeCriticalSection( &(impl->cs) );
    int rc = pthread_mutex_init( &(impl->m), 0 );
    if( rc )
    {
        perror("unix_hal_init pthread_mutex_init");
        exit(33);
    }

    impl->name = name;
    return impl;
}

int unix_hal_mutex_lock(void *_m)
{
    struct phantom_mutex_impl *impl = _m;
    impl->lock++;
//    EnterCriticalSection( &(impl->cs) );
    int rc = pthread_mutex_lock(&(impl->m));
    if( rc )
        perror("unix_hal_mutex_lock");
    return 0;
}

int unix_hal_mutex_unlock(void *_m)
{
    struct phantom_mutex_impl *impl = _m;
//    LeaveCriticalSection( &(impl->cs) );
    int rc = pthread_mutex_unlock(&(impl->m));
    if( rc )
        perror("unix_hal_mutex_unlock");
    impl->lock--;
    return 0;
}

int unix_hal_mutex_is_locked(void *_m)
{
    struct phantom_mutex_impl *impl = _m;
    return impl->lock;
}

/* to nonstandalone
//extern int errno;
//errno_t k_write( int *nwritten, int fd, const void *addr, int count )
int k_write( int *nwritten, int fd, const void *addr, int count )
{
	int rc = write( fd, addr, count );
	if( rc < 0 ) return errno;
	*nwritten = rc;
	return 0;
}
*/









