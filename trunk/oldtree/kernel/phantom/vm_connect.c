/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Virtual machine objects interface.
 *
**/

#define DEBUG_MSG_PREFIX "vmconnect"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


//#include <kernel/config.h>

#include <phantom_libc.h>

#include <vm/object.h>
#include <vm/exec.h>
#include <vm/internal_da.h>
#include <vm/root.h>
#include <vm/p2c.h>
#include <vm/reflect.h>
#include <vm/alloc.h>

#include <vm/syscall.h>

#include <vm/khandle.h>

#include <khash.h>


/**
 *
 * TODO
 *
 * Kernel must use pool to access objects. Kernel will then keep its own volatile
 * object ref count (with the help of pool), and object's own ref count will be incremented
 * just by one so that if kernel crashes we will decrement it on next restart and delete
 * object, if it was used just by kernel. Got it? If no, ask dz. :)
 *
**/



// -----------------------------------------------------------------------
// Object handles support
// -----------------------------------------------------------------------

struct kohandle_entry {
    struct kohandle_entry *	next;
    pvm_object_t 		o;
    //hal_mutex_t lock;
    //int ref_count;
};

typedef struct kohandle_entry kohandle_entry_t;

static int kohandle_entry_compare_func(void *_e, const void *_key)
{
    kohandle_entry_t *e = _e;
    const pvm_object_t *o = _key;

    return ((e->o.data == o->data) && (e->o.interface == o->interface)) ? 0 : 1;
}

static unsigned int kohandle_entry_hash_func(void *_e, const void *_key, unsigned int range)
{
    kohandle_entry_t *e = _e;
    const pvm_object_t *o = _key;

    if(e)
        return ((addr_t)e->o.data) % range;
    else
        return ((addr_t)o->data) % range;
}

static kohandle_entry_t *kohandles;
static hal_mutex_t      kohandles_lock;

errno_t object_handles_init()
{
    kohandles = hash_init(256, offsetof(kohandle_entry_t, next),
                          &kohandle_entry_compare_func,
                          &kohandle_entry_hash_func);
    hal_mutex_init( &kohandles_lock, "kohandles" );
    return 0;
}

errno_t  object_assign_handle( ko_handle_t *h, pvm_object_t o )
{
    kohandle_entry_t *e;

    hal_mutex_lock(&kohandles_lock);
    e = hash_lookup(kohandles, &o);
    //if(e)         udp_endpoint_acquire_ref(e);
    mutex_unlock(&kohandles_lock);

    if(!e)
        return ENOENT;

    // it is possible to get two objects with same data and different iface
    //*h = o.data; // that simple - just use data address as handle
    SHOW_ERROR0( 0, "Implement me" );
    (void) h;


    return 0;
}

errno_t  object_revoke_handle( ko_handle_t h, pvm_object_t o )
{
    (void) o;
    (void) h;
    SHOW_ERROR0( 0, "Implement me" );
    return EFAULT;
}

errno_t  handle_release_object( ko_handle_t *h )
{
    (void) h;
    SHOW_ERROR0( 0, "Implement me" );
    return EFAULT;
}


// These two work after handle is assigned
errno_t  object2handle( ko_handle_t *h, pvm_object_t o )
{
    (void) o;
    (void) h;
    SHOW_ERROR0( 0, "Implement me" );
    return EFAULT;
}

errno_t  handle2object( pvm_object_t *o, ko_handle_t h )
{
    (void) o;
    (void) h;
    SHOW_ERROR0( 0, "Implement me" );
    return EFAULT;
}


// -----------------------------------------------------------------------
// General object land interface
// -----------------------------------------------------------------------

errno_t ko_make_int( ko_handle_t *ret, int i )
{
    return object2handle( ret, pvm_create_int_object(i) );
}

errno_t ko_make_string( ko_handle_t *ret, const char *s )
{
    return object2handle( ret, pvm_create_string_object(s) );
}

errno_t ko_make_string_binary( ko_handle_t *ret, const char *s, int len )
{
    return object2handle( ret, pvm_create_string_object_binary(s, len) );
}

errno_t ko_make_object( ko_handle_t *ret, ko_handle_t ko_class, void *init ) // C'tor args? Or call it manually?
{
    assert(init == 0); // not impl

    pvm_object_t co;

    errno_t rc = handle2object( &co, ko_class );
    if( rc )
    {
        SHOW_ERROR( 5, "Class not mapped to handle %x", ko_class );
        return rc;
    }

    return object2handle( ret, pvm_create_object(co) );
}


errno_t ko_get_class( ko_handle_t *ret, const char *class_name )
{
    struct pvm_object cname = pvm_create_string_object(class_name);
    errno_t rc = object2handle( ret, pvm_exec_lookup_class_by_name( cname ) );
    ref_dec_o( cname );

    return rc;
}

errno_t ko_map_method( int *out_ord, ko_handle_t *ko_this, const char *method_name )
{
    pvm_object_t _this;

    errno_t rc = handle2object( &_this, *ko_this );
    if( rc )
    {
        SHOW_ERROR( 5, "Object not mapped to handle %x", ko_this );
        return rc;
    }

    handle_release_object( ko_this );

    pvm_object_t tclass = _this.data->_class;

    // TODO XXX VERY ineffective!
    int ord = 0;
    while( ord < 256 )
    {
        pvm_object_t mn = pvm_get_method_name( tclass, ord );
        if( pvm_is_null(mn) )
            return ENOENT;

        if(EQ_STRING_P2C(mn,method_name))
        {
            *out_ord = ord;
            return 0;
        }
    }

    return ENOENT;
}

//! Async call - void, does not wait
void ko_spawn_method( ko_handle_t *ko_this, int method, int nargs, ko_handle_t *args, int flags );
//! Async call - void, does not wait, calls calback
void ko_spawn_method_callback( ko_handle_t *ko_this, int method, int nargs, ko_handle_t *args, int flags, void (*callback)( void *cb_arg, ko_handle_t *ret ), void *cb_arg );


//#define KO_CALL_FLAG_NEW_THREAD (1<<0) - no, spawn instead

// -----------------------------------------------------------------------
// Connection object support
// -----------------------------------------------------------------------


errno_t phantom_connect_object( struct data_area_4_connection *da, struct data_area_4_thread *tc)
{
    const char *name = da->name;
    da->owner = tc;

    return ENOMEM; 
}

errno_t phantom_disconnect_object( struct data_area_4_connection *da, struct data_area_4_thread *tc) 
{
    errno_t ret = 0;

    if( da->kernel == 0 )
    {
        return ENXIO;
    }

    if( da->kernel->disconnect )
    {
        ret = da->kernel->disconnect(da,tc);
    }

    da->kernel = 0;

    return ret;
}

