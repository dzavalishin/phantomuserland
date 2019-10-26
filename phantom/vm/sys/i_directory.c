/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal (native) class: directory (hash map)
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/


#define DEBUG_MSG_PREFIX "vm.sysc.dir"
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

#include <hashfunc.h>


static int debug_print = 0;




// --------- directory -------------------------------------------------------



static int si_directory_4_equals( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_THROW_STRING( "dir.equals: not implemented" );
    //SYSCALL_RETURN(pvm_create_string_object( "(directory)" ));
}


static int si_directory_5_tostring( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(directory)" ));
}

static int si_directory_8_put( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_directory *da = pvm_object_da( o, directory );
    DEBUG_INFO;

    CHECK_PARAM_COUNT(2);

    pvm_object_t val = args[1];
    pvm_object_t key = args[0];
    ASSERT_STRING(key);

    errno_t rc = hdir_add( da, pvm_get_str_data(key), pvm_get_str_len(key), val );

    SYS_FREE_O(key); // dir code creates it's own binary object
    if( rc ) SYS_FREE_O( val ); // we didn't put it there

    SYSCALL_RETURN(pvm_create_int_object( rc ));
}

static int si_directory_9_get( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_directory *da = pvm_object_da( o, directory );
    DEBUG_INFO;

    CHECK_PARAM_COUNT(1);
    pvm_object_t key = args[0];
    ASSERT_STRING(key);

    pvm_object_t out;
    errno_t rc = hdir_find( da, pvm_get_str_data(key), pvm_get_str_len(key), &out, 0 );
    if( rc )
        SYSCALL_RETURN_NOTHING;
    else
        SYSCALL_RETURN(out);
}


static int si_directory_10_remove( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_directory *da = pvm_object_da( o, directory );
    DEBUG_INFO;

    CHECK_PARAM_COUNT(1);

    pvm_object_t key = args[0];
    ASSERT_STRING(key);

    pvm_object_t out; // unused
    errno_t rc = hdir_find( da, pvm_get_str_data(key), pvm_get_str_len(key), &out, 1 );
    SYSCALL_RETURN(pvm_create_int_object( rc ));
}

static int si_directory_11_size( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_directory *da = pvm_object_da( o, directory );
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_int_object( da->nEntries ));
}

// Returns iterator
static int si_directory_12_iterate( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    // TODO implement dir iterator

    SYSCALL_THROW_STRING( "dir.iterate: not implemented" );
    SYSCALL_RETURN_NOTHING;
}




syscall_func_t  syscall_table_4_directory[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_directory_4_equals,         &si_directory_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_directory_8_put,            &si_directory_9_get,
    &si_directory_10_remove,        &si_directory_11_size,
    &si_directory_12_iterate,       &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode,

};
DECLARE_SIZE(directory);

