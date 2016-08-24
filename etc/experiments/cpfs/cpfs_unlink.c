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

    rc = cpfs_descend_dir( fs, full_name, &last, &last_dir_ino );
    if( rc ) return rc;

    cpfs_ino_t ret;
    rc = cpfs_namei( fs, last_dir_ino, last, &ret, 1 );

    return rc;
}

