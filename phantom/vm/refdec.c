/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Deferred refcount decrement - done this way to fight races:
 *  - one thread reads object slot
 *  - another thread overwrites that slot, decrements refcnt and kills obj
 *  - first thread attempts to ref inc and corrupts memory
 *
 * Here we make sure that we decrement refcount only after making
 * sure that all threads pass bytecode instruction boundary.
 *
**/


#include <kernel/init.h>
#include <kernel/mutex.h>
#include <kernel/stats.h>
#include <kernel/atomic.h>
#include <kernel/snap_sync.h>

#include <threads.h>

#include <vm/alloc.h>
#include <vm/object.h>

static int inited = 0;
static volatile int stop_refdec_thread = 0;


static void deferred_refdec_init(void);
static void deferred_refdec_thread(void *a);


INIT_ME( 0, deferred_refdec_init, 0 )
//STOP_ME( deferred_refdec_stop )


#define REFDEC_BUFFER_SIZE (1024*16)

    // Where to start agressive action
#define REFDEC_BUFFER_RED_ZONE (REFDEC_BUFFER_SIZE/4)

#define REFDEC_BUFFER_HALF (REFDEC_BUFFER_SIZE/2)



static hal_mutex_t deferred_refdec_mutex;
static hal_cond_t  start_refdec_cond, end_refdec_cond;
static tid_t deferred_refdec_thread_id;


static void deferred_refdec_init(void)
{
    hal_mutex_init( &deferred_refdec_mutex, "refdec");

    hal_cond_init(  &start_refdec_cond, "refdec st" );
    hal_cond_init(  &end_refdec_cond, "refdec end" );

    deferred_refdec_thread_id = hal_start_thread( deferred_refdec_thread, 0, 0 );
    assert(deferred_refdec_thread_id > 0 );

    inited = 1;
}

/*
static void deferred_refdec_stop(void) //__attribute__((unused))
{
    stop_refdec_thread = 1;
    hal_cond_signal( &start_refdec_cond );
}
*/



// 2 pages
static volatile pvm_object_storage_t *refdec_buffer[REFDEC_BUFFER_SIZE];
static volatile int refdec_put_ptr = 0;
//static volatile int npage = 0;


void deferred_refdec(pvm_object_storage_t *os)
{
    assert(inited);
#if 0
    if( os->_ah.refCount > 1 )
    {
        os->_ah.refCount--;
        if( os->_ah.refCount <= 0 )
        {
            os->_ah.refCount++;
            goto long_way;
        }
        return;
    }
#endif

    STAT_INC_CNT(DEFERRED_REFDEC_REQS);

    if(
       ( (refdec_put_ptr <= REFDEC_BUFFER_HALF) && (refdec_put_ptr > REFDEC_BUFFER_RED_ZONE ) )
       ||
       ( refdec_put_ptr > REFDEC_BUFFER_RED_ZONE+REFDEC_BUFFER_HALF )
      )
    {
        //hal_mutex_lock(  &deferred_refdec_mutex );

        hal_cond_signal( &start_refdec_cond );
        //hal_cond_wait(   &end_refdec_cond, &deferred_refdec_mutex );

        //hal_mutex_unlock( &deferred_refdec_mutex );
    }


    //long_way:
    // TODO ERROR atomic_add returns not what we assume!
    //int pos = atomic_add( (int *)&refdec_put_ptr, 1 );
    int pos = ATOMIC_ADD_AND_FETCH( (int *)&refdec_put_ptr, 1 );

    // Overflow
    if( (pos >= REFDEC_BUFFER_SIZE) || (pos == REFDEC_BUFFER_HALF) )
    {
        STAT_INC_CNT(DEFERRED_REFDEC_LOST);
        // We just loose refdec - big GC will pick it up
        return;
    }

    refdec_buffer[pos] = os;


}

/**
 *
 * Now here we actially perform decrements.
 *
 * We divide buffer into 2 parts. First one is filled,
 * other is processed.
 *
**/


static void deferred_refdec_thread(void *a)
{
    t_current_set_name("RefDec");
    t_current_set_priority( THREAD_PRIO_HIGH );

    while(!stop_refdec_thread)
    {
        hal_mutex_lock(  &deferred_refdec_mutex );
        // TODO timed wait
        hal_cond_wait( &start_refdec_cond, &deferred_refdec_mutex );

        STAT_INC_CNT(DEFERRED_REFDEC_RUNS);

        // Decide where to switch put pointer
        int new_put_ptr = REFDEC_BUFFER_HALF + 1; // first one used to check low half overflow

        // Was in upper page?
        if( refdec_put_ptr >= REFDEC_BUFFER_HALF )
            new_put_ptr = 0;

        //int last_pos = atomic_set( &refdec_put_ptr, new_put_ptr);
        int last_pos = ATOMIC_FETCH_AND_SET( &refdec_put_ptr, new_put_ptr);
        int start_pos = (last_pos >= REFDEC_BUFFER_HALF) ? REFDEC_BUFFER_HALF+1 : 0;

        // Check that all VM threads are either sleep or passed an bytecode instr boundary
        phantom_check_threads_pass_bytecode_instr_boundary();

        int pos;
        for( pos = start_pos; pos < last_pos; pos++ )
        {
            pvm_object_storage_t volatile *os;
            os = refdec_buffer[pos];

            assert( os->_ah.refCount > 0);
            do_ref_dec_p((pvm_object_storage_t *)os);
        }

        hal_cond_broadcast(   &end_refdec_cond );
        hal_mutex_unlock( &deferred_refdec_mutex );
    }
}


