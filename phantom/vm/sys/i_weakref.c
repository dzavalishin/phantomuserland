/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal (native) class: weakref
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/


#define DEBUG_MSG_PREFIX "vm.sysc.weak"
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

#include <vm/p2c.h>

#include <vm/wrappers.h>

static int debug_print = 0;



#if COMPILE_WEAKREF
// --------- weakref -------------------------------------------------------

static int si_weakref_5_tostring( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(weakref)" ));
}


static int si_weakref_8_getMyObject( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
#if 0
    struct data_area_4_weakref *da = pvm_object_da( o, weakref );

    // All we do is return new reference to our object,
    // incrementing refcount before
    int ie = hal_save_cli();
    hal_spin_lock( &da->lock );

    pvm_object_t out = ref_inc_o( da->object );

    hal_spin_unlock( &da->lock );
    if( ie ) hal_sti();
#else
    pvm_object_t out = pvm_weakref_get_object( o );
#endif
    SYSCALL_RETURN( out );

}

errno_t si_weakref_9_resetMyObject(pvm_object_t o )
{
    struct data_area_4_weakref *da = pvm_object_da( o, weakref );

    errno_t rc = EWOULDBLOCK;

#if WEAKREF_SPIN
    wire_page_for_addr( &da->lock );
    int ie = hal_save_cli();
    hal_spin_lock( &da->lock );
#else
    hal_mutex_lock( &da->mutex );
#endif

    // As we are interlocked with above, no refcount inc can be from us
    // ERROR if more than one weakref is pointing to obj, possibility
    // exist than GC will reset some of them, and than will still let
    // object to exist. We need to make sure only one weakref exists.
    if(da->object->_ah.refCount == 0)
    {
        da->object = 0;
        //da->object.interface = 0;
        rc = 0;
    }

#if WEAKREF_SPIN
    hal_spin_unlock( &da->lock );
    if( ie ) hal_sti();
    unwire_page_for_addr( &da->lock );
#else
    hal_mutex_unlock( &da->mutex );
#endif

    return rc;
}





syscall_func_t  syscall_table_4_weakref[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_weakref_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_weakref_8_getMyObject,      &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(weakref);


#endif
