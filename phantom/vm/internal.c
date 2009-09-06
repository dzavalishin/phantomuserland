/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/


#include "vm/internal.h"
#include "vm/object_flags.h"

#include <phantom_libc.h>



struct internal_class pvm_internal_classes[] =
{
    {
        ".internal.void",
        PVM_ROOT_OBJECT_NULL_CLASS,
        syscall_table_4_void,
        pvm_internal_init_void,
        pvm_gc_iter_void,
        0,
        0
    },
    {
        ".internal.class",
        PVM_ROOT_OBJECT_CLASS_CLASS,
        syscall_table_4_class,
        pvm_internal_init_class,
        pvm_gc_iter_class,
        sizeof(struct data_area_4_class),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS
    },
    {
        ".internal.interface",
        PVM_ROOT_OBJECT_INTERFACE_CLASS,
        syscall_table_4_interface,
        pvm_internal_init_interface,
        pvm_gc_iter_interface,
        0, // Dynamic
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE // no internal flag?
    },
    {
        ".internal.code",
        PVM_ROOT_OBJECT_CODE_CLASS,
        syscall_table_4_code,
        pvm_internal_init_code,
        pvm_gc_iter_code,
        0, // Dynamic
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE
    },
    {
        ".internal.int",
        PVM_ROOT_OBJECT_INT_CLASS,
        syscall_table_4_int,
        pvm_internal_init_int,
        pvm_gc_iter_int,
        sizeof(struct data_area_4_int),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_INT
    },
    {
        ".internal.string",
        PVM_ROOT_OBJECT_STRING_CLASS,
        syscall_table_4_string,
        pvm_internal_init_string,
        pvm_gc_iter_string,
        sizeof(struct data_area_4_string), // Dynamic!
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING
    },
    {
        ".internal.container.array",
        PVM_ROOT_OBJECT_ARRAY_CLASS,
        syscall_table_4_array,
        pvm_internal_init_array,
        pvm_gc_iter_array,
        sizeof(struct data_area_4_array),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_DECOMPOSEABLE|PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE
    },
    {
        ".internal.container.page",
        PVM_ROOT_OBJECT_PAGE_CLASS,
        syscall_table_4_page,
        pvm_internal_init_page,
        pvm_gc_iter_page,
        0, // Dynamic
        0 // PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL
    },
    {
        ".internal.thread",
        PVM_ROOT_OBJECT_THREAD_CLASS,
        syscall_table_4_thread,
        pvm_internal_init_thread,
        pvm_gc_iter_thread,
        sizeof(struct data_area_4_thread),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL
    },
    {
        ".internal.call_frame",
        PVM_ROOT_OBJECT_CALL_FRAME_CLASS,
        syscall_table_4_call_frame,
        pvm_internal_init_call_frame,
        pvm_gc_iter_call_frame,
        sizeof(struct data_area_4_call_frame),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CALL_FRAME
    },
    {
        ".internal.istack",
        PVM_ROOT_OBJECT_ISTACK_CLASS,
        syscall_table_4_istack,
        pvm_internal_init_istack,
        pvm_gc_iter_istack,
        sizeof(struct data_area_4_integer_stack),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME
    },
    {
        ".internal.ostack",
        PVM_ROOT_OBJECT_OSTACK_CLASS,
        syscall_table_4_ostack,
        pvm_internal_init_ostack,
        pvm_gc_iter_ostack,
        sizeof(struct data_area_4_object_stack),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME
    },
    {
        ".internal.estack",
        PVM_ROOT_OBJECT_ESTACK_CLASS,
        syscall_table_4_estack,
        pvm_internal_init_estack,
        pvm_gc_iter_estack,
        sizeof(struct data_area_4_exception_stack),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME
    },
    {
        ".internal.boot",
        PVM_ROOT_OBJECT_BOOT_CLASS,
        syscall_table_4_boot,
        pvm_internal_init_boot,
        pvm_gc_iter_boot,
        sizeof(struct data_area_4_boot),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL
    },
    {
        ".internal.io.tty",
        PVM_ROOT_OBJECT_TTY_CLASS,
        syscall_table_4_tty,
        pvm_internal_init_tty,
        pvm_gc_iter_tty,
        sizeof(struct data_area_4_tty),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL
    },
/*    {
        ".internal.io.driver",
        PVM_ROOT_OBJECT_DRIVER_CLASS,
        syscall_table_4_driver,
        pvm_internal_init_driver,
        pvm_gc_iter_driver,
        sizeof(struct data_area_4_driver),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL
    },
*/
    {
        ".internal.mutex",
        PVM_ROOT_OBJECT_MUTEX_CLASS,
        syscall_table_4_mutex,
        pvm_internal_init_mutex,
        pvm_gc_iter_mutex,
        sizeof(struct data_area_4_mutex),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL
    },

    {
        ".internal.binary",
        PVM_ROOT_OBJECT_BINARY_CLASS,
        syscall_table_4_binary,
        pvm_internal_init_binary,
        pvm_gc_iter_binary,
        sizeof(struct data_area_4_binary), // TODO problem - dynamically sized!
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL
    },

    {
        ".internal.bitmap",
        PVM_ROOT_OBJECT_BITMAP_CLASS,
        syscall_table_4_bitmap,
        pvm_internal_init_bitmap,
        pvm_gc_iter_bitmap,
        sizeof(struct data_area_4_bitmap),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL
    },

    {
        ".internal.closure",
        PVM_ROOT_OBJECT_CLOSURE_CLASS,
        syscall_table_4_closure,
        pvm_internal_init_closure,
        pvm_gc_iter_closure,
        sizeof(struct data_area_4_closure),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL
    },

    {
        ".internal.world",
        PVM_ROOT_OBJECT_WORLD_CLASS,
        syscall_table_4_world,
        pvm_internal_init_world,
        pvm_gc_iter_world,
        sizeof(struct data_area_4_world),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL
    },


};

int pvm_n_internal_classes = sizeof(pvm_internal_classes) / sizeof(struct internal_class);

int     pvm_iclass_by_root_index( int index )
{
    int i;
    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        if( pvm_internal_classes[i].root_index == index )
            return i;
    }
    panic("no iclass with root index = %d", index);
    /*NORETURN*/
}

int     pvm_iclass_by_class( struct pvm_object_storage *cs )
{
    int i;
    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        if( pvm_internal_classes[i].class_object.data == cs )
            return i;
    }
    panic("no iclass for 0x%x", cs);
    /*NORETURN*/
}

int     pvm_iclass_by_name( const char *name )
{
    int i;
    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        if( 0 == strcmp( pvm_internal_classes[i].name, name ) )
            return i;
    }
    panic("no iclass named %s", name);
}


#if 0
#define CHECK_NAME(data, len, name) ( (len == (sizeof(name)-1)) && (0 == strncmp(data, name, len)) )

static struct pvm_object
pvm_hardcoded_lookup_class( struct pvm_object class_name )
{
    if(!IS_PHANTOM_STRING(class_name))
        return pvm_object(); // just return null

    int len = class_name.get_str_len();
    const char *data = (const char *)class_name.get_str_data();

    if( 0 != strncmp( data, ".internal.", 10 ) )
        return fall_back( class_name );

    data += 10; // skip ".internal"
    len -= 10;

    if( CHECK_NAME(data, len, "void"))	return get_void_class();
    if( CHECK_NAME(data, len, "object"))    return get_void_class();
    if( CHECK_NAME(data, len, "int"))	return pvm_object_storage::get_int_class();
    if( CHECK_NAME(data, len, "string"))	return pvm_object_storage::get_string_class();

    if( 0 == strncmp( data, "container.", 10) )
    {
        data += 10; // skip "container."
        len -= 10;

        if( CHECK_NAME(data, len, "array"))	return pvm_object_storage::get_array_class();

        return fall_back( class_name );
    }

    if( 0 == strncmp( data, "io.", 3) )
    {
        data += 3;
        len -= 3;

        if( CHECK_NAME(data, len, "tty"))	return get_text_display_class();

        return fall_back( class_name );
    }

    return fall_back( class_name );
}
#endif

struct pvm_object pvm_lookup_internal_class(struct pvm_object name)
{
    if( EQ_STRING_P2C(name, ".internal.object" ) )
        return pvm_internal_classes[PVM_ROOT_OBJECT_NULL_CLASS].class_object;

    int i;
    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        if( EQ_STRING_P2C(name, pvm_internal_classes[i].name ) )
            return pvm_internal_classes[i].class_object;
    }

    struct pvm_object retNull;
    retNull.data = 0;
    return retNull;
}

