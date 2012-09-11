/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Snap helpers.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#if NEW_SNAP_SYNC

#include <assert.h>
#include <stdarg.h>
#include <hal.h>
#include <phantom_libc.h>
#include <kernel/page.h>
#include <kernel/snap_sync.h>

#include <thread_private.h>

#include <phantom_libc.h>

static int temp_catch_var; // so that any code can touch catch pointer before we init all of this

static physaddr_t 	snap_catch_pa;
volatile int *		snap_catch_va = &temp_catch_var;


static hal_cond_t	threads_sleep_cond;
static hal_cond_t	snapper_sleep_cond;
static hal_mutex_t	threads_snapper_interlock;


static volatile int	all_threads_snap_lock; // nonzero = can't begin a snapshot
static volatile int	snapper_lock_flag;

static volatile int     interlock_inited = 0;

void t_assert_can_snap(void)
{
    assert(threads_inited);
    TA_LOCK();
    // Don't need it in release
#if !NDEBUG
    int i;
    for( i = 0; i < MAX_THREADS; i++ )
    {
        if( phantom_kernel_threads[i] == 0 )
            continue;

        phantom_thread_t *t = phantom_kernel_threads[i];

        assert( t->snap_lock == 0 );
    }
#endif
    assert(all_threads_snap_lock == 0);
    TA_UNLOCK();
}


// Called by thread to tell that snap is not possible
void snap_lock(void)
{
    assert(threads_inited);

    hal_mutex_lock( &threads_snapper_interlock );
    if( snapper_lock_flag )
        hal_cond_wait( &threads_sleep_cond, &threads_snapper_interlock );

    TA_LOCK();

    phantom_thread_t *t = get_current_thread();
    int tf = t->thread_flags;

    if(phantom_virtual_machine_stop_request && (tf & (THREAD_FLAG_VM|THREAD_FLAG_JIT)) )
    {
        TA_UNLOCK();
        hal_mutex_unlock( &threads_snapper_interlock );
        hal_exit_kernel_thread();
    }

    if(tf & THREAD_FLAG_SNAPPER)
    {
    }
    else
    {
        all_threads_snap_lock++;
        t->snap_lock++;
    }

    TA_UNLOCK();

    hal_mutex_unlock( &threads_snapper_interlock );

}


void snap_unlock(void)
{
    assert(threads_inited);
    TA_LOCK();

    phantom_thread_t *t = get_current_thread();
    int tf = t->thread_flags;

    if(tf & THREAD_FLAG_SNAPPER)
    {
    }
    else
    {
        t->snap_lock--;
        all_threads_snap_lock--;
    }

    TA_UNLOCK();

    hal_mutex_lock( &threads_snapper_interlock );
    if( 0 == all_threads_snap_lock )
        hal_cond_broadcast( &snapper_sleep_cond );
    hal_mutex_unlock( &threads_snapper_interlock );

    if(phantom_virtual_machine_stop_request && (tf & (THREAD_FLAG_VM|THREAD_FLAG_JIT)) )
        hal_exit_kernel_thread();
}

void t_release_snap_locks(void)
{
    // called on thread death to make sure snap is not locked forever

    int n;

    {
        TA_LOCK();
        phantom_thread_t *t = get_current_thread();
        n = t->snap_lock;
        TA_UNLOCK();
    }

    while(n--)
    {
        snap_unlock();
    }

    {
        TA_LOCK();
        phantom_thread_t *t = get_current_thread();
        assert( !t->snap_lock );
        TA_UNLOCK();
    }
}




// called by snapper to enter snap (pages -> ro) state
void snapper_lock(void)
{
    TA_LOCK();
    phantom_thread_t *t = get_current_thread();
    assert(t->thread_flags & THREAD_FLAG_SNAPPER);
    TA_UNLOCK();

    hal_mutex_lock( &threads_snapper_interlock );
    while( all_threads_snap_lock )
        hal_cond_wait( &snapper_sleep_cond, &threads_snapper_interlock );

    // set flag
    snapper_lock_flag++;

    
    // turn on catch page
    hal_page_control( snap_catch_pa, (void *)snap_catch_va, page_map, page_ro );

    hal_mutex_unlock( &threads_snapper_interlock );
}


// called by snapper to leave snap state (pages made ro, longer snap stage follows)
void snapper_unlock(void)
{
    // turn off catch page
    hal_page_control( snap_catch_pa, (void *)snap_catch_va, page_map, page_readwrite );

    // reset flag
    snapper_lock_flag--;

    hal_cond_broadcast( &threads_sleep_cond );
}


void snap_trap(void)
{
    // Called from pagefault - threads supposed to touch it
    // at points where snap is ok instead of calling snap_unlock()

    snap_unlock();
    snap_lock();
}


void phantom_thread_init_snapper_interlock(void)
{
    hal_cond_init( &snapper_sleep_cond, "SnSnap" );
    hal_cond_init( &threads_sleep_cond, "ThSnap" );
    hal_mutex_init( &threads_snapper_interlock, "SnapLock" );

    void * tmp;

    hal_pv_alloc( &snap_catch_pa, &tmp, PAGE_SIZE );
    hal_page_control( snap_catch_pa, tmp, page_map, page_readwrite );

    snap_catch_va = tmp; // activate

    interlock_inited = 1;
}

// ------------------------------------------------------------------
// Old func names
// ------------------------------------------------------------------

void phantom_thread_wait_4_snap( void )
{
    snap_unlock();
    snap_lock();
}

void phantom_snapper_wait_4_threads( void )
{
    snapper_lock();
}

void phantom_snapper_reenable_threads( void )
{
    snapper_unlock();
}


#endif // NEW_SNAP_SYNC

