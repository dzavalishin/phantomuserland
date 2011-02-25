/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Address space range based allocator.
 *
**/

#include <kernel/amap.h>
#include <malloc.h>
#include <phantom_assert.h>
#include <phantom_libc.h>


static amap_entry_t * new_entry( amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags )
{
    amap_entry_t *ret = (amap_entry_t *)malloc(sizeof(amap_entry_t));
    if(ret == 0) panic("out of mem in amap");

    ret->start = from;
    ret->n_elem = n_elem;
    ret->flags = flags;

    return ret;
}

// Join entry with next
static void join( amap_t *map, amap_entry_t *e )
{
    assert( e->chain.next != &(map->queue) );
    amap_entry_t *ne = (amap_entry_t *)e->chain.next;

    e->n_elem += ne->n_elem;
    queue_remove(&(map->queue), ne, amap_entry_t *, chain);

    free(ne);
}

// Split entry so that first entry after split will have n_elem_in_first elems
// Returns new (next) elem, e is not removed but chanegd inplace
static amap_entry_t * split( amap_t *map, amap_entry_t *e, amap_elem_size_t n_elem_in_first )
{
    assert(e->n_elem > n_elem_in_first);
    assert(n_elem_in_first > 0);

    amap_entry_t *ne = new_entry( e->start+n_elem_in_first, e->n_elem-n_elem_in_first, e->flags );
    e->n_elem = n_elem_in_first;
    queue_enter_after(&(map->queue), e, ne, amap_entry_t *, chain);
    return ne;
}


static amap_entry_t *
find( amap_t *map, amap_elem_addr_t from )
{
    amap_entry_t *ie;
    queue_iterate(&(map->queue), ie, amap_entry_t *, chain)
    {
        if( from >= ie->start && from < ie->start+ie->n_elem)
            return ie;
    }

    return 0;
}


#if 0
static int a_covers_b(
                      amap_elem_addr_t a_start, amap_elem_size_t a_n_elem,
                      amap_elem_addr_t b_start, amap_elem_size_t b_n_elem
                     )
{
    return
        (a_start <= b_start) &&
        ( (a_start+a_n_elem) >= (b_start+b_n_elem) );
}
#endif










void
amap_init( amap_t *map, amap_elem_addr_t start, amap_elem_size_t n_elem, u_int32_t start_state_flags )
{
    map->start 		= start;
    map->n_elem         = n_elem;

    queue_init(&(map->queue));

    amap_entry_t *ne = new_entry( start, n_elem, start_state_flags );
    queue_enter(&(map->queue), ne, amap_entry_t *, chain);
}

void
amap_destroy( amap_t *map )
{
    amap_entry_t *ie;
    while( !queue_empty(&(map->queue)) )
    {
        queue_remove_first(&(map->queue), ie, amap_entry_t *, chain);
        free(ie);
    }
}



void
amap_iterate_all( amap_t *map, void (*f)( amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags, void *arg ), void *arg )
{
    amap_entry_t *ie;
    queue_iterate(&(map->queue), ie, amap_entry_t *, chain)
    {
        f(ie->start, ie->n_elem, ie->flags, arg);
    }
}

void
amap_iterate_flags( amap_t *map, void (*f)( amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags, void *arg ), void *arg, u_int32_t flags )
{
    amap_entry_t *ie;
    queue_iterate(&(map->queue), ie, amap_entry_t *, chain)
    {
        if( ie->flags == flags )
            f(ie->start, ie->n_elem, ie->flags, arg);
    }
}



// Check if range has these attributes. Return 0 if it has.
errno_t
amap_check( amap_t *map, amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags )
{
    if( from >= map->start+map->n_elem )
        return E2BIG;

    if( map->start >= from+n_elem )
        return E2BIG;

    if( from < map->start )
        return E2BIG;

    if( map->start+map->n_elem < from+n_elem )
        return E2BIG;

    amap_entry_t *e = find( map, from );

    do
    {
        if(e == 0) return ENOENT;

        if(e->flags != flags ) return EFTYPE;

        // Ok, this el is good, exclude its range from requested range

        amap_elem_addr_t finish = from+n_elem;

        from = e->start + e->n_elem; // Start where elem ended
        n_elem = finish-from;

        if(n_elem <= 0)
            break;

        e = (amap_entry_t *)e->chain.next;
        if( (void *)e == &(map->queue) )
        {
            return EFBIG;
        }

    } while(n_elem > 0);

    return 0;
}


static errno_t
rejoin_range( amap_t *map, amap_elem_addr_t from, amap_elem_size_t n_elem )
{
    if( from >= map->start+map->n_elem )
        return E2BIG;

    if( map->start >= from+n_elem )
        return E2BIG;

    if( from < map->start )
        return E2BIG;

    if( map->start+map->n_elem < from+n_elem )
        return E2BIG;

    amap_entry_t *e = find( map, from );
    if( e == 0 )
        panic("amap - no elem within map bounds");

    amap_elem_addr_t finish = from+n_elem; // One AFTER our range


    do {

        amap_entry_t *nexte = (amap_entry_t *)e->chain.next;
        if( (void *)nexte == &(map->queue) )
            panic("amap - fall out of map bounds");

        if( e->flags != nexte->flags )
        {
            e = nexte;
            continue;
        }

        join( map, e );

    } while(e->start < finish);

    return 0;
}


void
amap_dump( amap_t *map )
{

    amap_entry_t *ie;
    queue_iterate(&(map->queue), ie, amap_entry_t *, chain)
    {
        /*
        printf(
               "[0x%08X - 0x%08X[ : 0x%4X\n",
               (int)ie->start, (int)ie->start+ie->n_elem, ie->flags
              );
        */
        printf(
               "[0x%09qX - 0x%09qX[ : 0x%4X\n",
               ie->start, ie->start+ie->n_elem, ie->flags
              );
    }

}





errno_t
amap_modify( amap_t *map, amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags )
{
    return amap_check_modify( map, from, n_elem, flags, 0 );
}




// Set range to have these attributes
errno_t
amap_check_modify( amap_t *map, amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags, int *modified )
{
    if(modified) *modified = 0;

    if( from >= map->start+map->n_elem )
        return E2BIG;

    if( map->start >= from+n_elem )
        return E2BIG;

    if( from < map->start )
        return E2BIG;

    if( map->start+map->n_elem < from+n_elem )
        return E2BIG;

    amap_entry_t *e = find( map, from );
    if( e == 0 )
        panic("amap - no elem within map bounds");

    amap_elem_addr_t finish = from+n_elem; // One AFTER our range

    do {
        if(e->flags == flags )
        {
            // Maybe partial, but has good flags
            goto next;
        }
        else
        {
            if(modified) *modified = 1;
        }

        amap_elem_addr_t e_finish = e->start+e->n_elem; // One AFTER el end

        // Completely within
        if( e->start >= from && e_finish <= finish )
        {
            e->flags = flags;
            goto next;
        }


        // Partial at beginning
        if( e->start < from )
        {
            // Just split and let next iteration to do the rest
            amap_entry_t * ne = split( map, e, from - e->start );
            e = ne;
            continue;
        }

        // Partial at end
        {
            assert(e_finish > finish);
            // Just split and let next iteration to do the rest
            split( map, e, finish - e->start );
            continue;
        }

    next:

        if( e_finish == finish )
            break;

        e = (amap_entry_t *)e->chain.next;
        if( (void *)e == &(map->queue) )
            panic("amap - fall out of map bounds");

    } while(e->start < finish);

    amap_elem_addr_t e_finish = e->start+e->n_elem;

    // We can finish later 'cause last elem can be bigger than our interval and have good flags
    //if(e_finish != finish)        panic("AMAP - finished not on exact end");

    if(e_finish < finish)
        panic("AMAP - finished before");


    rejoin_range( map, from, n_elem );

    return 0;
}

