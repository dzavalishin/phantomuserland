/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Synchronization - userland sleep/wakeup
 *
**/

#define DEBUG_MSG_PREFIX "vm.sleep"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <phantom_libc.h>
//#include <time.h>
#include <threads.h>

#include <kernel/snap_sync.h>
#include <kernel/vm.h>
#include <kernel/atomic.h>
#include <kernel/debug.h>

//#include <vm/syscall.h>
#include <vm/object.h>
//#include <vm/root.h>
#include <vm/exec.h>
//#include <vm/bulk.h>
#include <vm/spin.h>
//#include <vm/internal.h>

//#include <vm/p2c.h>

//#include <hal.h>

extern hal_mutex_t  snap_interlock_mutex;  // from snap_sync.c
extern hal_cond_t   vm_thread_wakeup_cond; // from snap_sync.c


// ------------------------------------------- UNTESTED CODE --------------------------------------------


void phantom_thread_put_asleep( struct data_area_4_thread *thda, VM_SPIN_TYPE *unlock_spin )
{
    // FIXME can't sleep in spinlock!
    //hal_mutex_lock( &snap_interlock_mutex );
    pvm_spin_lock( &thda->lock);

    // TODO atomic assign
    if( thda->spin_to_unlock )
        panic( "spin unlock > 1" );

    thda->spin_to_unlock = unlock_spin;
    thda->sleep_flag++;
    
    pvm_spin_unlock( &thda->lock);
    //hal_mutex_unlock( &snap_interlock_mutex );
    // NB! This will work if called from SYS only! That's
    // ok since no other bytecode instr can call this.
    // Real sleep happens in phantom_thread_sleep_worker
    SHOW_FLOW0( 5, "put thread asleep");

    // Actual sleep happens in sleep_worker    
}


void phantom_thread_wake_up( struct data_area_4_thread *thda )
{
    // TODO of course it is a bottleneck - need separate sync objects for threads
    // we can't keep usual mutexes in objects for objects are in paged mem and mutex uses
    // spinlock to run its internals
    // TODO implement old unix style sleep( var address )/wakeup( var address )? 
    //hal_mutex_lock( &snap_interlock_mutex );

    pvm_spin_lock( &thda->lock);

    thda->sleep_flag--;

    assert(thda->sleep_flag >= 0);

    if(thda->sleep_flag <= 0)
        hal_cond_broadcast( &vm_thread_wakeup_cond );

    pvm_spin_unlock( &thda->lock);
    //hal_mutex_unlock( &snap_interlock_mutex );
}



/*
 *
 * Called by any virt. machine thread when it sees
 * thread da->sleep_flag to be nonzero.
 *
 */

void phantom_thread_sleep_worker( struct data_area_4_thread *thda )
{
    pvm_exec_save_fast_acc(thda); // Before snap

    if(phantom_virtual_machine_stop_request)
    {
        SHOW_FLOW0( 5, "VM thread will die now");
        hal_exit_kernel_thread();
    }

    SHOW_FLOW0( 5, "VM thread will sleep for sleep");
    pvm_spin_lock( &thda->lock );

resleep:
    if( thda->spin_to_unlock )
    {
        pvm_spin_unlock( thda->spin_to_unlock );
        thda->spin_to_unlock = 0;
    }
    else
    {
        if(thda->sleep_flag)
            SHOW_ERROR(0, "Warn: vm th (da %zx) sleep, no spin unlock requested", (size_t)thda);
    }

    pvm_spin_unlock( &thda->lock );

    vm_unlock_persistent_memory();    
    hal_mutex_lock( &snap_interlock_mutex );
    // TODO pass thread to unlock down there? Or is it ok to unlock here?
    while(thda->sleep_flag)
    {
        SHOW_ERROR(0, "new vm sleep used, th (da %zx)", (size_t)thda);
        hal_cond_wait( &vm_thread_wakeup_cond, &snap_interlock_mutex );
    }
    hal_mutex_unlock( &snap_interlock_mutex );
    vm_lock_persistent_memory();

#warning check resleep on mutex
    // If we wake up after cond, there was some mutex unlocked by cond wait
    // Now we have to relock it
    if( thda->cond_mutex )
    {
        pvm_spin_lock( &thda->lock );

        pvm_object_t mutex = thda->cond_mutex;
        thda->cond_mutex = 0;

        vm_mutex_lock( mutex, thda );

        goto resleep;
    }

    SHOW_FLOW0( 5, "VM thread awaken");
}


