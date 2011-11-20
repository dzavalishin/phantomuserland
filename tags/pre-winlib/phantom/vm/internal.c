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

#define IINIT(__cn) \
        syscall_table_4_##__cn, \
        pvm_internal_init_##__cn, \
        pvm_gc_iter_##__cn

/*
#define IINIT(__cn) \
        syscall_table_4_##__cn, n_syscall_table_4_##__cn, \
        pvm_internal_init_##__cn, \
        pvm_gc_iter_##__cn
*/


// NB!  GC optimization: if PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE flag is present, the pvm_gc_iter_##something  function will not be used!

struct internal_class pvm_internal_classes[] =
{
    {
        ".internal.void",
        PVM_ROOT_OBJECT_NULL_CLASS,
        syscall_table_4_void, // n_syscall_table_4_void,
        pvm_internal_init_void,
        0 /*pvm_gc_iter_void*/,
        0, // no finalizer
        0, // no restart func
        0,
        PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE,
        {0,0}
    },
    {
        ".internal.class",
        PVM_ROOT_OBJECT_CLASS_CLASS,
        IINIT(class),
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_class),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS,
        {0,0}
    },
    {
        ".internal.interface",
        PVM_ROOT_OBJECT_INTERFACE_CLASS,
        IINIT(interface),
        0, // no finalizer
        0, // no restart func
        0, // Dynamic
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE, // no internal flag!
        {0,0}
    },
    {
        ".internal.code",
        PVM_ROOT_OBJECT_CODE_CLASS,
        syscall_table_4_code, // n_syscall_table_4_code,
        pvm_internal_init_code,
        0 /*pvm_gc_iter_code*/,
        0, // no finalizer
        0, // no restart func
        0, // Dynamic
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE|PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE,
        {0,0}
    },
    {
        ".internal.int",
        PVM_ROOT_OBJECT_INT_CLASS,
        syscall_table_4_int, // n_syscall_table_4_int,
        pvm_internal_init_int,
        0 /*pvm_gc_iter_int*/,
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_int),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_INT|PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE,
        {0,0}
    },
    {
        ".internal.string",
        PVM_ROOT_OBJECT_STRING_CLASS,
        syscall_table_4_string, // n_syscall_table_4_string,
        pvm_internal_init_string,
        0 /*pvm_gc_iter_string*/,
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_string), // Dynamic!
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING|PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE,
        {0,0}
    },
    {
        ".internal.container.array",
        PVM_ROOT_OBJECT_ARRAY_CLASS,
        IINIT(array),
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_array),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_DECOMPOSEABLE|PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE,
        {0,0}
    },
    {
        ".internal.container.page",
        PVM_ROOT_OBJECT_PAGE_CLASS,
        IINIT(page),
        0, // no finalizer
        0, // no restart func
        0, // Dynamic
        0, // PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL
        {0,0}
    },
    {
        ".internal.thread",
        PVM_ROOT_OBJECT_THREAD_CLASS,
        IINIT(thread),
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_thread),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_THREAD,
        {0,0}
    },
    {
        ".internal.call_frame",
        PVM_ROOT_OBJECT_CALL_FRAME_CLASS,
        IINIT(call_frame),
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_call_frame),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CALL_FRAME,
        {0,0}
    },
    {
        ".internal.istack",
        PVM_ROOT_OBJECT_ISTACK_CLASS,
        IINIT(istack),
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_integer_stack),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME,
        {0,0}
    },
    {
        ".internal.ostack",
        PVM_ROOT_OBJECT_OSTACK_CLASS,
        IINIT(ostack),
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_object_stack),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME,
        {0,0}
    },
    {
        ".internal.estack",
        PVM_ROOT_OBJECT_ESTACK_CLASS,
        IINIT(estack),
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_exception_stack),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME,
        {0,0}
    },
    {
        ".internal.boot",
        PVM_ROOT_OBJECT_BOOT_CLASS,
        syscall_table_4_boot, // n_syscall_table_4_boot,
        pvm_internal_init_boot,
        0 /*pvm_gc_iter_boot*/,
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_boot),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE,
        {0,0}
    },
    {
        ".internal.io.tty",
        PVM_ROOT_OBJECT_TTY_CLASS,
        syscall_table_4_tty, // n_syscall_table_4_tty,
        pvm_internal_init_tty,
        0 /*pvm_gc_iter_tty*/,
        pvm_gc_finalizer_tty, // no finalizer
        pvm_restart_tty,
        sizeof(struct data_area_4_tty),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE,
        {0,0}
    },
/*    {
        ".internal.io.driver",
        PVM_ROOT_OBJECT_DRIVER_CLASS,
        syscall_table_4_driver, // n_syscall_table_4_driver,
        pvm_internal_init_driver,
        pvm_gc_iter_driver,
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_driver),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL,
        {0,0}
    },
*/
    {
        ".internal.mutex",
        PVM_ROOT_OBJECT_MUTEX_CLASS,
        syscall_table_4_mutex, // n_syscall_table_4_mutex,
        pvm_internal_init_mutex,
        0 /*pvm_gc_iter_mutex*/,
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_mutex),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE,
        {0,0}
    },

    {
        ".internal.cond",
        PVM_ROOT_OBJECT_COND_CLASS,
        syscall_table_4_cond, // n_syscall_table_4_cond,
        pvm_internal_init_cond,
        0 /*pvm_gc_iter_cond*/,
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_cond),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE,
        {0,0}
    },

    {
        ".internal.sema",
        PVM_ROOT_OBJECT_SEMA_CLASS,
        syscall_table_4_sema, // n_syscall_table_4_sema,
        pvm_internal_init_sema,
        0 /*pvm_gc_iter_sema*/,
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_sema),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE,
        {0,0}
    },

    {
        ".internal.binary",
        PVM_ROOT_OBJECT_BINARY_CLASS,
        syscall_table_4_binary, // n_syscall_table_4_binary,
        pvm_internal_init_binary,
        0 /*pvm_gc_iter_binary*/,
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_binary), // TODO problem - dynamically sized!
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE,
        {0,0}
    },

    {
        ".internal.bitmap",
        PVM_ROOT_OBJECT_BITMAP_CLASS,
        IINIT(bitmap),
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_bitmap),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL,
        {0,0}
    },

    {
        ".internal.closure",
        PVM_ROOT_OBJECT_CLOSURE_CLASS,
        IINIT(closure),
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_closure),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL,
        {0,0}
    },

#if COMPILE_WEAKREF
    {
        ".internal.weakref",
        PVM_ROOT_OBJECT_WEAKREF_CLASS,
        IINIT(weakref),
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_weakref),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL,
        {0,0}
    },
#endif
    {
        ".internal.world",
        PVM_ROOT_OBJECT_WORLD_CLASS,
        syscall_table_4_world, // n_syscall_table_4_world,
        pvm_internal_init_world,
        0 /*pvm_gc_iter_world*/,
        0, // no finalizer
        0, // no restart func
        sizeof(struct data_area_4_world),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE,
        {0,0}
    },

#if 0
    {
        ".internal.udp",
        PVM_ROOT_OBJECT_UDP_CLASS,
        syscall_table_4_udp, // n_syscall_table_4_udp,
        pvm_internal_init_udp,
        0 /*pvm_gc_iter_udp*/,
        pvm_..._udp,
        0, // no restart func
        sizeof(struct data_area_4_udp),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|
        PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE|
        PHANTOM_OBJECT_STORAGE_FLAG_IS_FINALIZER,
        {0,0}
    },
#endif


    {
        ".internal.window",
        PVM_ROOT_OBJECT_WINDOW_CLASS,
        IINIT(window),
        pvm_gc_finalizer_window,
        pvm_restart_window, // no restart func
        sizeof(struct data_area_4_window),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL,
        {0,0}
    },

    {
        ".internal.directory",
        PVM_ROOT_OBJECT_DIRECTORY_CLASS,
        IINIT(directory),
        pvm_gc_finalizer_directory,
        pvm_restart_directory, // no restart func
        sizeof(struct data_area_4_directory),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL,// TODO add DIR flag?
        {0,0}
    },

    {
        ".internal.connection",
        PVM_ROOT_OBJECT_CONNECTION_CLASS,
        IINIT(connection),
        pvm_gc_finalizer_connection,
        pvm_restart_connection,
        sizeof(struct data_area_4_connection),
        PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL|
        PHANTOM_OBJECT_STORAGE_FLAG_IS_FINALIZER,
        {0,0}
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
    int len = strlen(name);
    int i;
    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        if( 0 == strncmp( pvm_internal_classes[i].name, name, len ) )
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

