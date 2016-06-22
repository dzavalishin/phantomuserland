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

// todo - caller must validate read size does not exceed file size
// returns einval if read can be partially done (and is partially done)

errno_t
cpfs_ino_file_read  ( cpfs_ino_t ino, cpfs_size_t pos, const void *data, cpfs_size_t size )
{
    //
    // do first (partial) block
    //

    cpfs_blkno_t logical_blk = CPFS_FILE_POS_2_BLK( pos );

    cpfs_blkno_t phys_blk =  cpfs_find_block_4_file( ino, logical_blk );
    if( phys_blk == 0 ) return EINVAL;

    char *blk_data = cpfs_lock_blk( phys_blk );
    //void                    cpfs_touch_blk(  cpfs_blkno_t blk ); // marks block as dirty, will be saved to disk on unlock

    if( blk_data == 0 ) return EINVAL;


    cpfs_size_t off = CPFS_FILE_POS_2_OFF( pos );
    cpfs_size_t part = CPFS_BLKSIZE-off;

    if( part > size ) part = size;

    memcpy( blk_data+off, data, part );

    pos += part;
    data += part;
    size -= part;

    cpfs_unlock_blk( phys_blk );


    //
    // do full blocks
    //


    while( size > CPFS_BLKSIZE )
    {

        logical_blk = CPFS_FILE_POS_2_BLK( pos );

        phys_blk =  cpfs_find_block_4_file( ino, logical_blk );
        if( phys_blk == 0 ) return EINVAL;

        char *blk_data = cpfs_lock_blk( phys_blk );
        //void                    cpfs_touch_blk(  cpfs_blkno_t blk ); // marks block as dirty, will be saved to disk on unlock

        if( blk_data == 0 ) return EINVAL;

        off = CPFS_FILE_POS_2_OFF( pos );
        if( off ) return EFAULT; // assert? panic? we're fried?

        part = CPFS_BLKSIZE;
        if( part > size ) return EFAULT; // can't be? assert? panic?

        memcpy( blk_data, data, part );

        pos += part;
        data += part;
        size -= part;

        cpfs_unlock_blk( phys_blk );
    }


    //
    // do last (partial) block
    //

    logical_blk = CPFS_FILE_POS_2_BLK( pos );

    phys_blk =  cpfs_find_block_4_file( ino, logical_blk );
    if( phys_blk == 0 ) return EINVAL;

    blk_data = cpfs_lock_blk( phys_blk );
    //void                    cpfs_touch_blk(  cpfs_blkno_t blk ); // marks block as dirty, will be saved to disk on unlock

    if( blk_data == 0 ) return EINVAL;

    off = CPFS_FILE_POS_2_OFF( pos );
    if( off ) return EFAULT; // assert? panic? we're fried?

    part = CPFS_BLKSIZE;
    if( part > size ) part = size;

    memcpy( blk_data, data, part );

    pos += part;
    data += part;
    size -= part;

    cpfs_unlock_blk( phys_blk );


    if( size ) return EFAULT; // assert? panic? we're fried?

    return 0;
}






errno_t
cpfs_ino_file_write ( cpfs_ino_t ino, cpfs_size_t pos, const void *data, cpfs_size_t size )
{
}




