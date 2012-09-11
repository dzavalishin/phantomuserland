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
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#define MULTIPLE_DPC_THREADS 1
#define DPC_WAKE_TIMER 1

//---------------------------------------------------------------------------

#include <phantom_libc.h>
#include <threads.h>
#include <kernel/config.h>

#include <kernel/dpc.h>

#if DPC_WAKE_TIMER
#include <kernel/net_timer.h>
#endif

//---------------------------------------------------------------------------



hal_spinlock_t          dpc_request_lock;
dpc_request *           dpc_request_first = 0;
hal_cond_t              dpc_thread_sleep_stone;
static hal_mutex_t      unused_dpc_mutex; // cond needs it!
volatile int            dpc_stop_request = 0;
volatile int            dpc_init_ok = 0;


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
    t_current_set_name("DPC Work");
    t_current_set_priority(PHANTOM_SYS_THREAD_PRIO);

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


static void dpc_timed(void)
{
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

#if !DPC_WAKE_TIMER
static void dpc_timed_waker_thread(void)
{
    t_current_set_name("DPC Waker");

    while(1)
    {
        hal_sleep_msec(1000);
        if(dpc_stop_request)
            hal_exit_kernel_thread();

        dpc_timed();
    }
}

#else // DPC_WAKE_TIMER

static void dpc_timer_wake( void *arg );

static net_timer_event dpc_nte;

static void dpc_timer_wake( void *arg )
{
    (void) arg;

    set_net_timer( &dpc_nte, 1000, dpc_timer_wake, 0, 0 );
    dpc_timed();
}

#endif

void dpc_init()
{
    SHOW_FLOW0( 1, "Starting DPC");
    hal_cond_init( &dpc_thread_sleep_stone, "DPC" );
    spinlock_init( &dpc_request_lock );

    hal_mutex_init(&unused_dpc_mutex, "DPC");

    SHOW_FLOW0( 1, "Starting DPC thread...");
    //dpc_thread_object =
    hal_start_kernel_thread(dpc_thread);

#if !DPC_WAKE_TIMER
    SHOW_FLOW0( 1, " starting DPC waker thread...");
    //dpc_timed_waker_thread_object =
    hal_start_kernel_thread(dpc_timed_waker_thread);
#else
    dpc_timer_wake(0);
#endif

    while(!dpc_init_ok)
    {
        SHOW_FLOW0( 2, " wait for DPC threads...");
        hal_sleep_msec(1); // wait for thread to start
    }

    SHOW_FLOW0( 0, "DPC start done");
}


void dpc_finish()
{
    dpc_stop_request = 1;
    // Now wake up DPC worker
    hal_cond_broadcast( &dpc_thread_sleep_stone );

}

void dpc_panic(void)
{
    if( (dpc_stop_request) || (!dpc_init_ok) )
        panic("DPC requested after dpc_finish() or before init");
}
