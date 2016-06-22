/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Inode related code.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"

// maps logical blocks to physical, block must be allocated
errno_t
cpfs_find_block_4_file( cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys )
{
    // TODO assert( phys );
    // Read inode first
    struct cpfs_inode inode;
    struct cpfs_inode *inode_p = cpfs_lock_ino( ino );
    if( inode_p ) inode = *inode_p;
    cpfs_unlock_ino( ino );
    if( inode_p == 0 ) return EIO;

    if( logical*CPFS_BLKSIZE > inode.fsize ) return E2BIG; // TODO err?

    if( logical < CPFS_INO_DIR_BLOCKS )
    {
        *phys = inode.blocks0[logical];
        return 0;
    }


    // TODO write indirect blocks support!
    return E2BIG;
}



errno_t
cpfs_block_4_inode( cpfs_ino_t ino, cpfs_blkno_t *oblk )
{
    if( ino >= fs_sb.ninode ) return E2BIG;

    cpfs_blkno_t blk =  fs_sb.itable_pos + (ino/CPFS_INO_PER_BLK);
    //if( blk >= fs_sb.itable_end ) return E2BIG;

    *oblk = blk;
    return 0;
}

