/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Deferred procedure calls.
 * Callouts are done from special thread, not from
 * interrupt context.
 *
**/

#define DEBUG_MSG_PREFIX "dpc"
#include "debug_ext.h"
#define debug_level_flow 1
#define debug_level_error 10
#define debug_level_info 10

//---------------------------------------------------------------------------

#include <phantom_libc.h>
#include <threads.h>
#include <kernel/config.h>

#include "dpc.h"

//---------------------------------------------------------------------------

#define MULTIPLE_DPC_THREADS 1


hal_spinlock_t         	dpc_request_lock;
dpc_request *		dpc_request_first = 0;
hal_cond_t		dpc_thread_sleep_stone;
static hal_mutex_t 	unused_dpc_mutex; // cond needs it!
volatile char		dpc_stop_request = 0;
static volatile char	dpc_init_ok = 0;


static dpc_request *    dpc_request_find()
{
    int ie = hal_save_cli();
    spinlock_lock( &dpc_request_lock, "dpc find" );
    SHOW_FLOW0( 2, "DPC find... ");

    dpc_request *i;
    for( i = dpc_request_first; i; i = i->next )
    {
        //if( !i->go ) return; // q must be sorted...
        if( !i->go ) continue;
        i->go = 0;

        SHOW_FLOW0( 2, "DPC found\n");
        spinlock_unlock( &dpc_request_lock, "dpc find" );
        if(ie) hal_sti();
        return i;
    }

    SHOW_FLOW0( 2, "DPC not found\n");
    spinlock_unlock( &dpc_request_lock, "dpc find" );
    if(ie) hal_sti();
    return 0;
}


//static
void
dpc_request_run()
{
    dpc_request *i;
    do
    {
        i = dpc_request_find();
        if(i == 0) return;
        SHOW_FLOW( 3, "\n\nDPC fires 0x%X...", i->func);
        //SHOW_FLOW0( 2, "<DPC ");
        i->func(i->arg);
        //SHOW_FLOW0( 2, " DPC> ");
        SHOW_FLOW0( 3, " ...DPC fire DONE\n\n");

    } while(1);
}


static int idle_dpc_threads = 0;
static int dpc_threads = 0;

#define MAX_DPC_IDLE_THREADS 6
#define MIN_DPC_IDLE_THREADS 2

static void dpc_thread(void)
{
    hal_set_thread_name("DPC Work");
    hal_set_current_thread_priority(PHANTOM_SYS_THREAD_PRIO);

    dpc_init_ok = 1;

    dpc_threads++;

    while(1)
    {
        SHOW_FLOW0( 4, "\nDPC thread step");

        //do {
        //asm("int $3");
        dpc_request_run();
        //  } while( dpc_request::have_active() );

        idle_dpc_threads++;

        if(dpc_stop_request
#if MULTIPLE_DPC_THREADS
           || (idle_dpc_threads > MAX_DPC_IDLE_THREADS)
#endif
          )
        {
            SHOW_FLOW0( 1, "DPC stop" );
            dpc_threads--;
            idle_dpc_threads--;
            hal_exit_kernel_thread();
        }

        hal_mutex_lock(&unused_dpc_mutex);
        SHOW_FLOW( 3, "DPC sleep at 0x%X\n", &dpc_thread_sleep_stone);
        hal_cond_wait( &dpc_thread_sleep_stone, &unused_dpc_mutex );
        SHOW_FLOW0( 3, "DPC wakeup\n");
        hal_mutex_unlock(&unused_dpc_mutex);

        idle_dpc_threads--;
    }

}

// TODO use regular timedcall
static void dpc_timed_waker_thread(void)
{
    hal_set_thread_name("DPC Waker");

    while(1)
    {
        hal_sleep_msec(1000);
        if(dpc_stop_request)
            hal_exit_kernel_thread();

#if MULTIPLE_DPC_THREADS
        if( idle_dpc_threads < MIN_DPC_IDLE_THREADS )
        {
            SHOW_FLOW( 1, "Starting extra %d DPC thread...", dpc_threads);
            hal_start_kernel_thread(dpc_thread);
        }
#endif


        //hal_cond_broadcast( &dpc_thread_sleep_stone );
        hal_cond_signal( &dpc_thread_sleep_stone ); // Wake one thread
    }
}

//static void * dpc_thread_object;
//static void * dpc_timed_waker_thread_object;

void dpc_init()
{
    SHOW_FLOW0( 0, "Starting DPC");
    hal_cond_init( &dpc_thread_sleep_stone, "DPC" );
    spinlock_init( &dpc_request_lock );

    hal_mutex_init(&unused_dpc_mutex, "DPC");

    SHOW_FLOW0( 1, "Starting DPC thread...");
    //dpc_thread_object =
    hal_start_kernel_thread(dpc_thread);

    SHOW_FLOW0( 1, " starting DPC waker thread...");
    //dpc_timed_waker_thread_object =
    hal_start_kernel_thread(dpc_timed_waker_thread);

    while(!dpc_init_ok)
    {
        SHOW_FLOW0( 2, " wait for DPC threads...");
        hal_sleep_msec(1); // wait for thread to start
    }

    SHOW_FLOW0( 1, "DPC start done");
}


void dpc_finish()
{
    dpc_stop_request = 1;
    // Now wake up DPC worker
    hal_cond_broadcast( &dpc_thread_sleep_stone );

}

