/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Unlink (delete) file/dir.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"



errno_t
cpfs_file_unlink( struct cpfs_fs *fs, const char *full_name, void * user_id_data )
{
    const char *last;
    cpfs_ino_t last_dir_ino;
    errno_t rc;
    //cpfs_ino_t ret;

    rc = cpfs_descend_dir( fs, full_name, &last, &last_dir_ino );
    if( rc ) return rc;

    // TODO check if it is a directory, check non-empty
#if 1
    cpfs_ino_t possible_dir_ino;
    rc = cpfs_namei( fs, last_dir_ino, last, &possible_dir_ino );
    if( rc ) return rc;

    int isdir = 0;
    rc = cpfs_is_dir( fs, possible_dir_ino, &isdir ); // Check if inode contains a directory

    if( isdir )
    {
    	rc = cpfs_is_empty_dir( fs, possible_dir_ino );
    	if( rc ) return rc;
    }
#endif

    rc = cpfs_free_dirent( fs, last_dir_ino, last );

    return rc;
}

