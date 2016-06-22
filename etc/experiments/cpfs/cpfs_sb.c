


static int sb_blk = 0;


errno_t cpfs_mkfs(cpfs_blkno_t disk_size)
{

    struct cpfs_sb      *sb = cpfs_lock_blk( sb_blk );

    if( blk_data == 0 ) return EFAULT; // ? TODO


    sb->sb_magic_0 = CPFS_SB_MAGIC;

    sb->ninode = 8192*8192; // todo magic?

    int ino_table_blkno = CPFS_INO_PER_BLK * sb->ninode;

    sb->itable_pos = 1; // just after sb
    sb->itable_end = sb->itable_pos;

    sb->disk_size = disk_size;

    sb->first_unallocated = ino_table_blkno+sb->itable_pos;
    sb->free_list = 0;




    if( sb->first_unallocated <= disk_size )
    {
        cpfs_unlock_blk( sb_blk );
        return EINVAL;
    }



    cpfs_touch_blk( sb_blk ); // marks block as dirty, will be saved to disk on unlock
    cpfs_unlock_blk( sb_blk );

    return 0;
}


errno_t cpfs_init_sb(void)
{

    // TODO assert( sizeof(struct cpfs_inode) < CPFS_INO_REC_SIZE );

    struct cpfs_sb      *sb = cpfs_lock_blk( sb_blk );

    if( blk_data == 0 ) return EFAULT; // ? TODO


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

    if( blk_data == 0 ) return EFAULT; // ? TODO


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





