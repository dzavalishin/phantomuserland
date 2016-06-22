/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Inode io.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"


// TODO make some cache, at least for multiple clients to be able to work


static char write = 0;
static char used = 0;
static cpfs_blkno_t curr_blk = -1;
static cpfs_ino_t curr_ino = -1;

static char data[CPFS_BLKSIZE];


struct cpfs_inode *
cpfs_lock_ino( cpfs_ino_t ino ) // makes sure that block is in memory
{
    errno_t rc;

    if( used ) cpfs_panic( "out of inode buffers" );

    cpfs_blkno_t blk;
    rc = cpfs_block_4_inode( ino, &blk );
    if( rc ) cpfs_panic( "cpfs_block_4_inode" );

    curr_blk = blk;
    curr_ino = ino;

    write = 0;
    used = 1;

    rc = cpfs_disk_read( 0, blk, data );

    if( rc ) cpfs_panic( "read inode blk" );

    int ino_in_blk = ino % CPFS_INO_PER_BLK;

    return data + (ino_in_blk * CPFS_INO_REC_SIZE);
}

void
cpfs_touch_ino(  cpfs_ino_t ino ) // marks block as dirty, will be saved to disk on unlock
{
    if( curr_ino != ino ) cpfs_panic( "wrong ino in touch" );
    //if( curr_blk != blk ) cpfs_panic( "wrong blk in inode touch" );
    write = 1;
}


void
cpfs_unlock_ino( cpfs_ino_t ino ) // flushes block to disk before unlocking it, if touched
{
    if( !used ) cpfs_panic( "double cpfs_unlock_blk" );
    if( curr_ino != ino ) cpfs_panic( "wrong ino in unlock" );
    //if( curr_blk != blk ) cpfs_panic( "wrong blk in inode unlock" );

    if( write )
    {
        errno_t rc = cpfs_disk_write( 0, curr_blk, data );

        if( rc ) cpfs_panic( "write inode blk" );
    }

    used = 0;
}

