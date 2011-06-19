/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Disk IO cache - supposed to be used by filesystems code.
 * Meaningless for native Phantom VM IO, of course.
 *
**/

#define DEBUG_MSG_PREFIX "DiskCache"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <disk.h>
#include <phantom_disk.h>
#include <assert.h>
#include <malloc.h>
#include <phantom_libc.h>
#include <string.h>
#include <kernel/vm.h>
#include <kernel/page.h>
#include <queue.h>

#include <threads.h>

#include "khash.h"

// ------------------------------------------------------------
// Internals
// ------------------------------------------------------------
typedef struct cache_el
{
    void *              data;           // must be wired v=r mem
    long                blk; 		// disk block no (blocksize is in cache_t)
    int                 valid;          // 0 = el is unused
    int                 dirty;          // 0 = same as disk contents

    queue_chain_t       lru;
    struct cache_el *   hash_next;
} cache_el_t;



typedef struct
{
    hal_mutex_t         lock;

    size_t      	page_size;      // Size of one cached element
    size_t      	cache_size;     // Current num of elements

    // Each el is in hash and in q at the same time:

    queue_head_t        lru;            // This queue implements LRU
    void *              hash;           // This hash helps with lookup time

} cache_t;

// ------------------------------------------------------------
// Interface
// ------------------------------------------------------------



// ------------------------------------------------------------
// Implementation
// ------------------------------------------------------------

int c_el_compare_func(void *a, const void *key)
{
    cache_el_t *el = a;
    const long blk = *(const long *)key;

    return !( el->blk == blk );
}

unsigned int c_el_hash_func(void *a, const void *key, unsigned int range)
{
    cache_el_t *el = a;
    const long blk = *(const long *)key;

    if(el)
        return el->blk % range;
    else
        return blk % range;
}

static void cache_do_create_els( cache_t *c, int n );


static errno_t cache_do_init( cache_t *c, size_t page_size )
{
    memset( c, sizeof(cache_t), 0 );

    hal_mutex_init( &c->lock, "DiskCache" );
    hal_mutex_lock( &c->lock );

    c->page_size = page_size;

    queue_init( &c->lru );

    unsigned int table_size = 2048;
    c->hash = hash_init( table_size, offsetof(cache_el_t, hash_next),
                         c_el_compare_func,
                         c_el_hash_func );

    hal_mutex_unlock( &c->lock );

    cache_do_create_els( c, 1024 );

    return 0;
}


static cache_el_t * cache_do_find( cache_t *c, long blk )
{
    assert(hal_mutex_is_locked(&c->lock));
    cache_el_t * e;
    e = hash_lookup( c->hash, &blk );
    return e;
}


static cache_el_t * cache_do_create_el( cache_t *c )
{
    void *elm = calloc( 1, c->page_size );
    assert(elm);

    cache_el_t *el = calloc( 1, sizeof(cache_el_t) );
    assert(el);

    el->data = elm;

    el->hash_next = 0;
    el->valid = 0;
    el->dirty =0;

    return el;
}


// return unused el - must be not in q/hash
static void cache_do_put( cache_t *c, cache_el_t *el)
{
    assert(el->valid);
    assert(hal_mutex_is_locked(&c->lock));
    assert( !cache_do_find( c, el->blk ));
    // costs too much to check if we are on q, and
    // dups on q make no big harm

    assert(!hash_insert( c->hash, el));

    // insert at start
    queue_enter_first( &c->lru, el, cache_el_t *, lru );
}


// return unused el - must be not in q/hash
static void cache_do_return_unused( cache_t *c, cache_el_t *el)
{
    assert(!el->valid);
    assert(hal_mutex_is_locked(&c->lock));
    assert( !cache_do_find( c, el->blk ));
    // costs too much to check if we are on q, and
    // dups on q make no big harm

    // insert at end
    queue_enter( &c->lru, el, cache_el_t *, lru );
}




static void cache_do_create_els( cache_t *c, int n )
{
    while(n-- > 0)
    {
        cache_el_t *el = cache_do_create_el( c );
        cache_do_return_unused(c, el);
    }
}


static errno_t cache_read( cache_t *c, long blk, void *data )
{
    assert(!queue_empty(&c->lru));

    errno_t ret = 0;
    hal_mutex_lock( &c->lock );

    cache_el_t * el = cache_do_find( c, blk );
    if( el == 0 )
    {
        ret = ENOENT;
        goto done;
    }

    // remove
    queue_remove( &c->lru, el, cache_el_t *, lru );

    // insert at start
    queue_enter_first( &c->lru, el, cache_el_t *, lru );

    memcpy( data, el->data, c->page_size );

done:
    hal_mutex_unlock( &c->lock );
    return ret;
}


static errno_t cache_write( cache_t *c, long blk, void *data )
{
    assert(!queue_empty(&c->lru));

    errno_t ret = 0;
    hal_mutex_lock( &c->lock );

    cache_el_t * el = cache_do_find( c, blk );
    if( el == 0 )
    {
        el = queue_last(&c->lru);

        // if valid and diry - must flush!

        hash_remove( c->hash, el );

        el->valid = 1;
        el->blk = blk;

        assert(!hash_insert( c->hash, el));
    }

    el->dirty = 1;

    // remove
    queue_remove( &c->lru, el, cache_el_t *, lru );

    // insert at start
    queue_enter_first( &c->lru, el, cache_el_t *, lru );

    memcpy( el->data, data, c->page_size );

done:
    hal_mutex_unlock( &c->lock );
    return ret;
}






