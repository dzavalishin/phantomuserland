/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * This source file implements Windows based wrapper for VM to
 * run in Windows-hosted environment. 
 *
**/

#include <windows.h>



static CRITICAL_SECTION default_critical;

void win_hal_init( void )
{
    InitializeCriticalSection(&default_critical);

    //int rc =
    //CreateThread( 0, 0, (void *) &winhal_debug_srv_thread, 0, 0, 0);
    //if( rc) printf("Win32 can't run debugger thread\n");
}

void win_hal_disable_preemption(void)
{
    EnterCriticalSection(&default_critical);
}

void win_hal_enable_preemption(void)
{
    LeaveCriticalSection(&default_critical);
}



void win_hal_sleep_msec( int miliseconds )
{
    Sleep(miliseconds);
}




unsigned long win_hal_start_thread( void (*thread)(void *arg), void *arg )
{
    unsigned long tid;
    if( 0 == CreateThread( 0, 0, (void *)thread, arg, 0, &tid ) )
        tid = 0;

    return tid;
}




// -----------------------------------------------------------------------
// TODO - implement mutex/sema code for win sim environment


struct phantom_mutex_impl
{
    CRITICAL_SECTION cs;
    const char *name;
    int lock;
};



void * win_hal_mutex_init(const char *name)
{
    struct phantom_mutex_impl *impl;
    impl = calloc(1, sizeof(struct phantom_mutex_impl)+16); // to prevent corruption if kernel hal mutex func will be called
    if(impl == 0) return 0;

    InitializeCriticalSection( &(impl->cs) );

    impl->name = name;
    return impl;
}

int win_hal_mutex_lock(void *_m)
{
    struct phantom_mutex_impl *impl = _m;
    impl->lock++;
    EnterCriticalSection( &(impl->cs) );
    return 0;
}

int win_hal_mutex_unlock(void *_m)
{
    struct phantom_mutex_impl *impl = _m;
    LeaveCriticalSection( &(impl->cs) );
    impl->lock--;
    return 0;
}

int win_hal_mutex_is_locked(void *_m)
{
    struct phantom_mutex_impl *impl = _m;
    return impl->lock;
}














