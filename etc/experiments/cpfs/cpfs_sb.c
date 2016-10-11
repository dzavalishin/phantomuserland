/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Superblock ops, mkfs.
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"


// TODO move to header
#define sb_blk 0


// ----------------------------------------------------------------------------
//
// Make filesystem (format disk)
//
// ----------------------------------------------------------------------------



errno_t cpfs_mkfs( cpfs_fs_t *fs, cpfs_blkno_t disk_size)
{
    //errno_t rc;

    struct cpfs_sb      *sb = cpfs_lock_blk( fs, sb_blk );

    if( sb == 0 ) return EFAULT; // ? TODO

    memset( sb, 0, sizeof( *sb ) );

    sb->h.magic = CPFS_SB_MAGIC;

    //sb->ninode = 8192*8192; // todo magic?
    //sb->ninode = 1024; // todo magic?
    sb->ninode = disk_size/20; // todo magic?

    int ino_table_blkno = CPFS_INO_PER_BLK * sb->ninode;

    sb->itable_pos = 1; // just after sb
    sb->itable_end = sb->itable_pos;

    sb->disk_size = disk_size;

    sb->first_unallocated = ino_table_blkno+sb->itable_pos;
    sb->free_list = 0;

    sb->free_count = disk_size;
    sb->free_count -= 1; // sb
    sb->free_count -= ino_table_blkno;

    // sanity check
    if( sb->free_count != disk_size - sb->first_unallocated )
        cpfs_panic("sb->free_count (%lld) != disk_size (%lld) - sb->first_unallocated (%lld)", (long long)sb->free_count, (long long)disk_size, (long long)sb->first_unallocated);

    if( sb->first_unallocated >= disk_size )
    {
        cpfs_unlock_blk( fs, sb_blk );
        return EINVAL;
    }



    cpfs_touch_blk( fs, sb_blk ); // marks block as dirty, will be saved to disk on unlock
    cpfs_unlock_blk( fs, sb_blk );

    // temp init global superblock copy for lock_ino to work
    fs->sb = *sb;


    cpfs_ino_t root_dir = 0;

    // Init inode 0 as root dir

    struct cpfs_inode *rdi = cpfs_lock_ino( fs, root_dir );
    cpfs_touch_ino( fs, root_dir );
    cpfs_inode_init_defautls( fs, rdi );
    rdi->ftype = CPFS_FTYPE_DIR;
    rdi->nlinks = 1;
    cpfs_unlock_ino( fs, root_dir );

    // de-init!
    memset( &fs->sb, 0, sizeof( fs->sb ) );


    return 0;
}


// ----------------------------------------------------------------------------
//
// Superblock ops
//
// ----------------------------------------------------------------------------







errno_t cpfs_init_sb( cpfs_fs_t *fs )
{
    cpfs_assert( sizeof(struct cpfs_inode) < CPFS_INO_REC_SIZE );
    cpfs_mutex_init( &(fs->sb_mutex) );

    return 0;
}




errno_t cpfs_stop_sb( cpfs_fs_t *fs )
{
    cpfs_mutex_stop( fs->sb_mutex );
    return 0;
}





errno_t cpfs_mount_sb( cpfs_fs_t *fs )
{
    struct cpfs_sb      *sb = cpfs_lock_blk( fs, sb_blk );
    if( sb == 0 ) return EFAULT; // ? TODO

    if( sb->h.magic != CPFS_SB_MAGIC )
    {
        cpfs_log_error("can't mount disk, no FS magic");
        cpfs_unlock_blk( fs, sb_blk );
        return EINVAL;
    }

    if( sb->dirty )
    {
        cpfs_log_error("can't mound dirty disk, need FSCK"); // TODO run fsck from here? different return code? EFTYPE
        cpfs_unlock_blk( fs, sb_blk );
        return EINVAL;
    }

    fs->sb = *sb;

    cpfs_unlock_blk( fs, sb_blk );

    return 0;
}


errno_t cpfs_write_sb( cpfs_fs_t *fs )
{
    struct cpfs_sb      *sb = cpfs_lock_blk( fs, sb_blk );

    if( sb == 0 ) return EFAULT; // ? TODO

    *sb = fs->sb;

    cpfs_unlock_blk( fs, sb_blk );

    return 0;
}



void
cpfs_sb_lock( cpfs_fs_t *fs )
{
    cpfs_mutex_lock( fs->sb_mutex );
}

void
cpfs_sb_unlock( cpfs_fs_t *fs )
{
    cpfs_mutex_unlock( fs->sb_mutex );
}


errno_t
cpfs_sb_unlock_write( cpfs_fs_t *fs ) // if returns error, sb is not written and IS NOT UNLOCKED
{

    errno_t rc = cpfs_write_sb( fs );
    if( rc ) return rc;


    cpfs_sb_unlock( fs );
    return 0;
}





