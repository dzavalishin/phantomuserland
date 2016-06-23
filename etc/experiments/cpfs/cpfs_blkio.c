/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Disk blk io.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"


// TODO make some cache, at least for multiple clients to be able to work


static char write = 0;
static char used = 0;
static cpfs_blkno_t curr_blk = -1;

static char data[CPFS_BLKSIZE];


void *
cpfs_lock_blk( cpfs_blkno_t blk ) // makes sure that block is in memory
{
    if( used ) cpfs_panic( "out of disk buffers" );

    write = 0;
    used = 1;
    curr_blk = blk;

    errno_t rc = cpfs_disk_read( 0, blk, data );

    if( rc ) cpfs_panic( "read blk %lld", (long long)blk );

    return data;
}

void
cpfs_touch_blk(  cpfs_blkno_t blk ) // marks block as dirty, will be saved to disk on unlock
{
    if( curr_blk != blk ) cpfs_panic( "wrong blk in touch" );
    write = 1;
}


void
cpfs_unlock_blk( cpfs_blkno_t blk ) // flushes block to disk before unlocking it, if touched
{
    if( !used ) cpfs_panic( "double cpfs_unlock_blk" );
    if( curr_blk != blk ) cpfs_panic( "wrong blk in unlock" );

    if( write )
    {
        errno_t rc = cpfs_disk_write( 0, blk, data );

        if( rc ) cpfs_panic( "write blk %lld", (long long)blk );
    }

    used = 0;
}

