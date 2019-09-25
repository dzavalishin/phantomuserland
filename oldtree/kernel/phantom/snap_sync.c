/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * VM threads sleep/wakeup/snap synchronization
 *
 * See <https://github.com/dzavalishin/phantomuserland/wiki/SnapSync>
 *
**/

#define DEBUG_MSG_PREFIX "vm.sync"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <threads.h>
#include <thread_private.h>

#include <phantom_libc.h>

#include <kernel/snap_sync.h>
#include <kernel/init.h>
#include <kernel/config.h> // just for VSC to highlight code 
#include <hal.h>


#if CONF_DUAL_PAGEMAP

static void do_vm_lock_persistent_memory( void );
static void do_vm_unlock_persistent_memory( void );

static volatile int vm_snap_lock_inited = 0;
static volatile int vm_persistent_memory_lock_count = 0;

volatile int        phantom_virtual_machine_snap_request = 0; //< Non-zero if we want to do snapshot (or doing it)

static hal_cond_t   phantom_snap_wait_4_vm_enter;
//static hal_cond_t   phantom_snap_wait_4_vm_leave;
static hal_cond_t   phantom_vm_wait_4_snap;

// BUG - really inefficient, as it wakes single cond
// and wakes all threads for most of them to go sleep again
static hal_mutex_t  interlock_mutex;
//static hal_cond_t   vm_thread_wakeup_cond;


void phantom_snap_threads_interlock_init( void )
{
    if(
       hal_cond_init( &phantom_snap_wait_4_vm_enter, "Snap W4E" ) ||
       //hal_cond_init( &phantom_snap_wait_4_vm_leave, "Snap W4L" ) ||
       hal_cond_init( &phantom_vm_wait_4_snap, "Snap W4S" ) ||

       hal_mutex_init( &interlock_mutex, "Snap ILck" ) 
       //|| hal_cond_init( &vm_thread_wakeup_cond, "VmSleep")
      )
        panic("Can't init thread/snap interlocks");
    vm_snap_lock_inited++;
}



/*
 *
 * Called by any virt. machine or kernel thread when it sees
 * phantom_virtual_machine_snap_request to be nonzero.
 *
 * Thread is blocked for snapshot time.
 * 
 */

void phantom_thread_wait_4_snap( void )
{
    do_vm_unlock_persistent_memory(); // revoke access to pers mem

    if(phantom_virtual_machine_stop_request)
    {
        SHOW_FLOW0( 4, "VM thread will die now");
        hal_exit_kernel_thread();
    }

    SHOW_FLOW0( 5, "VM thread will sleep for snap");
    hal_mutex_lock( &interlock_mutex );

    hal_cond_broadcast( &phantom_snap_wait_4_vm_enter ); // No

    SHOW_FLOW0( 5, "VM thread reported sleep, will wait now");
    hal_cond_wait( &phantom_vm_wait_4_snap, &interlock_mutex );

    SHOW_FLOW0( 5, "VM thread awaken, will report wakeup");
    do_vm_lock_persistent_memory(); // gain access to pers mem
    //hal_cond_broadcast( &phantom_snap_wait_4_vm_leave );

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

    //phantom_virtual_machine_snap_request++; // Ask them to go sleep
    atomic_add( &phantom_virtual_machine_snap_request, 1 );

    hal_mutex_lock( &interlock_mutex );
    SHOW_FLOW( 5, "Will wait for %d threads to stop", vm_persistent_memory_lock_count );
    while( vm_persistent_memory_lock_count > 0 )
    {
        SHOW_FLOW0( 5, "Wait for thread");
        hal_cond_wait( &phantom_snap_wait_4_vm_enter, &interlock_mutex );
        SHOW_FLOW( 5, "Woken up aft wait for threads, %d not stopped yet", vm_persistent_memory_lock_count );
    }

    //SHOW_FLOW0( 5, "Finished waiting for threads");
    hal_mutex_unlock( &interlock_mutex );

    SHOW_FLOW0( 5, "Snapper is free to snap");
}

/**
 *
 * Release threads and let them access persistent memory again.
 *
 * Used by snapper and GC'or together, so can be reentered.
 *
**/

void phantom_snapper_reenable_threads( void )
{
    SHOW_FLOW0( 5, "Snapper will reenable threads");
    hal_mutex_lock( &interlock_mutex );
    //phantom_virtual_machine_snap_request--; // May wake up now
    atomic_add( &phantom_virtual_machine_snap_request, -1 );

    if(phantom_virtual_machine_snap_request > 0)
    {
        // I'm not one here
        SHOW_FLOW( 5, "Snapper - do not reenable threads, phantom_virtual_machine_snap_request is still not zero (%d)", phantom_virtual_machine_snap_request);
        hal_mutex_unlock( &interlock_mutex );
        return;
    }

    SHOW_FLOW( 5, "Snapper sleep request is %d, will broadcast", phantom_virtual_machine_snap_request);

    hal_cond_broadcast( &phantom_vm_wait_4_snap );
#if 0
    SHOW_FLOW( 5, "Snapper will wait for %d threads to awake", phantom_virtual_machine_threads_stopped);

#if VM_SYNC_NOWAIT_BLOCKED
    while( phantom_virtual_machine_threads_stopped - phantom_virtual_machine_threads_blocked > 0 )
#else
    while( phantom_virtual_machine_threads_stopped > 0 )
#endif
    {
        hal_cond_wait( &phantom_snap_wait_4_vm_leave, &interlock_mutex );
        SHOW_FLOW( 5, "Snapper: %d threads still sleep", phantom_virtual_machine_threads_stopped);

    }
#endif
    hal_mutex_unlock( &interlock_mutex );
    SHOW_FLOW0( 5, "Snapper done waiting for awake"); // TODO we did not wait - OK?

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
#include <vm/alloc.h>


//#define MAX_SYS_ARG 16

// interlock code of VM blocking syscall (part of .internal.connection class) implementation
// called from si_connection_13_blocking, calls passed syscall worker implemented in cn_*.c in kernel
int vm_syscall_block( pvm_object_t this, struct data_area_4_thread *tc, pvm_object_t (*syscall_worker)( pvm_object_t , struct data_area_4_thread *, int nmethod, pvm_object_t arg ) )
{

    // NB args must be popped before we push retcode

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 2);

    //if( n_param < 1 ) SYSCALL_THROW(pvm_create_string_object( "blocking: need at least 1 parameter" ));

    int nmethod = POP_INT();
    pvm_object_t arg = POP_ARG;

    // push zero to obj stack - fake return code if we are snapped during syscall

    pvm_ostack_push( tc->_ostack, pvm_create_null_object() ); 

    pvm_exec_save_fast_acc(tc); // Before snap

    if(phantom_virtual_machine_stop_request)
    {
        SHOW_FLOW0( 5, "VM thread will die now");
        hal_exit_kernel_thread();
    }

    vm_unlock_persistent_memory();

    // now do syscall - can block
    pvm_object_t ret = syscall_worker( this, tc, nmethod, arg );

    vm_lock_persistent_memory();

    ref_dec_o( arg ); // BUG FIXME ref will be lost if restart - add to restart list before call, remove after?
    // BUG FIXME snapper won't continue until this thread is unblocked: end of snap waits for all stooped threads to awake
    // ? fixed with phantom_virtual_machine_threads_blocked?

    // pop zero from obj stack
    // push ret val to obj stack

    pvm_ostack_pop( tc->_ostack );
    pvm_ostack_push( tc->_ostack, ret );

    return 1; // not throw
}



// ----------------------------------------------------------------
// persistent memory access interlock
// ----------------------------------------------------------------

static void do_vm_lock_persistent_memory( void )
{
	atomic_add( &vm_persistent_memory_lock_count, 1 );
    GET_CURRENT_THREAD()->sub_from_pers_mem_lock_count++; // to update counter above in thread death
    t_set_paged_mem( 1 ); //< Enable access to paged memory
}

void vm_lock_persistent_memory( void )
{

    do_vm_lock_persistent_memory();

    // catch thread if we wait for snapshot
    // we must enter there locked, hence such an order

    if(phantom_virtual_machine_snap_request)
        phantom_thread_wait_4_snap();
}

// release access to persistent memory address space, enable snapshots
void do_vm_unlock_persistent_memory( void )
{
	atomic_add( &vm_persistent_memory_lock_count, -1 );
    GET_CURRENT_THREAD()->sub_from_pers_mem_lock_count--; // to update counter above in thread death
    t_set_paged_mem( 0 ); //< Disable access to paged memory
}

void vm_unlock_persistent_memory( void )
{
    do_vm_unlock_persistent_memory();

    // trigger snapshot if it waits for us
    if(vm_snap_lock_inited) 
    {
        // TODO need to take interlock mutex? Seems that dont...
        hal_cond_broadcast( &phantom_snap_wait_4_vm_enter );
    }

}

/**
 * 
 * Called from thread death handler.
 * Passed counter is t->sub_from_pers_mem_lock_count
 * 
**/
void vm_release_snap_lock( int count )
{
    atomic_add( &vm_persistent_memory_lock_count, - count );
}

#else // CONF_DUAL_PAGEMAP

// ------------------------------------------- OLD UNUSED CODE --------------------------------------------

// Left for reference to implement pvm thread block/unblock

//#if OLD_VM_SLEEP
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

/* dz off
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
*/
//#endif // OLD_VM_SLEEP


void phantom_check_threads_pass_bytecode_instr_boundary( void )
{
	// TODO just wait for snapshot?
    SHOW_ERROR0( 0, "unimpl!");
}





#endif

