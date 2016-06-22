/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Interface impl.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"

#include <fcntl.h> // O_CREAT


// TODO can unlink open file as for now! FIXME BUG

struct fid
{
    cpfs_ino_t  inode;
};

errno_t
cpfs_file_open  ( int *file_id, const char *name, int flags, void * user_id_data )
{
    errno_t rc;
    cpfs_ino_t file_ino;

    //
    // Attempt top open existing one
    //

    // TODO nested dirs! :)

    //rc = cpfs_namei( cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t *file_ino ); // find name
    rc = cpfs_namei( 0, name, &file_ino );

    if(rc && (flags & O_CREAT))
    {
        //
        // Creating file
        //

        file_ino = cpfs_alloc_inode();
        if( file_ino == 0 )
            return EMFILE;


        rc = cpfs_alloc_dirent( 0, name, file_ino ); // allocate a new dir entry in a dir
        if( rc )
            cpfs_free_inode( file_ino );
    }

    if(rc) return rc;

    // TODO inode usage count
    struct fid *f = calloc( sizeof( struct fid ), 1 );
    if( 0 == f ) return ENOMEM;

    f->inode = file_ino;

    *file_id = (int) f;
    return 0;
}


errno_t
cpfs_file_close ( int file_id )
{
    // struct fid *f = (struct fid *) file_id;
    // TODO write me

}



errno_t
cpfs_file_read  ( int file_id, cpfs_size_t pos, void *data, cpfs_size_t size )
{
    struct fid *f = (struct fid *) file_id;
}



errno_t
cpfs_file_write ( int file_id, cpfs_size_t pos, const void *data, cpfs_size_t size )
{
    struct fid *f = (struct fid *) file_id;
}



