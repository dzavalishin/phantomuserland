/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * File data io.
 *
 * Internal read/wrire impl, used by user calls and internal directory io code
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"



#define CPFS_FILE_POS_2_BLK( __pos ) (__pos/CPFS_BLKSIZE)
#define CPFS_FILE_POS_2_OFF( __pos ) (__pos%CPFS_BLKSIZE)



// ----------------------------------------------------------------------------
//
// Read file data
//
// ----------------------------------------------------------------------------




// todo - caller must validate read size does not exceed file size
// returns einval if read can be partially done (and is partially done)

errno_t
cpfs_ino_file_read( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_size_t pos, void *data, cpfs_size_t size,  cpfs_blkno_t *phys_blk1 )
{
    errno_t rc;

    //
    // do first (partial) block
    //

    cpfs_blkno_t logical_blk = CPFS_FILE_POS_2_BLK( pos );

    cpfs_blkno_t phys_blk;
    rc = cpfs_find_block_4_file( fs, ino, logical_blk, &phys_blk );
    *phys_blk1=phys_blk;
    if( rc ) return rc;

    const char *blk_data = cpfs_lock_blk( fs, phys_blk );
    //void                    cpfs_touch_blk(  cpfs_blkno_t blk ); // marks block as dirty, will be saved to disk on unlock

    if( blk_data == 0 ) return EINVAL;


    cpfs_size_t off = CPFS_FILE_POS_2_OFF( pos );
    cpfs_size_t part = CPFS_BLKSIZE-off;

    if( part > size ) part = size;

    memcpy( data, blk_data+off, part );

    pos += part;
    data += part;
    size -= part;

    cpfs_unlock_blk( fs, phys_blk );
    cpfs_update_ino_atime( fs, ino ); // TODO rc


    //
    // do full blocks
    //


    while( size > CPFS_BLKSIZE )
    {

        logical_blk = CPFS_FILE_POS_2_BLK( pos );

        rc = cpfs_find_block_4_file( fs, ino, logical_blk, &phys_blk );
        if( rc ) return rc;

        char *blk_data = cpfs_lock_blk( fs, phys_blk );
        //void                    cpfs_touch_blk(  cpfs_blkno_t blk ); // marks block as dirty, will be saved to disk on unlock

        if( blk_data == 0 ) return EINVAL;

        off = CPFS_FILE_POS_2_OFF( pos );
        if( off ) return EFAULT; // assert? panic? we're fried?

        part = CPFS_BLKSIZE;
        if( part > size ) return EFAULT; // can't be? assert? panic?

        memcpy( data, blk_data, part );

        pos += part;
        data += part;
        size -= part;

        cpfs_unlock_blk( fs, phys_blk );
        cpfs_update_ino_atime( fs, ino ); // TODO rc
    }

    if( !size )
        return 0;

    //
    // do last (partial) block
    //

    logical_blk = CPFS_FILE_POS_2_BLK( pos );

    rc = cpfs_find_block_4_file( fs, ino, logical_blk, &phys_blk );
    if( rc ) return rc;

    blk_data = cpfs_lock_blk( fs, phys_blk );
    //void                    cpfs_touch_blk(  cpfs_blkno_t blk ); // marks block as dirty, will be saved to disk on unlock

    if( blk_data == 0 ) return EINVAL;

    off = CPFS_FILE_POS_2_OFF( pos );
    if( off ) return EFAULT; // assert? panic? we're fried?

    part = CPFS_BLKSIZE;
    if( part > size ) part = size;

    memcpy( data, blk_data, part );

    pos += part;
    data += part;
    size -= part;

    cpfs_unlock_blk( fs, phys_blk );
    cpfs_update_ino_atime( fs, ino ); // TODO rc

    cpfs_assert( size == 0 );

    return 0;
}




// ----------------------------------------------------------------------------
//
// Write file data
//
// ----------------------------------------------------------------------------




errno_t
cpfs_ino_file_write( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_size_t pos, const void *data, cpfs_size_t size )
{
    errno_t rc;

    //cpfs_size_t new_size = pos+size;

    //
    // do first (partial) block
    //

    cpfs_blkno_t logical_blk = CPFS_FILE_POS_2_BLK( pos );

    cpfs_blkno_t phys_blk;
    rc = cpfs_find_or_alloc_block_4_file( fs, ino, logical_blk, &phys_blk );
    if( rc ) return rc;

    char *blk_data = cpfs_lock_blk( fs, phys_blk );
    cpfs_touch_blk( fs, phys_blk );

    if( blk_data == 0 ) return EINVAL;


    cpfs_size_t off = CPFS_FILE_POS_2_OFF( pos );
    cpfs_size_t part = CPFS_BLKSIZE-off;

    if( part > size ) part = size;

    memcpy( blk_data+off, data, part );

    pos += part;
    data += part;
    size -= part;

    cpfs_unlock_blk( fs, phys_blk );
    cpfs_inode_update_fsize( fs, ino, pos );
    cpfs_update_ino_mtime( fs, ino ); // TODO rc

    //
    // do full blocks
    //


    while( size > CPFS_BLKSIZE )
    {

        logical_blk = CPFS_FILE_POS_2_BLK( pos );

        rc = cpfs_find_or_alloc_block_4_file( fs, ino, logical_blk, &phys_blk );
        if( rc ) return rc;

        char *blk_data = cpfs_lock_blk( fs, phys_blk );
        cpfs_touch_blk( fs, phys_blk );

        if( blk_data == 0 ) return EINVAL;

        off = CPFS_FILE_POS_2_OFF( pos );
        if( off ) return EFAULT; // assert? panic? we're fried?

        part = CPFS_BLKSIZE;
        if( part > size ) return EFAULT; // can't be? assert? panic?

        memcpy( blk_data, data, part );

        pos += part;
        data += part;
        size -= part;

        cpfs_unlock_blk( fs, phys_blk );
        cpfs_inode_update_fsize( fs, ino, pos );
        cpfs_update_ino_mtime( fs, ino ); // TODO rc
    }

    if( !size )
    {
        return 0;
    }

    //
    // do last (partial) block
    //

    logical_blk = CPFS_FILE_POS_2_BLK( pos );

    rc = cpfs_find_or_alloc_block_4_file( fs, ino, logical_blk, &phys_blk );
    if( rc ) return rc;

    blk_data = cpfs_lock_blk( fs, phys_blk );
    cpfs_touch_blk( fs, phys_blk );
    

    if( blk_data == 0 ) return EINVAL;

    off = CPFS_FILE_POS_2_OFF( pos );
    if( off ) return EFAULT; // assert? panic? we're fried?

    part = CPFS_BLKSIZE;
    if( part > size ) part = size;

    memcpy( blk_data, data, part );

    pos += part;
    data += part;
    size -= part;

    cpfs_unlock_blk( fs, phys_blk );
    cpfs_inode_update_fsize( fs, ino, pos );
    cpfs_update_ino_mtime( fs, ino ); // TODO rc


    cpfs_assert( size == 0 );

    return 0;
}




