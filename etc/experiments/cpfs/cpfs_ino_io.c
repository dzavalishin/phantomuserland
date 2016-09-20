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

#if 1
#  define write 	fs->ino_lock_write
#  define used 		fs->ino_lock_used
#  define curr_blk	fs->ino_lock_curr_blk
#  define curr_ino	fs->ino_lock_curr_ino
#  define data          fs->ino_lock_data
#else
static char write = 0;
static char used = 0;
static cpfs_blkno_t curr_blk = -1;
static cpfs_ino_t curr_ino = -1;

static char data[CPFS_BLKSIZE];
#endif


// ----------------------------------------------------------------------------
//
// Lock inode structure in memory - read and make available exclusively to this
// caller inode structure for given inode.
//
// ----------------------------------------------------------------------------




struct cpfs_inode *
cpfs_lock_ino( cpfs_fs_t *fs, cpfs_ino_t ino ) // makes sure that block is in memory
{
    errno_t rc;
/*
if( TRACE ) trace(1, "%*s > cpfs_lock_ino. ino=%lld\n", TRACE, " ", ino);   
*/
    cpfs_mutex_lock( fs->inode_mutex );

    if( used ) cpfs_panic( "out of inode buffers" );

    cpfs_blkno_t blk;
    rc = cpfs_block_4_inode( fs, ino, &blk );
    if( rc ) cpfs_panic( "cpfs_block_4_inode" );

    curr_blk = blk;
    curr_ino = ino;

    write = 0;
    used = 1;

    rc = cpfs_disk_read( fs->disk_id, blk, data );
    if( rc ) cpfs_panic( "read inode blk" );

    // Inode table growth

    cpfs_sb_lock( fs );

    if( blk > fs->sb.itable_end ) cpfs_panic( "attempt to read inode %lld after fs->sb.itable_end (%lld)", (long long) blk, (long long) fs->sb.itable_end );

    if( blk == fs->sb.itable_end )
    {
        fs->sb.itable_end++;
        //rc = cpfs_write_sb();
        //if( rc ) panic("can't write sb");

        memset( data, 0, CPFS_BLKSIZE );
    }

    rc = cpfs_sb_unlock_write( fs );
    if( rc ) cpfs_panic("can't write sb"); // TODO just log and refuse call?

    int ino_in_blk = ino % CPFS_INO_PER_BLK;

    struct cpfs_inode *ip = ((void *)data) + (ino_in_blk * CPFS_INO_REC_SIZE);

    // Init volatile part of inode - TODO unused yet, need deinit, turn off now

    //cpfs_mutex_init( &(ip->mutex) );

    //if( TRACE ) trace(0, "%*s < cpfs_lock_ino. ino=%lld, data: [links=%d, ]\n", TRACE, " ", ino, ip->nlinks);     

    return ip;
}


// ----------------------------------------------------------------------------
//
// Touch inode - make sure it will be written on unlock - TODO move to unlock
//
// ----------------------------------------------------------------------------



void
cpfs_touch_ino( cpfs_fs_t *fs, cpfs_ino_t ino ) // marks block as dirty, will be saved to disk on unlock
{
    if( curr_ino != ino ) cpfs_panic( "wrong ino in touch" );
    //if( curr_blk != blk ) cpfs_panic( "wrong blk in inode touch" );
    write = 1;
}


// ----------------------------------------------------------------------------
//
// Unlock inode - finish access to in-memory inode structure, write it if required.
//
// ----------------------------------------------------------------------------

void
cpfs_unlock_ino( cpfs_fs_t *fs, cpfs_ino_t ino ) // flushes block to disk before unlocking it, if touched
{
/*
    if( TRACE ) trace(-1, "%*s < cpfs_unlock_ino. ino=%lld\n", TRACE-TRACE_TAB, " ", ino);   
*/
    if( !used ) cpfs_panic( "double cpfs_unlock_blk" );
    if( curr_ino != ino ) cpfs_panic( "wrong ino in unlock" );
    //if( curr_blk != blk ) cpfs_panic( "wrong blk in inode unlock" );

    if( write )
    {
        /* No - sometimes we want to update atime only
        int ino_in_blk = ino % CPFS_INO_PER_BLK;

        struct cpfs_inode *ii = ((void *)data) + (ino_in_blk * CPFS_INO_REC_SIZE);

        ii->mtime = cpfs_get_current_time();
        ii->atime = ii->mtime;
        */
        errno_t rc = cpfs_disk_write( fs->disk_id, curr_blk, data );

        if( rc ) cpfs_panic( "write inode blk" );
    }

    used = 0;

    cpfs_mutex_unlock( fs->inode_mutex );
}


// ----------------------------------------------------------------------------
//
// Update inode time fields
//
// ----------------------------------------------------------------------------




errno_t
cpfs_update_ino_mtime( cpfs_fs_t *fs, cpfs_ino_t ino )
{
    struct cpfs_inode *ii = cpfs_lock_ino( fs, ino );

    if( 0 == ii ) return EIO;

    ii->mtime = cpfs_get_current_time();
    ii->atime = ii->mtime;

    cpfs_touch_ino( fs, ino );

    cpfs_unlock_ino( fs, ino );

    return 0;
}



errno_t
cpfs_update_ino_atime( cpfs_fs_t *fs, cpfs_ino_t ino )
{
#if CPFS_UPDATE_ATIME
    struct cpfs_inode *ii = cpfs_lock_ino( fs, ino );

    if( 0 == ii ) return EIO;

    ii->atime = cpfs_get_current_time();

    cpfs_touch_ino( fs, ino );

    cpfs_unlock_ino( fs, ino );
#else // CPFS_UPDATE_ATIME
    (void) fs;
    (void) ino;
#endif // CPFS_UPDATE_ATIME

    return 0;
}












