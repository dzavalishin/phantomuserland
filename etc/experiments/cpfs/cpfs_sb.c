/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Superblock ops.
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"



struct cpfs_sb fs_sb;


static int sb_blk = 0;


errno_t cpfs_mkfs(cpfs_blkno_t disk_size)
{
    errno_t rc;

    struct cpfs_sb      *sb = cpfs_lock_blk( sb_blk );

    if( sb == 0 ) return EFAULT; // ? TODO

    memset( sb, 0, sizeof( *sb ) );

    sb->sb_magic_0 = CPFS_SB_MAGIC;

    //sb->ninode = 8192*8192; // todo magic?
    sb->ninode = 1024; // todo magic?

    int ino_table_blkno = CPFS_INO_PER_BLK * sb->ninode;

    sb->itable_pos = 1; // just after sb
    sb->itable_end = sb->itable_pos;

    sb->disk_size = disk_size;

    sb->first_unallocated = ino_table_blkno+sb->itable_pos;
    sb->free_list = 0;




    if( sb->first_unallocated >= disk_size )
    {
        cpfs_unlock_blk( sb_blk );
        return EINVAL;
    }



    cpfs_touch_blk( sb_blk ); // marks block as dirty, will be saved to disk on unlock
    cpfs_unlock_blk( sb_blk );

    cpfs_ino_t root_dir = 0;

    /* can't be sure allocator starts with 0, just mark it as used
    rc = cpfs_alloc_inode( &root_dir );
    if( rc )
        cpfs_panic("root dir cna't alloc inode, rc=%d", rc);
    if( root_dir != 0 )
        cpfs_panic("root dir not in inode 0 but %d", root_dir );
        */

    struct cpfs_inode *rdi = cpfs_lock_ino( root_dir );
    cpfs_touch_ino( root_dir );

    rdi->ftype = CPFS_FTYPE_DIR;
    rdi->nlinks = 1;
    cpfs_unlock_ino( root_dir );

    return 0;
}


errno_t cpfs_init_sb(void)
{

    // TODO assert( sizeof(struct cpfs_inode) < CPFS_INO_REC_SIZE );

    struct cpfs_sb      *sb = cpfs_lock_blk( sb_blk );

    if( sb == 0 ) return EFAULT; // ? TODO


    if( sb->sb_magic_0 != CPFS_SB_MAGIC )
    {
        cpfs_unlock_blk( sb_blk );
        return EINVAL;
    }

    fs_sb = *sb;

    cpfs_unlock_blk( sb_blk );

    return 0;
}


errno_t cpfs_write_sb(void)
{
    struct cpfs_sb      *sb = cpfs_lock_blk( sb_blk );

    if( sb == 0 ) return EFAULT; // ? TODO


    *sb = fs_sb;

    cpfs_unlock_blk( sb_blk );

    return 0;
}



void
cpfs_sb_lock(void)
{
    // TODO
    // cpfs_mutex_lock( sb_mutex );
}

void
cpfs_sb_unlock(void)
{
    // TODO
    // cpfs_mutex_unlock( sb_mutex );
}


errno_t
cpfs_sb_unlock_write() // if returns error, sb is not written and IS NOT UNLOCKED
{

    errno_t rc = cpfs_write_sb();
    if( rc ) return rc;


    cpfs_sb_unlock();
    return 0;
}





