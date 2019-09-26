/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Phantom Virtual Machine - object representation and basic operations.
 *
 *
**/

#ifndef PVM_OBJECT_H
#define PVM_OBJECT_H

#include <phantom_types.h>
#include <phantom_libc.h>
#include <limits.h>
#include <errno.h>

// This structure must be first in any object
// for garbage collector to work ok

#define PVM_OBJECT_START_MARKER 0x7FAA7F55

// TODO add two bytes after flags to assure alignment
struct object_PVM_ALLOC_Header
{
    unsigned int                object_start_marker;
    volatile int32_t            refCount; // for fast dealloc of locally-owned objects. If grows to INT_MAX it will be fixed at that value and ignored further. Such objects will be GC'ed in usual way
    unsigned char               alloc_flags;
    unsigned char               gc_flags;
    unsigned int                exact_size; // full object size including this header
};

struct pvm_object_storage;

// This is object itself.
//
//   	_ah is allocation header, used by allocator/gc
//   	_class is class object reference.
//      _satellites is used to keep some related things such as weak ptr backlink
//      _flags used to keep some shortcut info about object type
//      _da_size is n of bytes in da[]
//      da[] is object contents
//
// NB! See JIT assembly hardcode for object structure offsets
struct pvm_object_storage
{
    struct object_PVM_ALLOC_Header      _ah;

    struct pvm_object_storage *         _class;
    struct pvm_object_storage *         _satellites; // Points to chain of objects related to this one
    u_int32_t                           _flags; 
    unsigned int                        _da_size; // in bytes!

    unsigned char                       da[];
};

typedef struct pvm_object_storage pvm_object_storage_t;

/*
// This struct is poorly named. In fact, it is an object reference!
struct pvm_object
{
    struct pvm_object_storage	* data;
    struct pvm_object_storage	* interface; // method list is here
};*/

typedef struct pvm_object_storage * pvm_object_t;


#define _obj_offsetof(type, field) ((char *)&((type *) 0)->field - (char *) 0)

#define DA_OFFSET() _obj_offsetof(pvm_object_storage_t, da)

//#undef _obj_offsetof

//void pvm_weakref_set_object( pvm_object_t wr, pvm_object_t o );
pvm_object_t pvm_weakref_get_object(pvm_object_t wr );
errno_t si_weakref_9_resetMyObject(pvm_object_t o );


/**
 *
 * Create object. Fixed (in class) or dynamic size.
 *
 * das is data area size in bytes.
 *
**/

pvm_object_t 	pvm_create_object(pvm_object_t type);
//pvm_object_t     	pvm_object_create_fixed( pvm_object_t object_class );

// THIS CAN'T BE USED FOR INTERNAL ONES! TODO: Remove or fix to use with internals
pvm_object_t     	pvm_object_create_dynamic( pvm_object_t object_class, int das );


struct data_area_4_thread;


pvm_object_t    pvm_get_class( pvm_object_t o );


/**
 * Lookup class. TODO reimplement! Can block!
 */

pvm_object_t pvm_exec_lookup_class_by_name( pvm_object_t name );

/**
 *
 * Is equal
 *
**/

#define pvm_is_eq( o1, o2 ) ((o1) == (o2) )


/**
 *
 * Is null
 *
**/

#define pvm_is_null( o ) ((o) == 0 || ((o) == pvm_create_null_object()))
#define pvm_isnull( o ) ((o) == 0 || ((o) == pvm_create_null_object()))

/**
 *
 * 'object' class is:
 *
 *	- exactly tclass
 *      - tclass or its parent
 *      - tclass or its child
 *
**/

int pvm_object_class_exactly_is( pvm_object_t object, pvm_object_t tclass );
int pvm_object_class_is_or_parent( pvm_object_t object, pvm_object_t tclass );
int pvm_object_class_is_or_child( pvm_object_t object, pvm_object_t tclass );

//#define pvm_class_check(__o,__c) if(!pvm_object_class_is( __o, __c )) pvm_panic("Wrong class");

/**
 *
 * Access object fields.
 * TODO: Check for not internal on read.
 *
**/

pvm_object_t     pvm_get_field( pvm_object_t , unsigned int no );
pvm_object_t     pvm_get_ofield( pvm_object_t , unsigned int no );
void                  pvm_set_field( pvm_object_t , unsigned int no, pvm_object_t value );
void                  pvm_set_ofield( pvm_object_t , unsigned int no, pvm_object_t value );

// Need it here? It will be called by usual set field ones...
pvm_object_t     pvm_get_array_ofield(pvm_object_t o, unsigned int slot  );
void                  pvm_set_array_ofield(pvm_object_t o, unsigned int slot, pvm_object_t value );

int                   get_array_size(pvm_object_t array);
void                  pvm_append_array(pvm_object_t array, pvm_object_t value_to_append );
void                  pvm_pop_array(pvm_object_t array, pvm_object_t value_to_pop );

// Debug

void                  pvm_object_print( pvm_object_t );
void                  pvm_object_dump( pvm_object_t o );
void                  dumpo( addr_t addr );
void                  pvm_puts(pvm_object_t o );
pvm_object_t     pvm_get_class_name( pvm_object_t );

/**
 *
 * Fast access to some basic classes, see pvm_root.c.
 *
**/

pvm_object_t     pvm_get_null_class(void);
pvm_object_t     pvm_get_class_class(void);
pvm_object_t     pvm_get_interface_class(void);
pvm_object_t     pvm_get_code_class(void);
pvm_object_t     pvm_get_int_class(void);
pvm_object_t     pvm_get_long_class(void);
pvm_object_t     pvm_get_float_class(void);
pvm_object_t     pvm_get_double_class(void);
pvm_object_t     pvm_get_string_class(void);
pvm_object_t     pvm_get_array_class(void);
pvm_object_t     pvm_get_page_class(void);
pvm_object_t     pvm_get_thread_class(void);
pvm_object_t     pvm_get_call_frame_class(void);
pvm_object_t     pvm_get_istack_class(void);
pvm_object_t     pvm_get_ostack_class(void);
pvm_object_t     pvm_get_estack_class(void);
pvm_object_t     pvm_get_boot_class(void);
pvm_object_t     pvm_get_binary_class(void);
pvm_object_t     pvm_get_bitmap_class(void);
pvm_object_t     pvm_get_weakref_class(void);
pvm_object_t     pvm_get_window_class(void);
pvm_object_t     pvm_get_directory_class(void);
pvm_object_t     pvm_get_connection_class(void);

pvm_object_t     pvm_get_mutex_class(void);
pvm_object_t     pvm_get_cond_class(void);
pvm_object_t     pvm_get_sema_class(void);


pvm_object_t     pvm_create_null_object(void);
pvm_object_t     pvm_create_class_object(pvm_object_t name, pvm_object_t iface, int da_size);
pvm_object_t     pvm_create_interface_object( int n_methods, pvm_object_t parent_class );
//pvm_object_t     pvm_create_interface_object(void);
pvm_object_t     pvm_create_code_object(int size, void *code);

pvm_object_t     pvm_create_int_object(int value);
pvm_object_t     pvm_create_long_object(int64_t value);
pvm_object_t     pvm_create_float_object(float value);
pvm_object_t     pvm_create_double_object(double value);

pvm_object_t     pvm_create_string_object(const char *value);
pvm_object_t     pvm_create_string_object_binary(const char *value, int length);
pvm_object_t     pvm_create_string_object_binary_cat(
        const char *value1, int n_bytes1,
        const char *value2, int n_bytes2 );

//pvm_object_t     pvm_create_array_object(void);
#define pvm_create_array_object() pvm_create_object( pvm_get_array_class() )
pvm_object_t     pvm_create_page_object( int n_slots, pvm_object_t *init, int init_slots );
pvm_object_t     pvm_create_thread_object( pvm_object_t start_call_frame );
pvm_object_t     pvm_create_call_frame_object(void);
pvm_object_t     pvm_create_istack_object(void);
pvm_object_t     pvm_create_ostack_object(void);
pvm_object_t     pvm_create_estack_object(void);
pvm_object_t     pvm_create_binary_object(int size, void *init);

//pvm_object_t     pvm_create_weakref_object(void);
pvm_object_t     pvm_create_weakref_object(pvm_object_t owned );


pvm_object_t     pvm_create_directory_object(void);
pvm_object_t     pvm_create_connection_object(void);


void     pvm_release_thread_object( pvm_object_t thread );


//static __inline__ pvm_object_t     pvm_get_null_object() { return pvm_create_null_object(); }
#define pvm_get_null_object pvm_create_null_object
#define pvm_get_null pvm_create_null_object
#define pvm_null pvm_create_null_object()

/**
 *
 * This is the most general object creation function. Serves bytecode 'new'.
 * Must call above creators on special cases.
 *
**/


pvm_object_t     pvm_create_object(pvm_object_t type);


/**
 *
 * Create a shallow copy of object.
 *
**/

//pvm_object_t     pvm_copy_object(pvm_object_t orig);




void pvm_check_is_thread( pvm_object_t new_thread );



#define pvm_get_default_interface(os) \
    ({  \
    struct data_area_4_class *da = pvm_object_da( os->_class, class );  \
    da->object_default_interface; \
    })

#define pvm_is_internal_class(co) \
    ({  \
    struct data_area_4_class *da = pvm_object_da( co, class );  \
    da->object_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL; \
    })


#endif // PVM_OBJECT_H

