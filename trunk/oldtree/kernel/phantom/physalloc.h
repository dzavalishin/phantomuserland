#ifndef PHYSALLOC_H
#define PHYSALLOC_H

#include <x86/phantom_page.h>
#include <x86/phantom_pmap.h>

#include <errno.h>
#include <phantom_types.h>
#include <hal.h>




#define BITS_PER_ELEM (sizeof(map_elem_t)*8)

#define MAP_SIZE_ELEM(items) ( ( ((items)-1) /(sizeof(map_elem_t)*8) ) + 1 )

typedef int  			physalloc_item_t; // alloc unit no

typedef u_int32_t 		map_elem_t;

struct physalloc
{
    hal_spinlock_t      lock;

    u_int32_t           total_size; // max num of alloc units - NOT USED YET

    /** Bit == 1 if page is used. */
    map_elem_t 		*map; //[MAP_SIZE_ELEM];

    int 		alloc_last_pos;
    int 		n_used_pages; // number of used mem pages now

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



#endif // PHYSALLOC_H



