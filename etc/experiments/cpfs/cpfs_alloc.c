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
cpfs_alloc_disk_block( cpfs_fs_t *fs )
{
    cpfs_blkno_t ret;

    //
    // Try free list first
    //
#if 1
    cpfs_mutex_lock( fs->freelist_mutex );

    if( !fs->sb.free_list )
        goto no_freelist;

    // read free list block

    ret = fs->sb.free_list;
    struct cpfs_freelist *fb = cpfs_lock_blk( fs, ret );

    if( fb->fl_magic != CPFS_FL_MAGIC )
    {
        fs->sb.free_list = 0; // freeing blocks will recreate free list, though part of disk is inavailable now
        cpfs_log_error("Freelist corrupt (blk %lld), attempt to continue on unallocated space", (long long)fs->sb.free_list );
        goto no_freelist;
    }

    fs->sb.free_list = fb->next;

    //cpfs_touch_blk( blk );
    cpfs_unlock_blk( fs, ret );

    fs->sb.free_count--;
    return ret;

no_freelist:
    cpfs_mutex_unlock( fs->freelist_mutex );
#endif


    //
    // Nothing in free list, alloc from rest of FS block space, if possible
    //

    cpfs_sb_lock( fs );

    // No space left on device
    if( fs->sb.first_unallocated >= fs->sb.disk_size )
    {
        cpfs_sb_unlock( fs );
        return 0;
    }

    fs->sb.free_count--;
    ret = fs->sb.first_unallocated++;

    if( fs->sb.free_count <= 0 )
        cpfs_panic("cpfs_alloc_disk_block disk state inconsistency: fs->sb.free_count <= 0");


    errno_t rc = cpfs_sb_unlock_write( fs );
    if( rc )
    {
        fs->sb.first_unallocated--;
        fs->sb.free_count++;
        cpfs_log_error("Can't write SB allocating from fs->sb.first_unallocated");
        cpfs_sb_unlock( fs );
        return 0;
    }

    return ret;
}


void
cpfs_free_disk_block( cpfs_fs_t *fs, cpfs_blkno_t blk )
{
    cpfs_mutex_lock( fs->freelist_mutex );

    // Write current head of free list block number to block we're freeing

    struct cpfs_freelist *fb = cpfs_lock_blk( fs, blk );

    fb->next = fs->sb.free_list;
    fb->fl_magic = CPFS_FL_MAGIC;

    cpfs_touch_blk( fs, blk );
    cpfs_unlock_blk( fs, blk );

    fs->sb.free_list = blk;
    fs->sb.free_count++;

    cpfs_mutex_unlock( fs->freelist_mutex );
}


