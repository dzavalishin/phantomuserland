/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Virtual machine threads handler.
 *
**/

#define DEBUG_MSG_PREFIX "vmthread"
#include <debug_ext.h>
#define debug_level_flow 2
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>

#include <phantom_libc.h>

#include <vm/object.h>
#include <vm/exec.h>
#include <vm/internal_da.h>
#include <vm/root.h>

#include <vm/syscall.h>

#include <threads.h>
#include <thread_private.h>

#include <time.h>

#include <kernel/snap_sync.h>
#include <hal.h>


volatile int     phantom_virtual_machine_stop_request = 0;


// TODO something against running the same thread twice or more times

static volatile int n_vm_threads = 0;
static volatile int all_threads_started = 0;


static void remove_vm_thread_from_list(pvm_object_storage_t *os)
{
    // TODO check that is is a thread

    int nthreads  = get_array_size(pvm_root.threads_list);

    if( !nthreads )
        SHOW_ERROR0( 0, "There were 0 live threads in image, and some thread is dead. Now -1?" );

    int nkill = 0;
    while(nthreads--)
    {
        pvm_object_t th =  pvm_get_array_ofield(pvm_root.threads_list, nthreads );
        pvm_check_is_thread( th );
        if( th == os )
        {
            pvm_set_array_ofield(pvm_root.threads_list, nthreads, pvm_create_null_object() );
            nkill++;
        }
    }

    if(1 != nkill)
        printf("Nkill = %d\n", nkill);
}


static void thread_death_handler( phantom_thread_t *t )
{
    //pvm_object_t current_thread = *((pvm_object_t *)arg);
    n_vm_threads--;

    printf("thread_death_handler called\n");

    vm_lock_persistent_memory();

    pvm_object_storage_t *os = t->owner;
    if( os == 0 )
    {
        SHOW_ERROR0( 0, "!!! thread_death_handler - no pointer to Vm thread object!" );
        vm_unlock_persistent_memory();
        return;
    }

    assert( os->_ah.object_start_marker == PVM_OBJECT_START_MARKER );

    //struct data_area_4_thread * tda = ((struct data_area_4_thread *)&(os->da));
    // TODO check os class == thread

    remove_vm_thread_from_list(os);

    vm_unlock_persistent_memory();
}


// KLUDGE! we're passing onstack object address - this var will be zero when
// all of them are not used anymore
static volatile int args_used = 0;

static void thread_run_func( void *arg )
{
    t_current_set_name("VM");

    pvm_object_t current_thread = *((pvm_object_t *)arg);

    args_used--;

    t_current_set_death_handler( (void *) thread_death_handler );

    n_vm_threads++;
    pvm_exec( current_thread );
    n_vm_threads--;
}



static void start_new_vm_thread(pvm_object_t new_thread)
{
    args_used++;

    //int tid = hal_start_thread(thread_run_func, &new_thread, THREAD_FLAG_VM|THREAD_FLAG_USER);
    int tid = hal_start_thread(thread_run_func, &new_thread, THREAD_FLAG_VM);

    struct data_area_4_thread *tda = pvm_object_da( new_thread, thread );
    tda->tid = tid;

    t_set_owner( tid, new_thread );
    //phantom_thread_t *t = get_thread(tid);
    //t->owner = new_thread;

    while(args_used > 0)
        hal_sleep_msec(1);
/*
#if OLD_VM_SLEEP
    if(tda->sleep_flag)
    {
        //timedcall_t *e = &(tda->timer);
        net_timer_event *e = &(tda->timer);

        int didit = 0;

        if( e->pending )
        {
            bigtime_t msecMore = e->sched_time - hal_system_time();

            msecMore /= 1000;

            if( msecMore > 0 )
            {
                // Thread is sleeping on timer, reactivate wakeup
                phantom_wakeup_after_msec( msecMore, tda );
            }
            else
            {
                // Sleep time is passed, just wake
                SYSCALL_WAKE_THREAD_UP(tda);
            }
            didit = 1;
        }

        if(!didit)
        {
            SHOW_ERROR( 0, "Sleeping VM thread has no means to wakeup (%p)", new_thread );
        }

    }
#endif // OLD_VM_SLEEP
*/
}


/* static
phantom_thread_t* vm_thread_2_kernel_thread(pvm_object_t vm_thread)
{
    struct data_area_4_thread *tda = pvm_object_da( vm_thread, thread );
    return get_thread(tda->tid);
}
*/

//int phantom_vm_threads_get_count() { return n_vm_threads; }


// -----------------------------------------------------------------------
/*
static void phantom_add_thread_to_thread_list(pvm_object_t new_thread)
{
    // ERROR! Races? Synchronize? Inside of method!
    
    pvm_append_array( pvm_root.threads_list, new_thread );

}
*/


void phantom_activate_thread(pvm_object_t new_thread)
{
    pvm_check_is_thread( new_thread );
    // already there, added in object create
    //phantom_add_thread_to_thread_list( new_thread );

    // Don't autostart threads until kernel startup
    // called activate_all_threads - or else
    // threads called from boot code will
    // start twice and first time they will start
    // too early - before actual user threads start.

    if(all_threads_started)
        start_new_vm_thread( new_thread );
}


void activate_all_threads()
{
    vm_lock_persistent_memory();

    int nthreads  = get_array_size(pvm_root.threads_list);

    if( nthreads == 0 )
        SHOW_ERROR0( 0, "There are 0 live threads in image, system must be dead :(" );

    SHOW_FLOW( 3, "Activating %d threads", nthreads);

    while(nthreads--)
    {
        pvm_object_t th =  pvm_get_array_ofield(pvm_root.threads_list, nthreads );
        pvm_check_is_thread( th );
        start_new_vm_thread( th );
    }

    vm_unlock_persistent_memory();
    all_threads_started = 1;
}


void phantom_finish_all_threads(void)
{
    phantom_virtual_machine_stop_request = 1;
#if !NEW_SNAP_SYNC
    phantom_virtual_machine_snap_request = 1; // TODO check snap_sync to make sure it works as supposed with vm_lock_persistent_memory based code
    SHOW_FLOW( 2, "Finishing %d threads", n_vm_threads);
    //SHOW_FLOW0( 2, "Finishing threads" );
    while(n_vm_threads > 0)
    {
        SHOW_FLOW( 6, "Waiting for threads to finish, %d left", n_vm_threads);
        hal_sleep_msec(1);
        // For snapper may, possibly, decrement it till then
        phantom_virtual_machine_snap_request = 1;
    }
#endif
}





