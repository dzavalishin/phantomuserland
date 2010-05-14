#include <string.h>
#include <phantom_assert.h>
#include <malloc.h>

#include "physalloc.h"

// Physical memory allocator, page by page

// todo dynamic map size, passed in init (for fsck)
// todo pass map ptr from outside, give out
//    macros to produce map size in bytes so that
//    it can be statically allocated (for kernel page maps,
//    which can't be alllocated with malloc!)

// todo if we change memory allocation seriously, change kvtophys()
// and phystokv() as well



static void do_phantom_phys_alloc_init(physalloc_t *arena, u_int32_t n_alloc_units, int n_map_elems, void *mapbuf)
{
    hal_spin_init(&(arena->lock));

    int ie = hal_save_cli();
    hal_spin_lock(&(arena->lock));


    arena->map = mapbuf;

    // Mark everything used.
    // Caller later will free what is supposed to be free
    memset( arena->map, 0xFF, n_map_elems );

    arena->alloc_last_pos = 0;
    arena->n_used_pages = 0;
    arena->total_size = n_alloc_units;
    arena->inited = 1;
    hal_spin_unlock(&(arena->lock));
    if(ie) hal_sti();
}


void phantom_phys_alloc_init_static(physalloc_t *arena, u_int32_t n_alloc_units, void *mapbuf)
{
    int n_map_elems = ((n_alloc_units-1)/8)+1;
    do_phantom_phys_alloc_init(arena, n_alloc_units, n_map_elems, mapbuf);
}


void phantom_phys_alloc_init(physalloc_t *arena, u_int32_t n_alloc_units)
{
    int n_map_elems = ((n_alloc_units-1)/8)+1;
    void *mapbuf = (void *)malloc(n_map_elems);
    do_phantom_phys_alloc_init(arena, n_alloc_units, n_map_elems, mapbuf);
}


errno_t phantom_phys_alloc_page( physalloc_t *arena, physalloc_item_t *ret )
{
    assert(arena->inited);

    int ie = hal_save_cli();
    hal_spin_lock(&(arena->lock));
    int prev_alloc_last_pos = arena->alloc_last_pos;

    do {
        if( ~(arena->map[arena->alloc_last_pos]) )
        {
            //have zero bit

            int page_no = arena->alloc_last_pos*BITS_PER_ELEM;

            map_elem_t elem = arena->map[arena->alloc_last_pos];

            map_elem_t mask = 0x01;
            while( elem & mask )
            {
                mask <<= 1;
                page_no++;
            }

            elem |= mask; // take it

            arena->map[arena->alloc_last_pos] = elem;

            *ret = page_no;
            arena->n_used_pages++;
            hal_spin_unlock(&(arena->lock));
            if(ie) hal_sti();
            return 0;
        }
        arena->alloc_last_pos++;
        if(arena->alloc_last_pos == arena->total_size)
            arena->alloc_last_pos = 0;  //wrap

    } while( arena->alloc_last_pos != prev_alloc_last_pos );

    // not found
    hal_spin_unlock(&(arena->lock));
    if(ie) hal_sti();
    return ENOMEM;
}

void phantom_phys_free_page( physalloc_t *arena, physalloc_item_t free )
{
    assert(arena->inited);

    int elem_no = free/BITS_PER_ELEM;
    int elem_pos = free%BITS_PER_ELEM;

    u_int32_t mask = 0x01 << elem_pos;

    assert( (arena->map[elem_no] & mask) != 0);

    arena->map[elem_no] &= ~mask;
    arena->n_used_pages--;
}


void phantom_phys_free_region( physalloc_t *arena, physalloc_item_t start, size_t n_pages )
{
    assert(arena->inited);

    while(n_pages)
    {
        int elem_no = start/BITS_PER_ELEM;
        int elem_pos = start%BITS_PER_ELEM;

        if( elem_pos == 0 && n_pages > BITS_PER_ELEM )
        {
            assert( ~(arena->map[elem_no]) == 0);

            arena->map[elem_no] = 0;
            n_pages -= BITS_PER_ELEM;
            start += BITS_PER_ELEM;
            continue;
        }

        u_int32_t mask = 0x01 << elem_pos;
        assert( (arena->map[elem_no] & mask) != 0);
        arena->map[elem_no] &= ~mask;

        n_pages--;
        start++;
    }
    arena->n_used_pages -= n_pages;
}


errno_t phantom_phys_alloc_region( physalloc_t *arena, physalloc_item_t *ret, size_t npages )
{
    assert(arena->inited);

    int ie = hal_save_cli();
    hal_spin_lock(&(arena->lock));

    //ATTN: share alloc_last_pos with phantom_phys_alloc_page()...
    int prev_alloc_last_pos = arena->alloc_last_pos;

    assert(npages != 0);
    int elem_no = npages/BITS_PER_ELEM;
    int elem_pos = npages%BITS_PER_ELEM;

    //full elements... -- simplest implementation for now!
    int N = elem_no;
    if (elem_pos > 0) N++;

    int i = 0;
    do {
        if( arena->map[arena->alloc_last_pos] == 0 )  i++;  else i = 0;

        arena->alloc_last_pos++;
        if (arena->alloc_last_pos == arena->total_size) { arena->alloc_last_pos = 0; i = 0;}  //wrap

    } while( arena->alloc_last_pos != prev_alloc_last_pos && i < N );

    if (i != N)
    {
        hal_spin_unlock(&(arena->lock));
        if(ie) hal_sti();
        return ENOMEM;  // not found
    }

    memset( &(arena->map[arena->alloc_last_pos - N]), ~0, elem_no*BITS_PER_ELEM/8 );

    //partial elements:
    map_elem_t elem = arena->map[arena->alloc_last_pos-1];
    map_elem_t mask = 0x01;
    for ( i = 0; i < elem_pos; i++ )
    {
        elem |= mask; // take it
        mask <<= 1;
    }
    arena->map[arena->alloc_last_pos-1] = elem;

    arena->n_used_pages += npages;
    int page_no = (arena->alloc_last_pos - N)*BITS_PER_ELEM;
    *ret = page_no;

    hal_spin_unlock(&(arena->lock));
    if(ie) hal_sti();
    return 0;
}
