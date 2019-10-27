/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal (native) class: class 
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/


#define DEBUG_MSG_PREFIX "vm.sysc.class"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <stdlib.h>

#include <phantom_libc.h>
#include <threads.h>

#include <kernel/snap_sync.h>
#include <kernel/vm.h>
#include <kernel/atomic.h>

#include <vm/syscall.h>
#include <vm/object.h>
#include <vm/object_flags.h>
#include <vm/root.h>
#include <vm/exec.h>
#include <vm/bulk.h>
#include <vm/alloc.h>
#include <vm/internal.h>
#include <vm/json.h>

#include <vm/p2c.h>

#include <vm/wrappers.h>


static int debug_print = 0;





static int si_class_class_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "class" ));
}

// TODO who needs this?
static int si_class_class_8_new_class( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;

    CHECK_PARAM_COUNT(3);

    pvm_object_t class_name = args[0];
    int n_object_slots = AS_INT(args[0]);
    pvm_object_t iface = args[0];

    ASSERT_STRING(class_name);

    pvm_object_t new_class = pvm_create_class_object(class_name, iface, sizeof(pvm_object_t ) * n_object_slots);

    SYSCALL_RETURN( new_class );
}

static int si_class_10_set_static( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_class *meda = pvm_object_da( me, class );

    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(2);

    pvm_object_t static_val = args[1];
    int n_slot = AS_INT(args[0]);

    pvm_set_ofield( meda->static_vars, n_slot, static_val );

    SYSCALL_RETURN_NOTHING;
}

static int si_class_11_get_static( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_class *meda = pvm_object_da( me, class );
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);

    int n_slot = AS_INT(args[0]);

    pvm_object_t iret = pvm_get_ofield( meda->static_vars, n_slot );
    ref_inc_o( iret );
    SYSCALL_RETURN( iret );
}


// Check if given object is instance of this class
static int si_class_14_instanceof( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    //struct data_area_4_class *meda = pvm_object_da( me, class );
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);

    pvm_object_t instance = args[0];

//#if VM_INSTOF_RECURSIVE
    int is = pvm_object_class_is_or_child( instance, me );
//#else
//    int is = pvm_object_class_exactly_is( instance, me );
//#endif // VM_INSTOF_RECURSIVE
    SYS_FREE_O(instance);

    SYSCALL_RETURN(pvm_create_int_object( is ));
}


syscall_func_t  syscall_table_4_class[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_class_class_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_class_class_8_new_class,    &invalid_syscall,
    &si_class_10_set_static,        &si_class_11_get_static,
    &invalid_syscall,               &invalid_syscall,
    &si_class_14_instanceof,        &si_void_15_hashcode
};

DECLARE_SIZE(class);

