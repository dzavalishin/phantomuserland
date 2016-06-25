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


#define USE_FDMAP 1


// TODO can unlink open file as for now! FIXME BUG


errno_t
cpfs_file_open( cpfs_fs_t *fs, int *file_id, const char *name, int flags, void * user_id_data )
{
    errno_t rc;
    cpfs_ino_t file_ino;

    //
    // Attempt top open existing one
    //

    // TODO nested dirs! :)

    //rc = cpfs_namei( cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t *file_ino ); // find name
    rc = cpfs_namei( fs, 0, name, &file_ino, 0 );

    if(rc && (flags & O_CREAT))
    {
        //
        // Creating file
        //

        if( cpfs_alloc_inode( fs, &file_ino ) )
            return EMFILE;


        rc = cpfs_alloc_dirent( fs, 0, name, file_ino ); // allocate a new dir entry in a dir
        if( rc )
            cpfs_free_inode( fs, file_ino );
    }

    if(rc) return rc;

#if USE_FDMAP
    rc = cpfs_fdmap_alloc( fs, file_ino, file_id );
    if( rc ) return rc; // TODO close file, or better do it before all

    //cpfs_fid_t *fid;
    //cpfs_fdmap_lock( *file_id, &fid );

    //cpfs_fdmap_unlock( *file_id );
#else
    struct fid *f = calloc( sizeof( struct fid ), 1 );
    if( 0 == f ) return ENOMEM;

    f->fs = fs;
    f->inode = file_ino;

    *file_id = (int) f;
#endif
    return 0;
}


errno_t
cpfs_file_close( int file_id )
{
    //struct fid *f = (struct fid *) file_id;
    //free( f );

    //cpfs_ino_t ino;
    //errno_t rc = cpfs_fdmap_get_inode( file_id, &ino );
    //if( rc ) return rc;

    return cpfs_fdmap_free( file_id );

}



errno_t
cpfs_file_read( int file_id, cpfs_size_t pos, void *data, cpfs_size_t size )
{
    //struct fid *f = (struct fid *) file_id;
    //cpfs_ino_t ino = f->inode;

    cpfs_ino_t ino;
    cpfs_fs_t *fs;
    errno_t rc = cpfs_fdmap_get( file_id, &ino, &fs );
    if( rc ) return rc;

    return cpfs_ino_file_read( fs, ino, pos, data, size );
}



errno_t
cpfs_file_write( int file_id, cpfs_size_t pos, const void *data, cpfs_size_t size )
{
    //struct fid *f = (struct fid *) file_id;
    //cpfs_ino_t ino = f->inode;

    cpfs_ino_t ino;
    cpfs_fs_t *fs;
    errno_t rc = cpfs_fdmap_get( file_id, &ino, &fs );
    if( rc ) return rc;

    return cpfs_ino_file_write( fs, ino, pos, data, size );
}



