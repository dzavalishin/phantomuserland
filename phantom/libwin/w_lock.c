/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - windows mutex.
 *
**/

#include <video/window.h>
#include <video/internal.h>
#include <kernel/mutex.h>

#define ALLW_MUTEX 1

#if ALLW_MUTEX
static hal_mutex_t      allw_mutex;
#else
//static
hal_spinlock_t  	allw_lock = {};
#endif

//char wild_ptr_catch[2048] = {};


#if !ALLW_MUTEX
static int wie;
#endif

// use with care!
void w_lock(void)
{
#if ALLW_MUTEX
    hal_mutex_lock(&allw_mutex);
#else
    wie = hal_save_cli();
    hal_spin_lock( &allw_lock );
#endif
}

void w_unlock(void)
{
#if ALLW_MUTEX
    hal_mutex_unlock(&allw_mutex);
#else
    hal_spin_unlock( &allw_lock );
    if(wie) hal_sti();
#endif
}

void w_assert_lock(void)
{
#if ALLW_MUTEX
    ASSERT_LOCKED_MUTEX( &allw_mutex );
#else
    assert(hal_spin_locked( &allw_lock ));
#endif
}






void drv_video_init_windows(void)
{
#if ALLW_MUTEX
    hal_mutex_init( &allw_mutex, "allw" );
#else
    hal_spin_init( &allw_lock ));
#endif
    init_truetype();
    init_task_bar();
}

