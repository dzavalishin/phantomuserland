/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Directories related code.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"




errno_t
cpfs_namei( cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t *file_ino )
{
    errno_t rc;
    // TODO some speedup? In-mem hash?

    // TODO assert(file_ino)
    // TODO assert(fname)

    // read in directory inode to find out dir file len

    /*
    struct cpfs_inode inode;
    struct cpfs_inode *inode_p = cpfs_lock_ino( dir_ino );
    if( inode_p ) inode = *inode_p;
    cpfs_unlock_ino( dir_ino );
    if( inode_p == 0 ) return EIO;

    int nentry = inode.fsize/CPFS_DIR_REC_SIZE;
    */

    cpfs_size_t fsize;
    if( cpfs_fsize( dir_ino, &fsize ) )
        return EIO;

    int nentry = fsize/CPFS_DIR_REC_SIZE;

    // TODO assert( (inode.fsize%CPFS_DIR_REC_SIZE) == 0 )

    int nblk = nentry / CPFS_DIR_PER_BLK;
    int blkpos = 0;

    // Scan sequentially through the dir looking for name

    while( nblk-- > 0 )
    {
        char data[CPFS_BLKSIZE];

        rc = cpfs_disk_read( 0, blkpos++, data );
        if( rc )
        {
            cpfs_log_error("Can't read dir ino %d @ blk %d", dir_ino, blkpos-1 );
            return ENOENT;
        }

        int i;
        for( i = 0; i < CPFS_DIR_PER_BLK; i++ )
        {
            if( nentry-- <= 0 )
                return ENOENT;

            // NB! sizeof(struct cpfs_dir_entry) != CPFS_DIR_REC_SIZE
            struct cpfs_dir_entry *de = ((void *)data) + (i*CPFS_DIR_REC_SIZE);


            if( 0 == strcmp( fname, de->name ) )
            {
                *file_ino = de->inode;
                return 0;
            }

        }


    }

    return ENOENT;
}







// allocate a new dir entry in a dir

errno_t
cpfs_alloc_dirent( cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t file_ino ) 
{
    errno_t rc;
    char data[CPFS_BLKSIZE];

    // TODO some speedup? In-mem hash?

    // TODO assert(fname)

    // read in directory inode to find out dir file len

    /*
    struct cpfs_inode inode;
    struct cpfs_inode *inode_p = cpfs_lock_ino( dir_ino );
    if( inode_p ) inode = *inode_p;
    cpfs_unlock_ino( dir_ino );
    if( inode_p == 0 ) return EIO;

    int nentry = inode.fsize/CPFS_DIR_REC_SIZE;
    */

    cpfs_size_t fsize;
    if( cpfs_fsize( dir_ino, &fsize ) )
        return EIO;

    int nentry = fsize/CPFS_DIR_REC_SIZE;

    // TODO assert( (inode.fsize%CPFS_DIR_REC_SIZE) == 0 )

    int nblk = nentry / CPFS_DIR_PER_BLK;
    int blkpos = 0;

    // Scan sequentially through the dir looking for name

    while( nblk-- > 0 )
    {
        rc = cpfs_disk_read( 0, blkpos, data );
        if( rc )
        {
            cpfs_log_error("Can't read dir ino %d @ blk %d", dir_ino, blkpos );
            return EIO;
        }

        int i;
        for( i = 0; i < CPFS_DIR_PER_BLK; i++ )
        {
            if( nentry-- <= 0 )
                break;

            // NB! sizeof(struct cpfs_dir_entry) != CPFS_DIR_REC_SIZE
            struct cpfs_dir_entry *de = ((void *)data) + (i*CPFS_DIR_REC_SIZE);


            if( 0 == de->inode )
            {
                de->inode = file_ino;
                strlcpy( de->name, fname, sizeof(de->name) );
                return cpfs_disk_write( 0, blkpos, data );
            }

        }

        blkpos++;
    }

    // No free entry found, attempt to append one - we append one block, following calls will find rest of blk to be free

    memset( data, 0, sizeof( data ) );
    struct cpfs_dir_entry *de = ((void *)data);

    de->inode = file_ino;
    strlcpy( de->name, fname, sizeof(de->name) );
    return cpfs_disk_write( 0, nblk, data );

}

















