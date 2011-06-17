/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Disk IO stack - pool of partitions and corresp ops
 *
**/

#define DEBUG_MSG_PREFIX "part.pool"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include <disk.h>
#include <kernel/pool.h>
#include <kernel/init.h>
#include <kernel/libkern.h>
#include <kernel/page.h>


static pool_t   *pp;

static void * 	do_part_create(void *arg);
static void  	do_part_destroy(void *arg);

static errno_t checkRange( struct phantom_disk_partition *p, long blockNo, int nBlocks )
{
    if( blockNo < 0 || blockNo > p->size )
        return EINVAL;

    if( blockNo+nBlocks < 0 || blockNo+nBlocks > p->size )
        return EINVAL;

    return 0;
}

// NB!! pager_io_request's disk block no is IGNORED! Parameter is used!
// Parameter has to be in partition's block size
// TODO must be static
//static
errno_t partAsyncIo( struct phantom_disk_partition *p, pager_io_request *rq )
{
    assert(p->specific == 0);
    assert(p->base);
    // Temp! Rewrite!
    assert(p->base->block_size == p->block_size);

    if( checkRange( p, rq->blockNo, rq->nSect ) )
        return EINVAL;

    SHOW_FLOW( 11, "part io i sect %d, shift %d, o sect %d", rq->blockNo, p->shift, rq->blockNo + p->shift );
    rq->blockNo += p->shift;

    p->base->asyncIo( p, rq );
    return 0;
}


void phantom_init_part_pool()
{
    pp = create_pool();
    pp->init = do_part_create;
    pp->destroy = do_part_destroy;
}


static void * 	do_part_create(void *arg)
{
    assert(arg == 0);

    phantom_disk_partition_t *ret = calloc( 1, sizeof(phantom_disk_partition_t) );

    ret->shift = 0;
    ret->size = 0;

    ret->block_size = 512;
    ret->flags = 0;

    ret->specific = 0;

    ret->asyncIo = partAsyncIo;

    ret->base = 0;
    ret->baseh.h = -1;

    return ret;
}

static void  	do_part_destroy(void *arg)
{
    phantom_disk_partition_t *p = arg;
    SHOW_FLOW( 1, "delete part %s", p->name );

    if(p->baseh.h >= 0)
        pool_release_el( pp, p->baseh.h );
}



partition_handle_t      dpart_create( partition_handle_t base, long shift, long size)
{
    pool_handle_t h = pool_create_el( pp, 0 );
    phantom_disk_partition_t *p = pool_get_el(pp,h);

    p->shift = shift;
    p->size = size;

    p->baseh = base;
    p->self.h = h;

    pool_release_el( pp, h );

    partition_handle_t      ret;

    ret.h = h;

    return ret;
}


// These work with 4096 byte blocks
errno_t dpart_read_block( partition_handle_t h, void *to, long blockNo, int nBlocks )
{
    phantom_disk_partition_t *p = pool_get_el(pp,h.h);
    errno_t ret = phantom_sync_read_block( p, to, blockNo, nBlocks );
    pool_release_el( pp, h.h );

    return ret;
}

errno_t dpart_write_block( partition_handle_t h, const void *from, long blockNo, int nBlocks )
{
    phantom_disk_partition_t *p = pool_get_el(pp,h.h);
    errno_t ret = phantom_sync_write_block( p, from, blockNo, nBlocks );
    pool_release_el( pp, h.h );

    return ret;
}

// These work with native sized (512byte) sectors 
errno_t dpart_read_sector( partition_handle_t h, void *to, long sectorNo, int nSectors )
{
    phantom_disk_partition_t *p = pool_get_el(pp,h.h);
    errno_t ret = phantom_sync_read_sector( p, to, sectorNo, nSectors );
    pool_release_el( pp, h.h );

    return ret;
}

errno_t dpart_write_sector( partition_handle_t h, const void *from, long sectorNo, int nSectors )
{
    phantom_disk_partition_t *p = pool_get_el(pp,h.h);
    errno_t ret = phantom_sync_write_sector( p, from, sectorNo, nSectors );
    pool_release_el( pp, h.h );

    return ret;
}


/** Start async disk io operation */
void dpart_enqueue( partition_handle_t h, pager_io_request *rq )
{
    // NB! We do not release it until IO is done - TODO release!
    phantom_disk_partition_t *p = pool_get_el(pp,h.h);

    int m = PAGE_SIZE/p->block_size;
    rq->blockNo = rq->disk_page*m;
    rq->nSect = m;

    assert( rq->flag_ioerror == 0 );
    assert( rq->flag_pagein != rq->flag_pageout );

    p->asyncIo( p, rq );
}



void dpart_release_async( pool_handle_t h )
{
    pool_release_el( pp, h );
}




