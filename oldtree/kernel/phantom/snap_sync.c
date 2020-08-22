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
//static hal_mutex_t  snap_interlock_mutex;
hal_mutex_t  snap_interlock_mutex; // accessed from vm_sleep.c

#if NEW_VM_SLEEP
//static hal_cond_t   vm_thread_wakeup_cond;
hal_cond_t   vm_thread_wakeup_cond; // accessed from vm_sleep.c
#endif

void phantom_snap_threads_interlock_init( void )
{
    if(
       hal_cond_init( &phantom_snap_wait_4_vm_enter, "Snap W4E" ) ||
       //hal_cond_init( &phantom_snap_wait_4_vm_leave, "Snap W4L" ) ||
       hal_cond_init( &phantom_vm_wait_4_snap, "Snap W4S" ) ||

       hal_mutex_init( &snap_interlock_mutex, "Snap ILck" ) 
#if NEW_VM_SLEEP
       || hal_cond_init( &vm_thread_wakeup_cond, "VmSleep")
#endif
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
    hal_mutex_lock( &snap_interlock_mutex );

    hal_cond_broadcast( &phantom_snap_wait_4_vm_enter ); // No

    SHOW_FLOW0( 5, "VM thread reported sleep, will wait now");
    hal_cond_wait( &phantom_vm_wait_4_snap, &snap_interlock_mutex );

    SHOW_FLOW0( 5, "VM thread awaken, will report wakeup");
    do_vm_lock_persistent_memory(); // gain access to pers mem
    //hal_cond_broadcast( &phantom_snap_wait_4_vm_leave );

    hal_mutex_unlock( &snap_interlock_mutex );
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

    hal_mutex_lock( &snap_interlock_mutex );
    SHOW_FLOW( 5, "Will wait for %d threads to stop", vm_persistent_memory_lock_count );
    while( vm_persistent_memory_lock_count > 0 )
    {
        SHOW_FLOW0( 5, "Wait for thread");
        hal_cond_wait( &phantom_snap_wait_4_vm_enter, &snap_interlock_mutex );
        SHOW_FLOW( 5, "Woken up aft wait for threads, %d not stopped yet", vm_persistent_memory_lock_count );
    }

    //SHOW_FLOW0( 5, "Finished waiting for threads");
    hal_mutex_unlock( &snap_interlock_mutex );

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
    hal_mutex_lock( &snap_interlock_mutex );
    //phantom_virtual_machine_snap_request--; // May wake up now
    atomic_add( &phantom_virtual_machine_snap_request, -1 );

    if(phantom_virtual_machine_snap_request > 0)
    {
        // I'm not one here
        SHOW_FLOW( 5, "Snapper - do not reenable threads, phantom_virtual_machine_snap_request is still not zero (%d)", phantom_virtual_machine_snap_request);
        hal_mutex_unlock( &snap_interlock_mutex );
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
        hal_cond_wait( &phantom_snap_wait_4_vm_leave, &snap_interlock_mutex );
        SHOW_FLOW( 5, "Snapper: %d threads still sleep", phantom_virtual_machine_threads_stopped);

    }
#endif
    hal_mutex_unlock( &snap_interlock_mutex );
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

#define TMP_RESTART_CALL 1
#define TMP_NO_UNLOCK 0
//#define MAX_SYS_ARG 16

// SYS code does instruction undo/redo so that snap stores state with SYS instruction not executed
#if 1


// interlock code of VM blocking syscall (part of .internal.connection class) implementation
// called from si_connection_13_blocking, calls passed syscall worker implemented in cn_*.c in kernel
int vm_syscall_block( pvm_object_t this, struct data_area_4_thread *tc, pvm_object_t (*syscall_worker)( pvm_object_t , struct data_area_4_thread *, int nmethod, pvm_object_t arg ), pvm_object_t* ret, int n_args, pvm_object_t *args )
{
    (void) n_args;

    ASSERT_INT(args[1]);
    int nmethod = pvm_get_int(args[1]);
    pvm_object_t arg = args[0];

    SHOW_FLOW( 5, "method %d start", nmethod );

    if(phantom_virtual_machine_stop_request)
    {
        SHOW_FLOW0( 5, "VM thread will die now");
        hal_exit_kernel_thread();
    }

    vm_unlock_persistent_memory();

    // now do syscall - can block
    pvm_object_t oret = syscall_worker( this, tc, nmethod, arg );

    vm_lock_persistent_memory();

    ref_dec_o( arg ); // Ref will be lost if restart? - No, control will get here again.
    *ret = oret;

    SHOW_FLOW( 5, "method %d end", nmethod );
    return 1; // Not throw
}


#else




// interlock code of VM blocking syscall (part of .internal.connection class) implementation
// called from si_connection_13_blocking, calls passed syscall worker implemented in cn_*.c in kernel
int vm_syscall_block( pvm_object_t this, struct data_area_4_thread *tc, pvm_object_t (*syscall_worker)( pvm_object_t , struct data_area_4_thread *, int nmethod, pvm_object_t arg ), pvm_object_t* ret, int n_args, pvm_object_t *args )
{

    // NB args must be popped before we push retcode

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 2);

    //if( n_param < 1 ) SYSCALL_THROW(pvm_create_string_object( "blocking: need at least 1 parameter" ));

#if TMP_RESTART_CALL

    pvm_object_t __ival = POP_ARG;
    ASSERT_INT(__ival); 
    //SYS_FREE_O(__ival);  // no - after syscall is done
    int nmethod = pvm_get_int(__ival);

    pvm_object_t arg = POP_ARG;

    // Attempt to restart us if snap/restart happened

    // Restore stack state and move back IP
    pvm_istack_push( tc->_istack, n_param ); // arg count
    pvm_ostack_push( tc->_ostack, arg );
    pvm_ostack_push( tc->_ostack, __ival );

    tc->code.IP -= VM_SYSCALL_INSTR_SIZE;
#else
    int nmethod = POP_INT();
    pvm_object_t arg = POP_ARG;
    // push zero to obj stack - fake return code if we are snapped during syscall
    pvm_ostack_push( tc->_ostack, pvm_create_null_object() ); 
#endif
    pvm_exec_save_fast_acc(tc); // Before snap

    if(phantom_virtual_machine_stop_request)
    {
        SHOW_FLOW0( 5, "VM thread will die now");
        hal_exit_kernel_thread();
    }
// JUST FOR TEST - run blocking syscalls with pers mem locked
// test if it is a cause for VM crash
#if TMP_NO_UNLOCK
#warning FIX ME
    pvm_object_t oret = syscall_worker( this, tc, nmethod, arg );
#else
    vm_unlock_persistent_memory();

    // now do syscall - can block
    pvm_object_t oret = syscall_worker( this, tc, nmethod, arg );

    vm_lock_persistent_memory();
#endif
    ref_dec_o( arg ); // BUG FIXME ref will be lost if restart - add to restart list before call, remove after?
    // BUG FIXME snapper won't continue until this thread is unblocked: end of snap waits for all stooped threads to awake
    // ? fixed with phantom_virtual_machine_threads_blocked?

    // pop zero from obj stack
    // push ret val to obj stack
#if TMP_RESTART_CALL
    // We did it, now clean up args from stack
    pvm_ostack_pop( tc->_ostack );
    pvm_ostack_pop( tc->_ostack );
    pvm_istack_pop( tc->_istack ); // arg count
    tc->code.IP += VM_SYSCALL_INSTR_SIZE;
#else
    pvm_ostack_pop( tc->_ostack );
#endif
    pvm_ostack_push( tc->_ostack, oret );

    return 1; // not throw
}
#endif



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



void phantom_check_threads_pass_bytecode_instr_boundary( void )
{
	// TODO just wait for snapshot?
    SHOW_ERROR0( 0, "unimpl!");
}





#endif

