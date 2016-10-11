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

//#include <fcntl.h> // O_CREAT


#define USE_FDMAP 1

int TRACE=1;
int TRACE_TAB=1;

#ifndef __POK_LIBC_STDIO_H__
FILE *fsck_scan_dir_log_file;
FILE *fsck_scan_ino_log_file;
#endif // __POK_LIBC_STDIO_H__



// ----------------------------------------------------------------------------
//
// Entry point - open
//
// ----------------------------------------------------------------------------

errno_t
cpfs_file_open( cpfs_fs_t *fs, int *file_id, const char *full_name, int flags, void * user_id_data )
{
    errno_t rc;
    cpfs_ino_t file_ino;
    const char *last;
    cpfs_ino_t last_dir_ino;

    rc = cpfs_os_access_rights_check( fs, (flags & (O_CREAT|O_WRONLY|O_RDWR)) ? cpfs_r_write : cpfs_r_read, user_id_data, full_name );
    if( rc ) return rc;

    //
    // Attempt to open existing one
    //

    rc = cpfs_descend_dir( fs, full_name, &last, &last_dir_ino );
    if( rc ) return rc;

    //rc = cpfs_namei( cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t *file_ino ); // find name
    rc = cpfs_namei( fs, last_dir_ino, last, &file_ino );

    if(rc && (flags & O_CREAT))
    {
        //
        // Creating file
        //

        if( cpfs_alloc_inode( fs, &file_ino ) )
            return ENOSPC; //EMFILE;


        rc = cpfs_alloc_dirent( fs, last_dir_ino, last, file_ino ); // allocate a new dir entry in a dir
        if( rc )
            cpfs_free_inode( fs, file_ino );
    }

    if(rc) return rc;

    cpfs_update_ino_atime( fs, file_ino ); // TODO rc

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




// ----------------------------------------------------------------------------
//
// Entry point - close
//
// ----------------------------------------------------------------------------



errno_t
cpfs_file_close( int file_id )
{
    errno_t rc = cpfs_fdmap_free( file_id );

    if( rc ) printf("can't close fd %d\n", file_id );

    return rc;

}


// ----------------------------------------------------------------------------
//
// Entry point - read
//
// ----------------------------------------------------------------------------



errno_t
cpfs_file_read( int file_id, cpfs_size_t pos, void *data, cpfs_size_t size )
{
    cpfs_ino_t ino;
    cpfs_fs_t *fs;
    errno_t rc = cpfs_fdmap_get( file_id, &ino, &fs );
    if( rc ) return rc;
cpfs_blkno_t phys_blk;
    return cpfs_ino_file_read( fs, ino, pos, data, size, &phys_blk );
}



// ----------------------------------------------------------------------------
//
// Entry point - write
//
// ----------------------------------------------------------------------------


errno_t
cpfs_file_write( int file_id, cpfs_size_t pos, const void *data, cpfs_size_t size )
{
    cpfs_ino_t ino;
    cpfs_fs_t *fs;
    errno_t rc = cpfs_fdmap_get( file_id, &ino, &fs );
    if( rc ) return rc;

    return cpfs_ino_file_write( fs, ino, pos, data, size );
}




