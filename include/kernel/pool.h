/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Pool of kernel structs accessed with handle.
 *
 **/

#include <hal.h>
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
    // This element was just created
    void *   	(*init)(void *arg);

    hal_mutex_t	mutex;

    pool_arena_t *arenas;
    int 	narenas;

    int 	last_handle; // RR alloc pointer

    int         flag_autodestroy;
    int         flag_nofail; // panics if can't create element
} pool_t;

pool_t *create_pool_ext( int inital_elems, int arena_size );
pool_t *create_pool();
//void destroy_pool(pool_t *);

void *pool_get_el( pool_t *pool, pool_handle_t handle );
void pool_release_el( pool_t *pool, pool_handle_t handle );
void pool_destroy_el( pool_t *pool, pool_handle_t handle );

//! Returns handle or -1 if unable to create element
//! Arg is element itself (if pool->init == 0) or arg to init
pool_handle_t pool_create_el( pool_t *pool, void *arg );


