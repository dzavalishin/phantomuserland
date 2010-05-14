#include <threads.h>

#include <phantom_libc.h>

#include "snap_sync.h"
#include "hal.h"

#define DEBUG 0

/* This is set from snap code to ask us to hold our breath */

volatile int     phantom_virtual_machine_snap_request = 0;
volatile int     phantom_virtual_machine_stop_request = 0;

//static 
volatile int     phantom_virtual_machine_threads_stopped = 0;

static hal_cond_t   phantom_snap_wait_4_vm_enter;
static hal_cond_t   phantom_snap_wait_4_vm_leave;
static hal_cond_t   phantom_vm_wait_4_snap;

static hal_mutex_t  interlock_mutex;

void phantom_snap_threads_interlock_init( void )
{
    if(
       hal_cond_init( &phantom_snap_wait_4_vm_enter, "Snap W4E" ) ||
       hal_cond_init( &phantom_snap_wait_4_vm_leave, "Snap W4L" ) ||
       hal_cond_init( &phantom_vm_wait_4_snap, "Snap W4S" ) ||

       hal_mutex_init( &interlock_mutex, "Snap ILck" )
      )
        panic("Can't init thread/snap interlocks");
}


/*
 *
 * Called by any virt. machine thread when it sees
 * phantom_virtual_machine_snap_request to be nonzero.
 *
 */

void phantom_thread_wait_4_snap( void )
{
    if(phantom_virtual_machine_stop_request)
    {
        if(DEBUG) printf("Thread will die now\n");
        //pthread_exit(0);
        hal_exit_kernel_thread();
    }

    if(DEBUG) printf("Thread will sleep for snap\n");
    hal_mutex_lock( &interlock_mutex );

    phantom_virtual_machine_threads_stopped++;
    hal_cond_broadcast( &phantom_snap_wait_4_vm_enter );
    if(DEBUG) printf("Thread reported sleep, will wait now\n");
    //while(phantom_virtual_machine_snap_request)
        hal_cond_wait( &phantom_vm_wait_4_snap, &interlock_mutex );

    if(DEBUG) printf("Thread awaken, will report wakeup\n");
    phantom_virtual_machine_threads_stopped--;
    hal_cond_broadcast( &phantom_snap_wait_4_vm_leave );

    hal_mutex_unlock( &interlock_mutex );
    if(DEBUG) printf("Thread returns to activity\n");
}

/*
 *
 * Make sure all virtual machine threads are asleep waiting for the
 * snapper to do a snap.
 *
 * Used by snapper and GC'or together, so can be reentered.
 */

void phantom_snapper_wait_4_threads( void )
{
    if(DEBUG) printf("phantom_snapper_wait_4_threads\n");

    phantom_virtual_machine_snap_request++; // Ask them to go sleep

    int threads_4_wait = phantom_vm_threads_get_count();

    while(1)
    {

        hal_mutex_lock( &interlock_mutex );
        if(DEBUG) printf("Will wait for %d threads to stop, %d already did\n", threads_4_wait, phantom_virtual_machine_threads_stopped );
        while( phantom_virtual_machine_threads_stopped < threads_4_wait )
        {
            if(DEBUG) printf("Wait for thread\n");
            hal_cond_wait( &phantom_snap_wait_4_vm_enter, &interlock_mutex );
            //hal_sleep_msec(2000);
            if(DEBUG) printf("Woken up aft wait for %d threads, %d stopped\n", threads_4_wait, phantom_virtual_machine_threads_stopped);

        }
        if(DEBUG) printf("Finished waiting for threads\n");
        hal_mutex_unlock( &interlock_mutex );


        // Check if some new threads were started till we count them
        int newc = phantom_vm_threads_get_count();
        if( newc > threads_4_wait )
        {
            // Yes - wait for new ones to stop as well
            threads_4_wait = newc;
            if(DEBUG) printf("Found more threads (%d total), wait again\n", threads_4_wait);
            continue;
        }

        break;
    }

    if(DEBUG) printf("Snapper is free to snap\n");

}


/*
 *
 * Used by snapper and GC'or together, so can be reentered.
 *
 */

void phantom_snapper_reenable_threads( void )
{
    if(DEBUG) printf("Snapper will reenable threads\n");
    hal_mutex_lock( &interlock_mutex );
    phantom_virtual_machine_snap_request--; // May wake up now

    if(phantom_virtual_machine_snap_request > 0)
    {
        // I'm not one here
        hal_mutex_unlock( &interlock_mutex );
        return;
    }

    if(DEBUG) printf("Snapper sleep request is %d, will broadcast\n", phantom_virtual_machine_snap_request);

    hal_cond_broadcast( &phantom_vm_wait_4_snap );


    if(DEBUG) printf("Snapper will wait for %d threads to awake\n", phantom_virtual_machine_threads_stopped);

    while( phantom_virtual_machine_threads_stopped > 0 )
    {
        hal_cond_wait( &phantom_snap_wait_4_vm_leave, &interlock_mutex );
        if(DEBUG) printf("Snapper: %d threads still sleep\n", phantom_virtual_machine_threads_stopped);

    }
    hal_mutex_unlock( &interlock_mutex );

    if(DEBUG) printf("Snapper done waiting for awake\n");

}








// ----------------------------------------------------------------
// userland sleep/wakeup code
// ----------------------------------------------------------------

/*
 *
 * Called by any virt. machine thread when it sees
 * phantom_virtual_machine_snap_request to be nonzero.
 *
 */

void phantom_thread_sleep_worker( struct data_area_4_thread *thda )
{
    if(phantom_virtual_machine_stop_request)
    {
        if(DEBUG) printf("Thread will die now\n");
        //pthread_exit(0);
        hal_exit_kernel_thread();
    }

    if(DEBUG) printf("Thread will sleep for sleep\n");
    hal_mutex_lock( &interlock_mutex );

    phantom_virtual_machine_threads_stopped++;
    hal_cond_broadcast( &phantom_snap_wait_4_vm_enter );
    //if(DEBUG) printf("Thread reported sleep, will wait now\n");
    while(thda->sleep_flag)
        hal_cond_wait( &(thda->wakeup_cond), &interlock_mutex );

    //if(DEBUG) printf("Thread awaken, will report wakeup\n");
    phantom_virtual_machine_threads_stopped--;

    hal_mutex_unlock( &interlock_mutex );
    if(DEBUG) printf("Thread awaken\n");
}


void phantom_thread_put_asleep( struct data_area_4_thread *thda )
{
    thda->sleep_flag++;
    // NB! This will work if called from SYS only! That's
    // ok since no other bytecode instr can call this.
    // Real sleep happens in phantom_thread_sleep_worker
printf("put thread asleep\n");
}


void phantom_thread_wake_up( struct data_area_4_thread *thda )
{
    thda->sleep_flag--;
    if(thda->sleep_flag <= 0)
        hal_cond_broadcast( &thda->wakeup_cond );
}



