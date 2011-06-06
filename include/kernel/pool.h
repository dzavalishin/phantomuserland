/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Pool of kernel structs accessed with handle.
 *
 * TODO poosibly use for: timed calls, threads, disks/partitions,
 *      exe_modules, drivers, wttys, windows, file descriptors,
 *      siginfos, processes, io requests, semas, fs instances,
 *      buses, udp/tcp endpoints, cpus, interrupt controllers,
 *      USB hcs and other structs, ports, uusockets, uufiles,
 *      uufses, fonts, bitmaps, virtio devices, trfs stuff,
 *      paging_devices, sound ports
 *      what else?
 *
 * TODO add magic to handle's upper byte to detect correct handle
 *
**/

#include <hal.h>
#include <errno.h>
#include <phantom_types.h>

typedef int pool_handle_t;

struct pool_arena
{
    void **	ptrs;
    int *	refc;
    int 	arena_size; // n elems
    int 	nused;
};

typedef struct pool_arena pool_arena_t;

typedef struct pool
{
    // This element will be deleted now
    void    	(*destroy)(void *pool_elem);
    // This element must be created
    void *   	(*init)(void *arg);

    hal_mutex_t	mutex;

    pool_arena_t *arenas;
    int 	narenas;

    int 	last_handle; // RR alloc pointer

    int         flag_autoclean; // clean pool on pool destroy
    int         flag_autodestroy; // destroy el on refcount == 0
    int         flag_nofail; // panics if smthng wrong, never returns error
} pool_t;

pool_t *create_pool_ext( int inital_elems, int arena_size );
pool_t *create_pool();
errno_t destroy_pool(pool_t *);

//! Inaccurate - no mutex taken
int pool_get_free( pool_t *pool );
//! Inaccurate - no mutex taken
int pool_get_used( pool_t *pool );

errno_t pool_foreach( pool_t *pool, errno_t (*ff)(pool_t *pool, void *el, pool_handle_t handle) );


void *pool_get_el( pool_t *pool, pool_handle_t handle );
errno_t pool_release_el( pool_t *pool, pool_handle_t handle );
errno_t pool_destroy_el( pool_t *pool, pool_handle_t handle );

//! Returns handle or -1 if unable to create element
//! Arg is element itself (if pool->init == 0) or arg to init
pool_handle_t pool_create_el( pool_t *pool, void *arg );


