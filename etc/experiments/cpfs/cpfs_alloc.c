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


static cpfs_mutex freelist_mutex;


cpfs_blkno_t
cpfs_alloc_disk_block( void )
{
    cpfs_blkno_t ret;

    //
    // Try free list first
    //

    cpfs_mutex_lock( freelist_mutex );

    if( !fs_sb.free_list )
        goto no_freelist;

    // read free list block

    ret = fs_sb.free_list;
    struct cpfs_freelist *fb = cpfs_lock_blk( ret );

    if( fb->fl_magic != CPFS_FL_MAGIC )
    {
        fs_sb.free_list = 0; // freeing blocks will recreate free list, though part of disk is inavailable now
        cpfs_log_error("Freelist corrupt (blk %lld), attempt to continue on unallocated space", (long long)fs_sb.free_list );
        goto no_freelist;
    }

    fs_sb.free_list = fb->next;

    //cpfs_touch_blk( blk );
    cpfs_unlock_blk( ret );

    fs_sb.free_count--;
    return ret;

no_freelist:
    cpfs_mutex_unlock( freelist_mutex );



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
    ret = fs_sb.first_unallocated++;

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
    cpfs_mutex_lock( freelist_mutex );

    // Write current head of free list block number to block we're freeing

    struct cpfs_freelist *fb = cpfs_lock_blk( blk );

    fb->next = fs_sb.free_list;
    fb->fl_magic = CPFS_FL_MAGIC;

    cpfs_touch_blk( blk );
    cpfs_unlock_blk( blk );

    fs_sb.free_list = blk;
    fs_sb.free_count++;

    cpfs_mutex_unlock( freelist_mutex );

}


