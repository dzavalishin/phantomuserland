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
#define debug_level_flow 1
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
#include <kernel/disk_cache.h>
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



//typedef
struct disk_cache
{
    hal_mutex_t         lock;

    size_t      	page_size;      // Size of one cached element
    //size_t      	cache_size;     // Current num of elements

    // Each el is in hash and in q at the same time:

    queue_head_t        lru;            // This queue implements LRU
    void *              hash;           // This hash helps with lookup time

    writeback_f_t *     writeback_f;    // Func to call to write buf to disk
    void *   		writeback_a;    // Arg or abovesaid func

}; // cache_t;

// ------------------------------------------------------------
// Local prototypes
// ------------------------------------------------------------

static errno_t cache_do_read( cache_t *c, long blk, void *data );
static errno_t cache_do_write( cache_t *c, long blk, const void *data );

static errno_t cache_do_init( cache_t *c, size_t page_size );

static void cache_do_create_els( cache_t *c, int n );


// ------------------------------------------------------------
// Interface
// ------------------------------------------------------------

errno_t cache_get( cache_t *c, long blk, void *data )
{
    return cache_do_read( c, blk, data );
}

errno_t cache_put( cache_t *c, long blk, const void *data )
{
    return cache_do_write( c, blk, data );
}

cache_t * cache_init( size_t page_size )
{
    SHOW_FLOW( 2, "Cache init blksize %d", page_size );

    cache_t *c = calloc(1, sizeof(cache_t));

    //assert(c);
    if( c == 0 )
        return 0;

    errno_t ret = cache_do_init( c, page_size );

    //if( ret )         panic("can't init cache");

    if( ret )
    {
        free(c);
        return 0;
    }

    return c;
}


errno_t cache_get_multiple( cache_t *c, long blk, int nblk, void *data )
{
    //SHOW_FLOW( 3, "get mult for %p from %ld, num %d", c, blk, nblk );
    while(nblk-- > 0)
    {
        errno_t rc = cache_get( c, blk, data );
        if(rc)
            return rc;
        blk++;
        data += c->page_size;
    }

    return 0;
}

errno_t cache_put_multiple( cache_t *c, long blk, int nblk, const void *data )
{
    //SHOW_FLOW( 3, "put mult for %p from %ld, num %d", c, blk, nblk );
    while(nblk-- > 0)
    {
        errno_t rc = cache_put( c, blk, data );
        if(rc)
            return rc;
        blk++;
        data += c->page_size;
    }

    return 0;
}

// Empty for this cache impl has no writeback capability
errno_t cache_flush_all( cache_t *c )
{
    SHOW_FLOW( 3, "Cache flush for %p -- ignored", c );
    return 0;
}

errno_t cache_set_writeback( cache_t *c, writeback_f_t *func, void *opaque )
{
    if(c->writeback_f)
        SHOW_ERROR( 3, "Reset wback f for %p", c );

    c->writeback_f = func;
    c->writeback_a = opaque;

    return 0;
}


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

    cache_do_create_els( c, 1024 );

    hal_mutex_unlock( &c->lock );

    return 0;
}


static cache_el_t * cache_do_find( cache_t *c, long blk )
{
    assert(hal_mutex_is_locked(&c->lock));
    cache_el_t * e;
    e = hash_lookup( c->hash, &blk );
    if(e) assert( e->blk == blk );
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

/*
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
*/

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
    assert(hal_mutex_is_locked(&c->lock));
    while(n-- > 0)
    {
        cache_el_t *el = cache_do_create_el( c );
        cache_do_return_unused(c, el);
    }
}


//! Find a cache entry and get data from it, or return ENOENT
static errno_t cache_do_read( cache_t *c, long blk, void *data )
{
    assert(!queue_empty(&c->lru));

    errno_t ret = 0;
    hal_mutex_lock( &c->lock );

    cache_el_t * el = cache_do_find( c, blk );
    if( el == 0 )
    {
        SHOW_FLOW( 10, "Cache r miss blk %ld", blk );
        ret = ENOENT;
        goto done;
    }

#if 0
    {
    ret = ENOENT;
    goto done;
    }
#endif
    // remove
    queue_remove( &c->lru, el, cache_el_t *, lru );

    // insert at start
    queue_enter_first( &c->lru, el, cache_el_t *, lru );

    memcpy( data, el->data, c->page_size );

    //hexdump( data, c->page_size, 0, 0 );

    SHOW_FLOW( 9, "Cache r _HIT_ blk %ld blksize %d", blk, c->page_size );
done:
    hal_mutex_unlock( &c->lock );
    return ret;
}

//! Place data to cache - find or reuse entry as needed
static errno_t cache_do_write( cache_t *c, long blk, const void *data )
{
    assert(!queue_empty(&c->lru));

    errno_t ret = 0;
    hal_mutex_lock( &c->lock );

    cache_el_t * el = cache_do_find( c, blk );
    if( el == 0 )
    {
        el = (cache_el_t *)queue_last(&c->lru);

        // if valid and dirty - must flush!

        hash_remove( c->hash, el );

        el->valid = 1;
        el->blk = blk;

        assert(!hash_insert( c->hash, el));
        SHOW_FLOW( 10, "Cache w miss blk %ld", blk );
    }
    else
    {
        assert(el->valid);
        assert(el->blk == blk);
        SHOW_FLOW( 9, "Cache w _HIT_ blk %ld", blk );
    }

    el->dirty = 1;

    // remove
    queue_remove( &c->lru, el, cache_el_t *, lru );

    // insert at start
    queue_enter_first( &c->lru, el, cache_el_t *, lru );

    memcpy( el->data, data, c->page_size );

//done:
    hal_mutex_unlock( &c->lock );
    return ret;
}






