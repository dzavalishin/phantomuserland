/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * VM threads sleep/wakeup/snap synchronization
 *
 *
**/

#define DEBUG_MSG_PREFIX "vm.sync"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <threads.h>

#include <phantom_libc.h>

#include <kernel/snap_sync.h>
#include <hal.h>

//#define DEBUG 0

/* This is set from snap code to ask us to hold our breath */

volatile int     phantom_virtual_machine_snap_request = 0;
volatile int     phantom_virtual_machine_stop_request = 0;

//static 
volatile int     phantom_virtual_machine_threads_stopped = 0;
//volatile int     phantom_virtual_machine_threads_got_stop_request = 0;

static hal_cond_t   phantom_snap_wait_4_vm_enter;
static hal_cond_t   phantom_snap_wait_4_vm_leave;
static hal_cond_t   phantom_vm_wait_4_snap;

// BUG - really inefficient, as it wakes single cond
// and wakes all threads for most of them to go sleep again
static hal_mutex_t  interlock_mutex;
static hal_cond_t   vm_thread_wakeup_cond;


void phantom_snap_threads_interlock_init( void )
{
    if(
       hal_cond_init( &phantom_snap_wait_4_vm_enter, "Snap W4E" ) ||
       hal_cond_init( &phantom_snap_wait_4_vm_leave, "Snap W4L" ) ||
       hal_cond_init( &phantom_vm_wait_4_snap, "Snap W4S" ) ||

       hal_mutex_init( &interlock_mutex, "Snap ILck" ) ||
       hal_cond_init( &vm_thread_wakeup_cond, "VmSleep")
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
        SHOW_FLOW0( 4, "VM thread will die now");
        hal_exit_kernel_thread();
    }

    SHOW_FLOW0( 5, "VM thread will sleep for snap");
    hal_mutex_lock( &interlock_mutex );

    phantom_virtual_machine_threads_stopped++;
    hal_cond_broadcast( &phantom_snap_wait_4_vm_enter );

    SHOW_FLOW0( 5, "VM thread reported sleep, will wait now");

    //while(phantom_virtual_machine_snap_request)
        hal_cond_wait( &phantom_vm_wait_4_snap, &interlock_mutex );

    SHOW_FLOW0( 5, "VM thread awaken, will report wakeup");
    phantom_virtual_machine_threads_stopped--;
    hal_cond_broadcast( &phantom_snap_wait_4_vm_leave );

    hal_mutex_unlock( &interlock_mutex );
    SHOW_FLOW0( 5, "VM thread returns to activity");
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
    SHOW_FLOW0( 5, "phantom_snapper_wait_4_threads");

    phantom_virtual_machine_snap_request++; // Ask them to go sleep

    int threads_4_wait = phantom_vm_threads_get_count();

    while(1)
    {

        hal_mutex_lock( &interlock_mutex );
        SHOW_FLOW( 5, "Will wait for %d threads to stop, %d already did", threads_4_wait, phantom_virtual_machine_threads_stopped );
        while( phantom_virtual_machine_threads_stopped < threads_4_wait )
        {
            SHOW_FLOW0( 5, "Wait for thread");
            hal_cond_wait( &phantom_snap_wait_4_vm_enter, &interlock_mutex );
            //hal_sleep_msec(2000);
            SHOW_FLOW( 5, "Woken up aft wait for %d threads, %d stopped", threads_4_wait, phantom_virtual_machine_threads_stopped);
        }

        SHOW_FLOW0( 5, "Finished waiting for threads");
        hal_mutex_unlock( &interlock_mutex );


        // Check if some new threads were started till we count them
        int newc = phantom_vm_threads_get_count();
        if( newc > threads_4_wait )
        {
            // Yes - wait for new ones to stop as well
            threads_4_wait = newc;
            SHOW_FLOW( 5, "Found more threads (%d total), wait again", threads_4_wait);
            continue;
        }

        // TODO FIXME what if someone makes a thread here?
        break;
    }

    SHOW_FLOW0( 5, "Snapper is free to snap");

}


/*
 *
 * Used by snapper and GC'or together, so can be reentered.
 *
 */

void phantom_snapper_reenable_threads( void )
{
    SHOW_FLOW0( 5, "Snapper will reenable threads");
    hal_mutex_lock( &interlock_mutex );
    phantom_virtual_machine_snap_request--; // May wake up now

    if(phantom_virtual_machine_snap_request > 0)
    {
        // I'm not one here
        hal_mutex_unlock( &interlock_mutex );
        return;
    }

    SHOW_FLOW( 5, "Snapper sleep request is %d, will broadcast", phantom_virtual_machine_snap_request);

    hal_cond_broadcast( &phantom_vm_wait_4_snap );


    SHOW_FLOW( 5, "Snapper will wait for %d threads to awake", phantom_virtual_machine_threads_stopped);

    while( phantom_virtual_machine_threads_stopped > 0 )
    {
        hal_cond_wait( &phantom_snap_wait_4_vm_leave, &interlock_mutex );
        SHOW_FLOW( 5, "Snapper: %d threads still sleep", phantom_virtual_machine_threads_stopped);

    }
    hal_mutex_unlock( &interlock_mutex );

    SHOW_FLOW0( 5, "Snapper done waiting for awake");

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
        SHOW_FLOW0( 5, "VM thread will die now");
        hal_exit_kernel_thread();
    }

    SHOW_FLOW0( 5, "VM thread will sleep for sleep");
    hal_mutex_lock( &interlock_mutex );

    phantom_virtual_machine_threads_stopped++;
    hal_cond_broadcast( &phantom_snap_wait_4_vm_enter );
    //SHOW_FLOW0( 5, "VM thread reported sleep, will wait now");

    if( thda->spin_to_unlock )
    {
        VM_SPIN_UNLOCK( (*thda->spin_to_unlock) );
        thda->spin_to_unlock = 0;
    }
    else
    {
        if(thda->sleep_flag)
            SHOW_ERROR(0, "Warn: vm th (da %x) sleep, no spin unlock requested", thda);
    }


    //while(thda->sleep_flag)        hal_cond_wait( &(thda->wakeup_cond), &interlock_mutex );
    while(thda->sleep_flag)
    {
        SHOW_ERROR(0, "Warn: old vm sleep used, th (da %x)", thda);
        hal_cond_wait( &vm_thread_wakeup_cond, &interlock_mutex );
    }

// TODO if snap is active someone still can wake us up - resleep for snap then!

    //SHOW_FLOW0( 5, "VM thread awaken, will report wakeup");
    phantom_virtual_machine_threads_stopped--;

    hal_mutex_unlock( &interlock_mutex );
    SHOW_FLOW0( 5, "VM thread awaken");
}


void phantom_thread_put_asleep( struct data_area_4_thread *thda, VM_SPIN_TYPE *unlock_spin )
{
    // FIXME can't sleep in spinlock!
    hal_mutex_lock( &interlock_mutex );
    // TODO atomic assign
    if( thda->spin_to_unlock )
        panic( "spin unlock > 1" );

    thda->spin_to_unlock = unlock_spin;
    thda->sleep_flag++;
    hal_mutex_unlock( &interlock_mutex );
    // NB! This will work if called from SYS only! That's
    // ok since no other bytecode instr can call this.
    // Real sleep happens in phantom_thread_sleep_worker
    SHOW_FLOW0( 5, "put thread asleep");
}


void phantom_thread_wake_up( struct data_area_4_thread *thda )
{
    // TODO of course it is a bottleneck - need separate sync objects for threads
    // we can't keep usual mutexes in objects for objects are in paged mem and mutex uses
    // spinlock to run its internals
    // TODO implement old unix style sleep( var address )/wakeup( var address )? 
    hal_mutex_lock( &interlock_mutex );

    thda->sleep_flag--;
    //if(thda->sleep_flag <= 0)        hal_cond_broadcast( &thda->wakeup_cond );
    if(thda->sleep_flag <= 0)
        hal_cond_broadcast( &vm_thread_wakeup_cond );

    hal_mutex_unlock( &interlock_mutex );
}


void phantom_check_threads_pass_bytecode_instr_boundary( void )
{
    SHOW_ERROR0( 0, "unimpl!");
}



/**
 *
 *   BLOCKING VM SYSCALLS IMPL
 *
 * - vm_spin_lock -> regular kernel mutex
 * - keep mutex ptr in object, recreate mutex on restart
 * 
 * - blocking syscall - must not use vm mutex, must be unlocked on restart
 * 
 * blocking sys:
 *    - call kernel (request op)
 *    - push null (will be returned if we're snapped and restarted)
 *    - sleep (snappable, don't use thda->sleep_flag!) - kernel wakes on data ready
 *    - pop null, push ret val
 * 
 * possibly:
 *    - save args, restart syscall on kern resrart
 *
**/

#include <vm/exec.h>
#include <vm/stacks.h>
#include <vm/syscall.h>


//#define MAX_SYS_ARG 16
int vm_syscall_block( pvm_object_t this, struct data_area_4_thread *tc, pvm_object_t (*syscall_worker)( pvm_object_t , struct data_area_4_thread *, int nmethod, pvm_object_t arg ) )
{

    // NB args must be popped before we push retcode

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 2);

    //if( n_param < 1 ) SYSCALL_THROW(pvm_create_string_object( "blocking: need at least 1 parameter" ));

    int nmethod = POP_INT();
    pvm_object_t arg = POP_ARG;

    // push zero to obj stack

    pvm_ostack_push( tc->_ostack, pvm_create_null_object() ); 

    pvm_exec_save_fast_acc(tc); // Before snap

    if(phantom_virtual_machine_stop_request)
    {
        SHOW_FLOW0( 5, "VM thread will die now");
        hal_exit_kernel_thread();
    }

    SHOW_FLOW0( 5, "VM thread will sleep for blocking syscall");
    hal_mutex_lock( &interlock_mutex );

    phantom_virtual_machine_threads_stopped++;
    hal_cond_broadcast( &phantom_snap_wait_4_vm_enter );

    hal_mutex_unlock( &interlock_mutex );


    // now do syscall - can block

    pvm_object_t ret = syscall_worker( this, tc, nmethod, arg );

    // BUG FIXME snapper won't continue until this thread is unblocked: end of snap waits for all stooped threads to awake

    hal_mutex_lock( &interlock_mutex );

    //while(tc->sleep_flag)         hal_cond_wait( &vm_thread_wakeup_cond, &interlock_mutex );

    if(phantom_virtual_machine_snap_request)
        hal_cond_wait( &phantom_vm_wait_4_snap, &interlock_mutex );

    //SHOW_FLOW0( 5, "VM thread awaken, will report wakeup");
    phantom_virtual_machine_threads_stopped--;
    hal_cond_broadcast( &phantom_snap_wait_4_vm_leave );

    hal_mutex_unlock( &interlock_mutex );
    SHOW_FLOW0( 5, "VM thread awaken after blocking syscall");

    // pop zero from obj stack
    // push ret val to obj stack

    pvm_ostack_pop( tc->_ostack );
    pvm_ostack_push( tc->_ostack, ret );

    return 1; // not throw
}







