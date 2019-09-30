/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * \brief
 * Pool of kernel structs accessed with handle.
 *
 * \todo TODO poosibly use for:
 *      timed calls,
 *      threads                 - already handle-based
 *      disks/partitions,
 *      exe_modules,            - TODO unix/exec.c
 *      drivers,
 *      wttys,
 *      file descriptors,
 *      siginfos,
 *      processes,              - already handle-based
 *      io requests,
 *      semas,
 *      fs instances,
 *      buses,
 *      udp/tcp endpoints,
 *      cpus,
 *      interrupt controllers,
 *      USB hcs and other structs,
 *      ports,
 *      uusockets,
 *      uufiles,
 *      uufses,
 *      virtio devices,
 *      trfs stuff,
 *      paging_devices,
 *      sound ports,
 *      fonts,
 *      bitmaps,
 *      windows                 - in progress
 *      what else?
 *
**/

#ifndef POOL_H
#define POOL_H

#include <hal.h>
#include <errno.h>
#include <phantom_types.h>

#if CONF_POOL_SPIN
#  include <spinlock.h>
#endif


#define INVALID_POOL_HANDLE -1

/**
 * \ingroup Containers
 * @{
**/

/** Pool item handle */
typedef int pool_handle_t;

struct pool_arena
{
    void **	ptrs;
    int *	refc;
    int 	arena_size; // n elems
    int 	nused;
};

typedef struct pool_arena pool_arena_t;

//! \brief Pool itself
typedef struct pool
{
    // This element will be deleted now
    void    	(*destroy)(void *pool_elem);
    // This element must be created
    void *   	(*init)(void *arg);

#if CONF_POOL_SPIN
    hal_spinlock_t	lock;
#else
    hal_mutex_t	mutex;
#endif

    pool_arena_t *arenas;
    int         narenas;

    int         magic;          // pool magic id - to check that handle is for this pool

    int         last_handle; // RR alloc pointer

    int         flag_autoclean; // clean pool on pool destroy
    int         flag_autodestroy; // destroy el on refcount == 0
    int         flag_nofail; // panics if smthng wrong, never returns error
} pool_t;

pool_t *create_pool_ext( int inital_elems, int arena_size );
//! \brief Create pool of default size
pool_t *create_pool();
errno_t destroy_pool(pool_t *);

//! Inaccurate - no mutex taken
int pool_get_free( pool_t *pool );
//! Inaccurate - no mutex taken
int pool_get_used( pool_t *pool );


errno_t pool_foreach( pool_t *pool, errno_t (*ff)(pool_t *pool, void *el, pool_handle_t handle, void *arg), void *arg );


//! Increase refcount
void *pool_get_el( pool_t *pool, pool_handle_t handle );
//! Decrease refcount
errno_t pool_release_el( pool_t *pool, pool_handle_t handle );
errno_t pool_destroy_el( pool_t *pool, pool_handle_t handle );

//! Inaccurate - no mutex taken
int pool_el_refcount( pool_t *pool, pool_handle_t handle );

//! Returns handle or -1 if unable to create element
//! Arg is element itself (if pool->init == 0) or arg to init
pool_handle_t pool_create_el( pool_t *pool, void *arg );

//! Do an action with pool element, does not destroy pool el in any case
errno_t do_pool_forone( pool_t *pool, pool_handle_t handle, errno_t (*ff)(pool_t *pool, void *el, void *arg), void *arg );


#endif // POOL_H

