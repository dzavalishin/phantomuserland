/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Alloc/free disk blk.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"




cpfs_blkno_t
cpfs_alloc_disk_block( void )
{
    // TODO implement free list

    //cpfs_blkno_t        free_list;              // Head of free block list, or 0 if none

    //
    // Nothing in free list, alloc from rest of FS block space, if possible
    //

    cpfs_sb_lock();

    // No space left on device
    if( fs_sb.first_unallocated >= fs_sb.disk_size )
    {
        cpfs_sb_unlock();
        return 0;
    }

    fs_sb.free_count--;
    cpfs_blkno_t ret = fs_sb.first_unallocated++;

    if( fs_sb.free_count <= 0 )
        cpfs_panic("cpfs_alloc_disk_block disk state inconsistency: fs_sb.free_count <= 0");


    errno_t rc = cpfs_sb_unlock_write();
    if( rc )
    {
        fs_sb.first_unallocated--;
        fs_sb.free_count++;
        cpfs_log_error("Can't write SB allocating from fs_sb.first_unallocated");
        cpfs_sb_unlock();
        return 0;
    }

    return ret;
}


void
cpfs_free_disk_block( cpfs_blkno_t blk )
{
    // TODO implement me
}


