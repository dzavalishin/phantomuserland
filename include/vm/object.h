/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 *
 *
**/

#ifndef PVM_OBJECT_H
#define PVM_OBJECT_H

#include <phantom_types.h>
#include <phantom_libc.h>
#include <limits.h>
#include <errno.h>

//struct pvm_object_storage;

// This structure must be first in any object
// for garbage collector to work ok

#define PVM_OBJECT_START_MARKER 0x7FAA7F55


struct object_PVM_ALLOC_Header
{
    unsigned int                object_start_marker;
    volatile int32_t            refCount; // for fast dealloc of locally-owned objects. If grows to INT_MAX it will be fixed at that value and ignored further. Such objects will be GC'ed in usual way
    unsigned char               alloc_flags;
    unsigned char               gc_flags;
    unsigned int                exact_size; // full object size including this header
};

// This struct is poorly named. In fact, it is an object reference!
struct pvm_object
{
    struct pvm_object_storage	* data;
    struct pvm_object_storage	* interface; // method list is here
};

typedef struct pvm_object pvm_object_t;

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

    struct pvm_object                   _class;
    struct pvm_object                   _satellites; // Points to chain of objects related to this one
    u_int32_t                           _flags; 
    unsigned int                        _da_size; // in bytes!

    unsigned char                       da[];
};

typedef struct pvm_object_storage pvm_object_storage_t;


#define _obj_offsetof(type, field) ((char *)&((type *) 0)->field - (char *) 0)

#define DA_OFFSET() _obj_offsetof(pvm_object_storage_t, da)

//#undef _obj_offsetof

//void pvm_weakref_set_object( struct pvm_object wr, struct pvm_object o );
struct pvm_object pvm_weakref_get_object(struct pvm_object wr );
errno_t si_weakref_9_resetMyObject(struct pvm_object o );


/**
 *
 * Create object. Fixed (in class) or dynamic size.
 *
 * das is data area size in bytes.
 *
**/

struct pvm_object	pvm_create_object(struct pvm_object type);
//struct pvm_object     	pvm_object_create_fixed( struct pvm_object object_class );

// THIS CAN'T BE USED FOR INTERNAL ONES! TODO: Remove or fix to use with internals
struct pvm_object     	pvm_object_create_dynamic( struct pvm_object object_class, int das );


struct data_area_4_thread;


pvm_object_t    pvm_get_class( pvm_object_t o );


/**
 * Lookup class. TODO reimplement! Can block!
 */

struct pvm_object pvm_exec_lookup_class_by_name( struct pvm_object name );

/**
 *
 * Is equal
 *
**/

#define pvm_is_eq( o1, o2 ) ((o1).data = (o2).data )


/**
 *
 * Is null
 *
**/

#define pvm_is_null( o ) ((o).data == 0 || ((o).data == pvm_create_null_object().data))
#define pvm_isnull( o ) ((o).data == 0 || ((o).data == pvm_create_null_object().data))

/**
 *
 * 'object' class is:
 *
 *	- exactly tclass
 *      - tclass or its parent
 *      - tclass or its child
 *
**/

int pvm_object_class_exactly_is( struct pvm_object object, struct pvm_object tclass );
int pvm_object_class_is_or_parent( struct pvm_object object, struct pvm_object tclass );
int pvm_object_class_is_or_child( struct pvm_object object, struct pvm_object tclass );

//#define pvm_class_check(__o,__c) if(!pvm_object_class_is( __o, __c )) pvm_panic("Wrong class");

/**
 *
 * Access object fields.
 * TODO: Check for not internal on read.
 *
**/

struct pvm_object       pvm_get_field( struct pvm_object_storage *, unsigned int no );
struct pvm_object       pvm_get_ofield( struct pvm_object, unsigned int no );
void       		pvm_set_field( struct pvm_object_storage *, unsigned int no, struct pvm_object value );
void       		pvm_set_ofield( struct pvm_object, unsigned int no, struct pvm_object value );

// Need it here? It will be called by usual set field ones...
struct pvm_object  	pvm_get_array_ofield(struct pvm_object_storage *o, unsigned int slot  );
void 			pvm_set_array_ofield(struct pvm_object_storage *o, unsigned int slot, struct pvm_object value );

int                     get_array_size(struct pvm_object_storage *array);
void 			pvm_append_array(struct pvm_object_storage *array, struct pvm_object value_to_append );
void			pvm_pop_array(struct pvm_object_storage *array, struct pvm_object value_to_pop );

// Debug

void                    pvm_object_print( struct pvm_object );
void                    pvm_object_dump( struct pvm_object o );
void                    dumpo( addr_t addr );
void					pvm_puts(struct pvm_object o );
struct pvm_object       pvm_get_class_name( struct pvm_object );

/**
 *
 * Fast access to some basic classes, see pvm_root.c.
 *
**/

struct pvm_object     pvm_get_null_class(void);
struct pvm_object     pvm_get_class_class(void);
struct pvm_object     pvm_get_interface_class(void);
struct pvm_object     pvm_get_code_class(void);
struct pvm_object     pvm_get_int_class(void);
struct pvm_object     pvm_get_long_class(void);
struct pvm_object     pvm_get_float_class(void);
struct pvm_object     pvm_get_double_class(void);
struct pvm_object     pvm_get_string_class(void);
struct pvm_object     pvm_get_array_class(void);
struct pvm_object     pvm_get_page_class(void);
struct pvm_object     pvm_get_thread_class(void);
struct pvm_object     pvm_get_call_frame_class(void);
struct pvm_object     pvm_get_istack_class(void);
struct pvm_object     pvm_get_ostack_class(void);
struct pvm_object     pvm_get_estack_class(void);
struct pvm_object     pvm_get_boot_class(void);
struct pvm_object     pvm_get_binary_class(void);
struct pvm_object     pvm_get_bitmap_class(void);
struct pvm_object     pvm_get_weakref_class(void);
struct pvm_object     pvm_get_window_class(void);
struct pvm_object     pvm_get_directory_class(void);
struct pvm_object     pvm_get_connection_class(void);

struct pvm_object     pvm_get_mutex_class(void);
struct pvm_object     pvm_get_cond_class(void);
struct pvm_object     pvm_get_sema_class(void);


struct pvm_object     pvm_create_null_object(void);
struct pvm_object     pvm_create_class_object(struct pvm_object name, struct pvm_object iface, int da_size);
struct pvm_object     pvm_create_interface_object( int n_methods, struct pvm_object parent_class );
//struct pvm_object     pvm_create_interface_object(void);
struct pvm_object     pvm_create_code_object(int size, void *code);

struct pvm_object     pvm_create_int_object(int value);
struct pvm_object     pvm_create_long_object(int64_t value);
struct pvm_object     pvm_create_float_object(float value);
struct pvm_object     pvm_create_double_object(double value);

struct pvm_object     pvm_create_string_object(const char *value);
struct pvm_object     pvm_create_string_object_binary(const char *value, int length);
struct pvm_object     pvm_create_string_object_binary_cat(
	const char *value1, int n_bytes1,
        const char *value2, int n_bytes2 );

//struct pvm_object     pvm_create_array_object(void);
#define pvm_create_array_object() pvm_create_object( pvm_get_array_class() )
struct pvm_object     pvm_create_page_object( int n_slots, struct pvm_object *init, int init_slots );
struct pvm_object     pvm_create_thread_object( struct pvm_object start_call_frame );
struct pvm_object     pvm_create_call_frame_object(void);
struct pvm_object     pvm_create_istack_object(void);
struct pvm_object     pvm_create_ostack_object(void);
struct pvm_object     pvm_create_estack_object(void);
struct pvm_object     pvm_create_binary_object(int size, void *init);

//struct pvm_object     pvm_create_weakref_object(void);
struct pvm_object     pvm_create_weakref_object(struct pvm_object owned );


struct pvm_object     pvm_create_directory_object(void);
struct pvm_object     pvm_create_connection_object(void);


void     pvm_release_thread_object( struct pvm_object thread );


//static __inline__ struct pvm_object     pvm_get_null_object() { return pvm_create_null_object(); }
#define pvm_get_null_object pvm_create_null_object
#define pvm_get_null pvm_create_null_object
#define pvm_null pvm_create_null_object()

/**
 *
 * This is the most general object creation function. Serves bytecode 'new'.
 * Must call above creators on special cases.
 *
**/


struct pvm_object     pvm_create_object(struct pvm_object type);


/**
 *
 * Create a shallow copy of object.
 *
**/

struct pvm_object     pvm_copy_object(struct pvm_object orig);




void pvm_check_is_thread( struct pvm_object new_thread );



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

