/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Bytecode interpreter: run new instance
 *
 * See <https://github.com/dzavalishin/phantomuserland/wiki/VirtualMachine>
 *
**/

#define DEBUG_MSG_PREFIX "vm.exec.run"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_assert.h>

#include <vm/root.h>
#include <vm/internal_da.h>
#include <vm/internal.h>
#include <vm/object_flags.h>
#include <vm/exception.h>
#include <vm/alloc.h>

#include <vm/exec.h>
#include <vm/code.h>
#include <vm/reflect.h>

#include <vm/stacks.h>
#include <vm/syscall.h>

//#include "ids/opcode_ids.h"

#include <kernel/snap_sync.h>
#include <kernel/debug.h>

#include <exceptions.h>

#include "main.h"



/**
 * @brief Run Phantom VM instance
 * 
 * @param this_object    Object to become 'this' in started method
 * @param method         Ordinal of method to run
 * @param n_args         Num of args passed
 * @param args           Arguments
 * 
 * @return Object: actual return value of method executed.
 * 
 * @note This executes code in a tight loop. It will prevent
 * snaps from being done. Redo with a separate thread start.
 *
 */

pvm_object_t 
pvm_exec_run_method(
                    pvm_object_t this_object,
                    int method,
                    int n_args,
                    pvm_object_t args[]
                   )
{
    pvm_object_t new_cf = pvm_create_call_frame_object();
    struct data_area_4_call_frame* cfda = pvm_object_da( new_cf, call_frame );

    int i;
    for( i = n_args; i > 0; i-- )
    {
        pvm_ostack_push( pvm_object_da(cfda->ostack, object_stack), ref_inc_o( args[i-1] ) );
    }

    pvm_istack_push( pvm_object_da(cfda->istack, integer_stack), n_args); // pass him real number of parameters

    pvm_object_t code = pvm_exec_find_method( this_object, method, NULL );
    pvm_exec_set_cs( cfda, code );
    cfda->this_object = ref_inc_o( this_object );

    pvm_object_t thread = pvm_create_thread_object( new_cf );

    //vm_unlock_persistent_memory(); // pvm_exec will lock, do not need nested lock - will prevent snaps
    // NO - we must prevent snap here!! it won't work right after restart, part of state is in C code
    pvm_exec( thread );
    //vm_lock_persistent_memory();


    pvm_object_t ret = pvm_ostack_pop( pvm_object_da(cfda->ostack, object_stack) );

    pvm_release_thread_object( thread );

    return ret;
}



// -----------------------------------------------------------------------
//
// Run new thread
//
// -----------------------------------------------------------------------


/**
 * @brief Run new runnable in thread.
 * 
 * @note Object must be of '.ru.dz.phantom.system.runnable' class, we run method no. 8.
 * 
**/
errno_t pvm_run_new_thread( pvm_object_t object ) //, int n_args, pvm_object_t args[] )
{
    // Don't need do SYS_FREE_O(object) since we store it as 'this'

    // Check object class to be runnable or subclass
    if( !pvm_object_class_is_or_child( object, pvm_get_class_noload(".ru.dz.phantom.system.runnable") ) )
        return EINVAL;
    
    pvm_object_t new_cf = pvm_create_call_frame_object();
    struct data_area_4_call_frame* cfda = pvm_object_da( new_cf, call_frame );

    //pvm_ostack_push( pvm_object_da(cfda->ostack, object_stack), me ); // No args
    pvm_istack_push( pvm_object_da(cfda->istack, integer_stack), 0); // pass him real number of parameters

    pvm_object_t code = pvm_exec_find_method( object, 8, 0 ); // last arg is used in debug only
    pvm_exec_set_cs( cfda, code );
    cfda->this_object = object;

    pvm_object_t thread = pvm_create_thread_object( new_cf );

    phantom_activate_thread(thread);
}

#if 0
struct cb_parm
{
    struct data_area_4_connection *	da;
    pvm_object_t                        arg;
};


static void run_cb_thread(void *arg)
{
    t_current_set_name("conn cb");

    struct cb_parm *p = arg;

    pvm_object_t args[1] = { p->arg };
    struct data_area_4_connection * da = p->da;

    free(p);

    SHOW_FLOW( 1, "cb conn '%s'", da->name );

    vm_lock_persistent_memory();

    while(da->n_active_callbacks > 16)
    {
        SHOW_ERROR( 1, "conn '%s' too much cb: %d", da->name, da->n_active_callbacks );
        vm_unlock_persistent_memory();
        hal_sleep_msec(100);
        vm_lock_persistent_memory();
    }

    da->n_active_callbacks++; // TODO atomic?
    pvm_exec_run_method(da->callback, da->callback_method, 1, args);
    da->n_active_callbacks--; // TODO atomic?

    vm_unlock_persistent_memory();

    SHOW_FLOW( 1, "cb conn '%s' done", da->name );

    // Just die, no more meaning of life
}

static errno_t run_cb( struct data_area_4_connection *da, pvm_object_t o )
{
    struct cb_parm *p = calloc(1, sizeof(struct cb_parm));
    if(!p)
    {
        ref_dec_o(o);
        return ENOMEM;
    }

    // TODO must add o to kernel referenced objects list?

    p->da = da;
    p->arg = o;

    int tid = hal_start_thread( run_cb_thread, p, 0);

    if( tid < 0 )
    {
        ref_dec_o(o);
        free(p);
        return EAGAIN;
    }

    return 0;
}


#define GOGO(__o) \
    if( pvm_isnull(__o) )        return ENOMEM; \
    return run_cb(da, __o);

//! Call connection's callback with binary payload
errno_t phantom_connection_callback_binary( struct data_area_4_connection *da, void *data, size_t size )
{
    pvm_object_t bo = pvm_create_binary_object( size, data );
    GOGO(bo);
}

//! Call connection's callback with string payload
errno_t phantom_connection_callback_string( struct data_area_4_connection *da, const char *data )
{
    pvm_object_t o = pvm_create_string_object( data );
    GOGO(o);
}

//! Call connection's callback with int payload
errno_t phantom_connection_callback_int( struct data_area_4_connection *da, int data )
{
    pvm_object_t o = pvm_create_int_object( data );
    GOGO(o);
}


# endif











