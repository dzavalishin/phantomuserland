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


// ----------------------------------------------------------------------------
//
// Give out FS info.
//
// ----------------------------------------------------------------------------



errno_t
cpfs_fs_stat( struct cpfs_fs *fs, cpfs_blkno_t *disk_size, cpfs_blkno_t *disk_free )
{
    cpfs_assert( fs );
    cpfs_assert( disk_size );
    cpfs_assert( disk_free );

    *disk_free = fs->sb.free_count;
    *disk_size = fs->sb.disk_size;

    return 0;
}




// ----------------------------------------------------------------------------
//
// Allocate disk block.
//
// ----------------------------------------------------------------------------


cpfs_blkno_t
cpfs_alloc_disk_block( cpfs_fs_t *fs )
{
    cpfs_blkno_t ret;

    //
    // Try free list first
    //

    cpfs_sb_lock( fs ); // we modify sb.free_count which is protected
    cpfs_mutex_lock( fs->freelist_mutex );

    if( !fs->sb.free_list )
        goto no_freelist;

    // read free list block

    ret = fs->sb.free_list;
    struct cpfs_freelist *fb = cpfs_lock_blk( fs, ret );

    if( fb->h.magic != CPFS_FL_MAGIC )
    {
        cpfs_log_error("Freelist corrupt (blk %lld), attempt to continue on unallocated space", (long long)fs->sb.free_list );
        fs->sb.free_list = 0; // freeing blocks will recreate free list, though part of disk is inavailable now
        goto no_freelist;
    }

    fs->sb.free_list = fb->next;

    //cpfs_touch_blk( blk );
    cpfs_unlock_blk( fs, ret );

    fs->sb.free_count--;
//printf(" freelist alloc ");
    cpfs_mutex_unlock( fs->freelist_mutex );
    cpfs_sb_unlock( fs ); // we modify sb.free_count which is protected

    return ret;

no_freelist:
    cpfs_mutex_unlock( fs->freelist_mutex );
    //cpfs_sb_unlock( fs ); // we modify sb.free_count which is protected



    //
    // Nothing in free list, alloc from rest of FS block space, if possible
    //

    //cpfs_sb_lock( fs );

    // No space left on device
    if( fs->sb.first_unallocated >= fs->sb.disk_size )
    {
        cpfs_sb_unlock( fs );
        return 0;
    }

    if( fs->sb.free_count <= 0 )
        cpfs_panic("cpfs_alloc_disk_block disk state inconsistency: fs->sb.free_count <= 0");

    fs->sb.free_count--;
    ret = fs->sb.first_unallocated++;

    //if( fs->sb.free_count <= 0 )
    //    cpfs_panic("cpfs_alloc_disk_block disk state inconsistency: fs->sb.free_count <= 0");


    errno_t rc = cpfs_sb_unlock_write( fs );
    if( rc )
    {
        fs->sb.first_unallocated--;
        fs->sb.free_count++;
        cpfs_log_error("Can't write SB allocating from fs->sb.first_unallocated");
        cpfs_sb_unlock( fs );
        return 0;
    }

    //printf(" ++ alloc ");
    return ret;
}


// ----------------------------------------------------------------------------
//
// Free disk block.
//
// ----------------------------------------------------------------------------


void
cpfs_free_disk_block( cpfs_fs_t *fs, cpfs_blkno_t blk )
{
    cpfs_assert( blk !=0 );
    cpfs_assert( fs != 0 );

    cpfs_mutex_lock( fs->freelist_mutex );

    // Write current head of free list block number to block we're freeing

    struct cpfs_freelist *fb = cpfs_lock_blk( fs, blk );
    cpfs_assert( fb != 0 );

    fb->next = fs->sb.free_list;
    fb->h.magic = CPFS_FL_MAGIC;

    cpfs_touch_blk( fs, blk );
    cpfs_unlock_blk( fs, blk );

    fs->sb.free_list = blk;
    fs->sb.free_count++;
//printf(" free head = %lld", fs->sb.free_list );
    cpfs_mutex_unlock( fs->freelist_mutex );
}



// ----------------------------------------------------------------------------
//
// Print out general FS info.
//
// ----------------------------------------------------------------------------




errno_t
cpfs_fs_dump( struct cpfs_fs *fs )
{
    cpfs_assert( fs );

    printf("fs size %lld, free %lld\n", (long long)fs->sb.disk_size, (long long)fs->sb.free_count );

    return 0;
}


