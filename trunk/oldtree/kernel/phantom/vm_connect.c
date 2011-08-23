/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Virtual machine threads handler.
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

#include <vm/syscall.h>

#include <vm/khandle.h>

#include <khash.h>

//#include <time.h>

//#include "snap_sync.h"
//#include <hal.h>

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


    return 0;
}

errno_t  object_revoke_handle( ko_handle_t h, pvm_object_t o );


// These two work after handle is assigned
errno_t  object2handle( ko_handle_t *h, pvm_object_t o );
errno_t  handle2object( pvm_object_t *o, ko_handle_t h );



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

