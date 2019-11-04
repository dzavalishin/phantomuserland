/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * .internal.thread class implementation
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/


#define DEBUG_MSG_PREFIX "vm.sysc.thread"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>

#include "vm/object.h"
#include "vm/internal.h"
#include "vm/internal_da.h"
#include "vm/syscall.h"
#include "vm/root.h"
#include "vm/p2c.h"
#include "vm/alloc.h"

static int debug_print = 0;

// --------- thread ---------------------------------------------------------

static int si_thread_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "thread" ));
}

/*
static int si_thread_8_start( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    //phantom_activate_thread(me);
    //SYSCALL_RETURN(pvm_create_string_object( "thread" ));
    SYSCALL_RETURN_NOTHING;
}
*/

static int si_thread_10_pause( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_thread *meda = pvm_object_da( me, thread );

    if(meda != tc)
        SYSCALL_THROW_STRING("Thread can pause itself only");

    SYSCALL_THROW_STRING("Not this way");

    SYSCALL_RETURN_NOTHING;
}

static int si_thread_11_continue( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
//#if OLD_VM_SLEEP
//    struct data_area_4_thread *meda = pvm_object_da( me, thread );

    //hal_spin_lock(&meda->spin);
//    if( !meda->sleep_flag )
//      SYSCALL_THROW_STRING("Thread is not sleeping in continue");
    //hal_spin_unlock(&meda->spin);

//    SYSCALL_WAKE_THREAD_UP(meda);
//#else
    SYSCALL_THROW_STRING("Not this way");
//#endif

    SYSCALL_RETURN_NOTHING;
}

static int si_thread_14_getOsInterface( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    pvm_object_t root = find_root_object_storage();
    pvm_object_t o = pvm_get_field( root, PVM_ROOT_OBJECT_OS_ENTRY );
    SYSCALL_RETURN( ref_inc_o( o ) );
}

static int si_thread_13_getUser( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    struct data_area_4_thread *meda = pvm_object_da( me, thread );

    SYSCALL_RETURN(ref_inc_o(meda->owner));
}


static int si_thread_12_getEnvironment( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    struct data_area_4_thread *meda = pvm_object_da( me, thread );

    // TODO vm_spinlock
    if( pvm_is_null(meda->environment) )
    {
        pvm_object_t env = pvm_create_string_object(".phantom.environment");
        pvm_object_t cl = pvm_exec_lookup_class_by_name( env );
        meda->environment = pvm_create_object(cl);
        ref_dec_o(env);
    }

    SYSCALL_RETURN(meda->environment);
}


syscall_func_t  syscall_table_4_thread[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_thread_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &invalid_syscall,               &invalid_syscall,
    &si_thread_10_pause,            &si_thread_11_continue,
    &si_thread_12_getEnvironment,   &si_thread_13_getUser,
    &si_thread_14_getOsInterface,   &si_void_15_hashcode
};

DECLARE_SIZE(thread);
