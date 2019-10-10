/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Page-level bitmap based allocator.
 *
**/

#include <string.h>
#include <stdio.h>
#include <phantom_assert.h>
#include <malloc.h>

#include <kernel/physalloc.h>
#include <kernel/debug.h>
#include <kernel/debug_graphical.h>

// Physical memory allocator, page by page

// TODO if we change memory allocation seriously, change kvtophys()
// and phystokv() as well

#define TRACE_USED 0


long phantom_phys_stat_arena( physalloc_t *arena );

#if TRACE_USED
void physalloc_check_diff( physalloc_t *arena, long prev, int tochange )
{
    long now = phantom_phys_stat_arena( arena );
    if( (now - prev) != tochange )
        lprintf("phys diff old %ld new %ld diff %ld, expect diff %d\n",
               prev, now,
               now - prev,
               tochange
              );
}
#endif


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
    arena->n_used_pages = n_alloc_units;
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

    hal_spin_lock_cli(&(arena->lock));

#if TRACE_USED
    long prev_used = phantom_phys_stat_arena( arena );
#endif

    unsigned int prev_alloc_last_pos = arena->alloc_last_pos;

    assert(arena->alloc_last_pos < arena->total_size / BITS_PER_ELEM);

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
#if TRACE_USED
            physalloc_check_diff( arena, prev_used, 1 );
#endif
            hal_spin_unlock_sti(&(arena->lock));

            return 0;
        }
        arena->alloc_last_pos++;
        if(arena->alloc_last_pos >= arena->total_size / BITS_PER_ELEM)
            arena->alloc_last_pos = 0;  //wrap

    } while( arena->alloc_last_pos != prev_alloc_last_pos );

    // not found
    hal_spin_unlock_sti(&(arena->lock));
    return ENOMEM;
}

void phantom_phys_free_page( physalloc_t *arena, physalloc_item_t free )
{
    assert(arena->inited);
#if TRACE_USED
    hal_spin_lock_cli(&(arena->lock));
    long prev_used = phantom_phys_stat_arena( arena );
#endif
    //assert(free >= 0 && free < arena->total_size);
    assert(free < arena->total_size);

    int elem_no = free/BITS_PER_ELEM;
    int elem_pos = free%BITS_PER_ELEM;

    map_elem_t mask = 0x01 << elem_pos;

    assert( (arena->map[elem_no] & mask) != 0);

    arena->map[elem_no] &= ~mask;
    arena->n_used_pages--;
#if TRACE_USED
    physalloc_check_diff( arena, prev_used, -1 );
    hal_spin_unlock_sti(&(arena->lock));
#endif
}


void phantom_phys_free_region( physalloc_t *arena, physalloc_item_t start, size_t _n_pages )
{
    size_t n_pages = _n_pages;
    assert(arena->inited);
    //assert(start >= 0 &&
    /*assert( start < arena->total_size &&
            n_pages < arena->total_size &&
            start + n_pages < arena->total_size);*/

    assert( start < arena->total_size );
    assert( n_pages < arena->total_size );
    assert( start + n_pages < arena->total_size ); // TODO bug? <= ?

#if TRACE_USED
    hal_spin_lock_cli(&(arena->lock));
    long prev_used = phantom_phys_stat_arena( arena );
#endif

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
            arena->n_used_pages -= BITS_PER_ELEM;
            continue;
        }

        map_elem_t mask = 0x01 << elem_pos;
        assert( (arena->map[elem_no] & mask) != 0);
        arena->map[elem_no] &= ~mask;

        n_pages--;
        arena->n_used_pages--;
        start++;
    }
    //arena->n_used_pages -= n_pages;
    assert(n_pages == 0);

#if TRACE_USED
    physalloc_check_diff( arena, prev_used, -_n_pages );
    hal_spin_unlock_sti(&(arena->lock));
#endif
}


errno_t phantom_phys_alloc_region( physalloc_t *arena, physalloc_item_t *ret, size_t npages )
{
    assert(arena->inited);

    hal_spin_lock_cli(&(arena->lock));

#if TRACE_USED
    long prev_used = phantom_phys_stat_arena( arena );
#endif

    //ATTN: share alloc_last_pos with phantom_phys_alloc_page()...
    unsigned int prev_alloc_last_pos = arena->alloc_last_pos;
    assert(arena->alloc_last_pos < arena->total_size / BITS_PER_ELEM);

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
        if (arena->alloc_last_pos >= arena->total_size / BITS_PER_ELEM)
        { arena->alloc_last_pos = 0; i = 0;}  //wrap

    } while( arena->alloc_last_pos != prev_alloc_last_pos && i < N );

    if (i != N)
    {
        hal_spin_unlock_sti(&(arena->lock));
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

#if TRACE_USED
    physalloc_check_diff( arena, prev_used, npages );
#endif

    hal_spin_unlock_sti(&(arena->lock));
    return 0;
}


#include <sys/libkern.h>
#include <stdio.h>

long phantom_phys_stat_arena( physalloc_t *arena )
{
    assert(arena->inited);

    unsigned long used_bits = 0;
    long longest_free = 0;
    long curr_free = 0;

    int i;
    int last = arena->total_size / BITS_PER_ELEM;

    for( i = 0; i < last; i++ )
    {
        map_elem_t e = arena->map[i];

        if( e )
        {
            longest_free = umax( longest_free, curr_free );
            curr_free = 0;
        }
        else
            curr_free++;

        if( e == ~0u )
            used_bits += BITS_PER_ELEM;
        else if( e )
        {
            unsigned j;
            map_elem_t mask = 0x01;
            for ( j = 0; j < BITS_PER_ELEM; j++ )
            {
                if( e & mask )
                    used_bits++;
                mask <<= 1;
            }
        }
    }

    //lprintf("used %d, realy %d, longest free %d\n", arena->n_used_pages, used_bits, longest_free );

    //if( used_bits != arena->n_used_pages ) // always differs, for part of map corresponding to non-existant memory is marked as used
    //lprintf("used diff @%p: arena->n_used_pages %d, realy %d, diff %d, longest free %d\n", arena, arena->n_used_pages, used_bits, arena->n_used_pages - used_bits, longest_free );

    return used_bits;
}

#include <video/window.h>

static rgba_t calc_pixel_color( physalloc_t *m, int elem, int units_per_pixel );

/**
 * 
 * \brief Generic painter for any allocator using us.
 * 
 * Used in debug window.
 * 
 * \param[in] w Window to draw to
 * \param[in] r Rectangle to paint inside
 * \param[in] m Memory map (allocator state) to paint state of
 * 
**/
void paint_allocator_memory_map(window_handle_t w, rect_t *r, physalloc_t *m )
{
    int pixels = r->xsize * r->ysize;
    int units_per_pixel =  1 + ((m->total_size-1) / pixels);

    int bits_per_elem = sizeof(map_elem_t) * 8;

    // make pixel to correspond to integer number of map_elem_t
    units_per_pixel = (((units_per_pixel-1) / bits_per_elem) + 1) * bits_per_elem;

    int x, y;
    for( y = 0; y < r->ysize; y++ )
    {
        for( x = 0; x < r->xsize; x++ )
            w_draw_pixel( w, x + r->x, y + r->y, calc_pixel_color( m, x + y * r->xsize, units_per_pixel ));
    }

}

static rgba_t calc_pixel_color( physalloc_t *m, int elem, int units_per_pixel )
{
    map_elem_t *ep = m->map + elem;

    int state = 0; // 0 = empty, 1 = partial, 2 = used
    int bits = 0;

    int i;
    for( i = 0; i < units_per_pixel; i++, ep++ )
    {
        if( 0 == *ep ) continue; // empty, no change
        if( 0 == ~*ep ) state = 2; // full

        // partial, if was empty - make partial
        if( state == 0 ) state = 1;

        bits += __builtin_popcount (*ep);
    }

    switch(state)
    {
        case 0: return COLOR_BLUE;

        case 1: 
        {
            //return COLOR_LIGHTGREEN;
            rgba_t c = COLOR_LIGHTGREEN;
            // lighter = less used
            c.g = 0xFF - (bits * 0xFF / (units_per_pixel * sizeof(map_elem_t) * 8));
            return c;
        }

        case 2: return COLOR_LIGHTRED;

        default: return COLOR_YELLOW;
    }
}
