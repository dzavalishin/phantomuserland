/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Bitmap style allocator. Used for physmem, vaddr, ldt, etc.
 *
**/

#ifndef PHYSALLOC_H
#define PHYSALLOC_H

#include <kernel/page.h>
#include <errno.h>
#include <phantom_types.h>
#include <hal.h>




#define BITS_PER_ELEM (sizeof(map_elem_t)*8)

#define MAP_SIZE_ELEM(items) ( ( ((items)-1) /(sizeof(map_elem_t)*8) ) + 1 )

typedef unsigned int  		physalloc_item_t; // alloc unit no

typedef u_int32_t 		map_elem_t;

struct physalloc
{
    hal_spinlock_t      lock;

    u_int32_t           total_size; // max num of alloc units
    u_int32_t           allocable_size; // max num of allocatable units - is not used by allocator, for stat print only

    /** Bit == 1 if page is used. */
    map_elem_t 		*map; //[MAP_SIZE_ELEM];

    u_int32_t 		alloc_last_pos; // index inside map array
    u_int32_t 		n_used_pages; // number of used mem pages now

    int 		inited;
};

typedef struct physalloc 	physalloc_t;






#define PHYSALLOC_CAN_SLEEP 1 // caller can sleep
#define PHYSALLOC_CAN_FAIL 1 // caller can sleep



void 	phantom_phys_alloc_init_static(physalloc_t *arena, u_int32_t n_alloc_units, void *mapbuf);
void 	phantom_phys_alloc_init(physalloc_t *arena, u_int32_t n_alloc_units );

errno_t phantom_phys_alloc_page( physalloc_t *arena, physalloc_item_t *ret );
void 	phantom_phys_free_page( physalloc_t *arena, physalloc_item_t free );

errno_t phantom_phys_alloc_region( physalloc_t *arena, physalloc_item_t *ret, size_t len );
void 	phantom_phys_free_region( physalloc_t *arena, physalloc_item_t start, size_t n_pages );


// N of free elems
int phantom_phys_free_count( physalloc_t *arena );

int have_lot_of_free_physmem(); // No reclaim really needed
int low_low_free_physmem(); // Really out of physmem
int low_free_physmem(); // Just need some more

#endif // PHYSALLOC_H



