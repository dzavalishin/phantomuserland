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

#define USE_BUF 1


#if !USE_BUF
static char write = 0;
static char used = 0;
static cpfs_blkno_t curr_blk = -1;

static char data[CPFS_BLKSIZE];
#endif // USE_BUF


// ----------------------------------------------------------------------------
//
// Lock disk buffer in memory, reading in if needed. If buffer is already in
// memory - access same buffer.
//
// ----------------------------------------------------------------------------




void *
cpfs_lock_blk( cpfs_fs_t *fs, cpfs_blkno_t blk ) // makes sure that block is in memory
{
#if USE_BUF
    errno_t rc;
    cpfs_buf_t *buf;

    rc = cpfs_buf_lock( fs, blk, &buf );
    if( rc ) return 0;

    return buf->data;
#else
    if( used ) cpfs_panic( "out of disk buffers" );

    write = 0;
    used = 1;
    curr_blk = blk;

    errno_t rc = cpfs_disk_read( fs->disk_id, blk, data );

    if( rc ) cpfs_panic( "read blk %lld", (long long)blk );

    return data;
#endif // USE_BUF
}


// ----------------------------------------------------------------------------
//
// Mark block as modified (written on unlock) - TODO just add parameter to unlock
//
// ----------------------------------------------------------------------------



void
cpfs_touch_blk( cpfs_fs_t *fs, cpfs_blkno_t blk ) // marks block as dirty, will be saved to disk on unlock
{
#if USE_BUF
    errno_t rc;
    cpfs_buf_t *buf;

    rc = cpfs_buf_lock( fs, blk, &buf );
    if( rc ) cpfs_panic( "lock blk %lld", (long long)blk );

    rc = cpfs_buf_unlock( fs, blk, 1 );
    if( rc ) cpfs_panic( "unlock blk %lld", (long long)blk );
#else
    if( curr_blk != blk ) cpfs_panic( "wrong blk in touch" );
    write = 1;
#endif // USE_BUF
}



// ----------------------------------------------------------------------------
//
// Unlock buffer, write out if required.
//
// ----------------------------------------------------------------------------



void
cpfs_unlock_blk( cpfs_fs_t *fs, cpfs_blkno_t blk ) // flushes block to disk before unlocking it, if touched
{
#if USE_BUF
    errno_t rc = cpfs_buf_unlock( fs, blk, 0 );
    if( rc ) cpfs_panic( "unlock blk %lld", (long long)blk );
#else
    if( !used ) cpfs_panic( "double cpfs_unlock_blk" );
    if( curr_blk != blk ) cpfs_panic( "wrong blk in unlock" );

    if( write )
    {
        errno_t rc = cpfs_disk_write( fs->disk_id, blk, data );

        if( rc ) cpfs_panic( "write blk %lld", (long long)blk );
    }

    used = 0;
#endif // USE_BUF
}

