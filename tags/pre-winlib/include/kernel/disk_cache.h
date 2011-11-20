/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Block io caching support.
 *
 *
**/

#ifndef DISK_CACHE_H
#define DISK_CACHE_H

#include <errno.h>

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------


struct disk_cache;

typedef struct disk_cache cache_t;

//! Function to call to write dirty buffers, when needed
typedef errno_t (*writeback_f_t)(cache_t *c, void *opaque, long blk, void *data, int datasize );


// ------------------------------------------------------------
// Methods
// ------------------------------------------------------------


//! Create cache of default size for given page (sector) size
//! Returns cache struct or null on fail (usually out of mem)
cache_t * cache_init( size_t page_size );

//! Find a cache entry and get data from it, or return ENOENT
errno_t cache_get( cache_t *c, long blk, void *data );

//! Find a cache entry and get data from it, or return ENOENT - multisector
errno_t cache_get_multiple( cache_t *c, long blk, int nblk, void *data );

//! Place data to cache - find or reuse entry as needed
errno_t cache_put( cache_t *c, long blk, const void *data );

//! Place data to cache - find or reuse entry as needed - multisector
errno_t cache_put_multiple( cache_t *c, long blk, int nblk, const void *data );


//! Make sure all the cached data is written to disk
// TODO cache_flush_all returns after all is surely done??
errno_t cache_flush_all( cache_t *c );

errno_t cache_set_writeback( cache_t *c, writeback_f_t *func, void *opaque );

//! Destroy cache
errno_t cache_destroy( cache_t * );


#endif // DISK_CACHE_H
