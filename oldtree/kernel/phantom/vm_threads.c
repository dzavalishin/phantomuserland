#include "config.h"

#include <phantom_libc.h>

#include <vm/object.h>
#include <vm/exec.h>
#include <vm/internal_da.h>
#include <vm/root.h>

#include <vm/syscall.h>

#include <threads.h>
// TEMP! Remove!
#include "threads/thread_private.h"

#include "snap_sync.h"
#include "hal.h"



#define DEBUG 0

// TODO something against running the same thread twice or more times

static volatile int n_vm_threads = 0;
static volatile int all_threads_started = 0;



static void thread_death_handler( void *arg )
{
    (void) arg;

    //struct pvm_object current_thread = *((struct pvm_object *)arg);
    n_vm_threads--;

    printf("thread_death_handler called\n");
}


// KLUDGE! we're passing onstack object address - this var will be zero when
// all of them are not used anymore
static volatile int args_used = 0;

static void thread_run_func( void *arg )
{
    hal_set_thread_name("VM");

    struct pvm_object current_thread = *((struct pvm_object *)arg);

    args_used--;

    hal_set_thread_death_handler( (void *) thread_death_handler );

    n_vm_threads++;
    pvm_exec( current_thread );
    n_vm_threads--;
}



static void start_new_vm_thread(struct pvm_object new_thread)
{
    args_used++;

    //int rc = pthread_create(&tid, 0, thread_run_func, &new_thread);
    //if( rc )        panic("can't create thread");

    //pthread_t tid =
    hal_start_thread(thread_run_func, &new_thread, THREAD_FLAG_VM|THREAD_FLAG_USER);

    while(args_used > 0)
        hal_sleep_msec(1);
}


int phantom_vm_threads_get_count() { return n_vm_threads; }


// -----------------------------------------------------------------------
/*
static void phantom_add_thread_to_thread_list(struct pvm_object new_thread)
{
    // ERROR! Races? Synchronize? Inside of method!
    
    pvm_append_array( pvm_root.threads_list.data, new_thread );

}
*/


void phantom_activate_thread(struct pvm_object new_thread)
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
    int nthreads  = get_array_size(pvm_root.threads_list.data);

#if DEBUG
    if( nthreads )
        printf("Activating %d threads", nthreads);
    else
        printf("There are 0 live threads in image, system must be dead :(");
#endif

    while(nthreads--)
    {
        struct pvm_object th =  pvm_get_array_ofield(pvm_root.threads_list.data, nthreads );
        pvm_check_is_thread( th );
        start_new_vm_thread( th );
    }

    all_threads_started = 1;
}


void phantom_finish_all_threads(void)
{
    phantom_virtual_machine_stop_request = 1;
    phantom_virtual_machine_snap_request = 1;

    if(DEBUG) printf("Finishing %d threads\n", n_vm_threads);
    while(n_vm_threads > 0)
    {
        if(DEBUG) printf("Waiting for threads to finish, %d left\n", n_vm_threads);
        hal_sleep_msec(1);
        // For snapper may, possibly, decrement it till then
        phantom_virtual_machine_snap_request = 1;
    }
}
