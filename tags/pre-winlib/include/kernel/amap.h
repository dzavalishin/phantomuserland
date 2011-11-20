/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Address map style allocator.
 *
 * Represents some addresable space where parts can have different attributes.
 *
**/

#ifndef AMAP_H
#define AMAP_H

#include <queue.h>
#include <errno.h>
#include <phantom_types.h>

/**
 * \ingroup Containers
 * \defgroup Containers Containers - low level data organization
 * @{
**/


typedef u_int64_t amap_elem_addr_t;
typedef u_int64_t amap_elem_size_t;

typedef struct amap_entry {
	amap_elem_addr_t	start;
        amap_elem_size_t	n_elem;
        u_int32_t		flags;

        queue_chain_t   	chain;
} amap_entry_t;


typedef struct amap {
	amap_elem_addr_t	start;
        amap_elem_size_t	n_elem;

        queue_head_t   		queue;
} amap_t;


// Create
void amap_init( amap_t *map, amap_elem_addr_t start, amap_elem_size_t n_elem, u_int32_t start_state_flags );

// Kill
void amap_destroy( amap_t *map );


// Set range to have these attributes
errno_t amap_modify( amap_t *map, amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags );

// Check if range has these attributes. Return 0 if it has.
errno_t amap_check( amap_t *map, amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags );

// Check if range has these attributes.
// Set *modified to nonzero if range had some other attrs.
// Set range to have these attributes after checking.
errno_t amap_check_modify( amap_t *map, amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags, int *modified );


void amap_iterate_all( amap_t *map, void (*f)( amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags, void *arg ), void *arg );
// Only visit entries with given flags
void amap_iterate_flags( amap_t *map, void (*f)( amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags, void *arg ), void *arg, u_int32_t flags );


void amap_dump( amap_t *map );

#endif // AMAP_H
