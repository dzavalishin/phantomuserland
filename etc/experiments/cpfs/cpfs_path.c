/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Path parsing.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"

/**
 *
 * Parse full file name, descend to last directory, return inode of the last directory and
 * pointer to final path element (dir/file name).
 *
**/

errno_t
cpfs_descend_dir( cpfs_fs_t *fs, const char *path, const char **last, cpfs_ino_t *last_dir_ino )
{
    errno_t ret;
    cpfs_ino_t cur_dir_ino = 0; // root dir inode
    cpfs_ino_t next_ino; // inode of el found in dir
    char name[CPFS_MAX_FNAME_LEN+1];
    const char *cur_name_start;

    cpfs_assert( last != 0 );
    cpfs_assert( last_dir_ino != 0 );
    cpfs_assert( fs != 0 );
    cpfs_assert( path != 0 );

    if( *path == '/' ) path++;

    cur_name_start = path;

    while(1)
    {
        if( ! *path ) return EINVAL; // ? can't happen? final stray '/'?

        const char *next_slash = index( path, '/' );

        // No more path elements?
        if( next_slash == 0 )
        {
            // Done
            *last_dir_ino = cur_dir_ino;
            *last = cur_name_start;
            //ast = next_slash+1;
            return 0;
        }

        unsigned int len = next_slash - cur_name_start;
        if( len >= CPFS_MAX_FNAME_LEN )
        {
            // Path name element is too long
            return EINVAL;
        }

        strncpy( name, cur_name_start, len );

        ret = cpfs_namei( fs, cur_dir_ino, name, &next_ino, 0 ); // find name
        printf("namei( \"%s\" ) = %d\n", name, ret );
        if( ret )
        {
            //printf("namei( %s ) = %d", name, ret );
            return ret;
        }

        // found, step in

        cur_name_start = next_slash+1; // Skip to the next part of path for next iteration
        cur_dir_ino = next_ino;
    }
}


errno_t
cpfs_mkdir( cpfs_fs_t *fs, const char *path )
{
    const char *last;
    cpfs_ino_t last_dir_ino;
    cpfs_ino_t new_dir_ino;
    errno_t rc;

    rc = cpfs_descend_dir( fs, path, &last, &last_dir_ino );
    if( rc ) return rc;

    rc = cpfs_alloc_inode( fs, &new_dir_ino );
    if( rc ) return rc;


    // Init inode as dir

    // TODO check all rc's here!

    struct cpfs_inode *rdi = cpfs_lock_ino( fs, new_dir_ino );
    if( rdi == 0 )
    {
        rc = EIO;
        goto free_inode;
    }

    cpfs_touch_ino( fs, new_dir_ino );
    cpfs_inode_init_defautls( fs, rdi );
    rdi->ftype = CPFS_FTYPE_DIR;
    rdi->nlinks = 1;
    cpfs_unlock_ino( fs, new_dir_ino );


    rc = cpfs_dir_scan( fs, last_dir_ino, last, &new_dir_ino, CPFS_DIR_SCAN_WRITE );
    if( rc ) goto free_inode;

    return 0;

free_inode:

    if( cpfs_free_inode( fs, new_dir_ino ) ) panic( "can't free inode %d", %d );
    return rc;
}

