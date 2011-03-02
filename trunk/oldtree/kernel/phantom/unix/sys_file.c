/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Unix syscalls - file io
 *
 *
**/


#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "funix"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <unix/uufile.h>
#include <unix/uuprocess.h>
#include <kernel/unix.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <phantom_types.h>

#include "unix/fs_pipe.h"

//int uu_find_fd( uuprocess_t *u, uufile_t * f );



int usys_open( int *err, uuprocess_t *u, const char *name, int flags, int mode )
{
    SHOW_FLOW( 7, "open '%s'", name );

    uufile_t * f = uu_namei( name );
    if( f == 0 )
    {
        *err = ENOENT;
        return -1;
    }

    mode &= ~u->umask;

    // TODO pass mode to open
    if( (f->fs == 0) || (f->fs->open == 0) )
        goto unlink;

    *err = f->fs->open( f, flags & O_CREAT, (flags & O_WRONLY) || (flags & O_RDWR) );
    if( *err )
        goto unlink;

    f->flags |= UU_FILE_FLAG_OPEN;

    int fd = uu_find_fd( u, f );

    if( fd < 0 )
    {
        *err = EMFILE;
        goto unlink;
    }

    return fd;

unlink:
    unlink_uufile( f );
    return -1;
}

int usys_creat( int *err, uuprocess_t *u, const char *name, int mode )
{
    return usys_open( err, u, name, O_CREAT|O_WRONLY|O_TRUNC, mode );
}

int usys_read(int *err, uuprocess_t *u, int fd, void *addr, int count )
{
    CHECK_FD(fd);
    struct uufile *f = GETF(fd);

    if( (f->ops == 0) || (f->ops->read == 0) )
    {
        *err = ENXIO;
        return -1;
    }

    int ret = f->ops->read( f, addr, count );

    if( ret < 0 ) *err = EIO;
    return ret;
}

int usys_write(int *err, uuprocess_t *u, int fd, const void *addr, int count )
{
    CHECK_FD(fd);
    struct uufile *f = GETF(fd);

    if( (f->ops == 0) || (f->ops->write == 0) )
    {
        *err = ENXIO;
        return -1;
    }

    int ret = f->ops->write( f, addr, count );

    if( ret < 0 ) *err = EIO;
    return ret;
}

int usys_close(int *err, uuprocess_t *u, int fd )
{
    CHECK_FD(fd);
    struct uufile *f = GETF(fd);

    uufs_t *fs = f->fs;
    assert(fs->close != 0);
    *err = fs->close( f );

    u->fd[fd] = 0;

    return err ? -1 : 0;
}

int usys_lseek( int *err, uuprocess_t *u, int fd, int offset, int whence )
{
    CHECK_FD(fd);
    struct uufile *f = GETF(fd);

    ssize_t size = f->ops->getsize( f );

    if(size < 0)
    {
        *err = ESPIPE;
        return -1;
    }

    off_t pos = offset;

    switch(whence)
    {
    case SEEK_SET: break;
    case SEEK_CUR: pos += f->pos;
    case SEEK_END: pos += size;
    }

    if(pos < 0)
    {
        *err = EINVAL;
        return -1;
    }

    f->pos = pos;
    return f->pos;
}





int usys_fchmod( int *err, uuprocess_t *u, int fd, int mode )
{
    CHECK_FD(fd);
    struct uufile *f = GETF(fd);

    if( f->ops->chmod == 0)
    {
        *err = ENOSYS;
        goto err;
    }

    if( (f->ops == 0) || (f->ops->chmod == 0) )
    {
        *err = ENXIO;
        return -1;
    }

    *err = f->ops->chmod( f, mode );
err:
    return *err ? -1 : 0;
}

int usys_fchown( int *err, uuprocess_t *u, int fd, int user, int grp )
{
    CHECK_FD(fd);
    struct uufile *f = GETF(fd);

    if( (f->ops == 0) || (f->ops->chown == 0) )
    {
        *err = ENXIO;
        return -1;
    }

    if( f->ops->chown == 0)
    {
        *err = ENOSYS;
        goto err;
    }

    *err = f->ops->chown( f, user, grp );
err:
    return *err ? -1 : 0;
}



int usys_ioctl( int *err, uuprocess_t *u, int fd, int request, void *data )
{
    CHECK_FD(fd);
    struct uufile *f = GETF(fd);

    if( (f->ops == 0) )
    {
        *err = ENXIO;
        return -1;
    }

    if( !f->ops->ioctl )
    {
        *err = ENOTTY;
        return -1;
    }

    return f->ops->ioctl( f, err, request, data );
}


int usys_stat( int *err, uuprocess_t *u, const char *path, struct stat *data, int statlink )
{
	(void) u;
	(void) statlink;


    uufile_t * f = uu_namei( path );
    if( f == 0 )
    {
        *err = ENOENT;
        return -1;
    }

    if( (f->ops == 0) || (f->ops->stat == 0) )
    {
        *err = ENXIO; // or what?
        return -1;
    }

    *err = f->ops->stat( f, data );

    unlink_uufile( f );
    return *err ? -1 : 0;
}


int usys_fstat( int *err, uuprocess_t *u, int fd, struct stat *data, int statlink )
{
    (void) statlink;

    CHECK_FD(fd);
    struct uufile *f = GETF(fd);

    if( (!f->ops) || (!f->ops->stat) )
    {
        *err = ENXIO; // or what?
        return -1;
    }

    *err = f->ops->stat( f, data );
    return *err ? -1 : 0;
}


int usys_truncate( int *err, uuprocess_t *u, const char *path, off_t length)
{
    (void) u;

    uufile_t * f = uu_namei( path );
    if( f == 0 )
    {
        *err = ENOENT;
        return -1;
    }

    if( (!f->ops) || ( !f->ops->setsize ) )
    {
        *err = ENXIO; // or what?
        return -1;
    }

    *err = f->ops->setsize( f, length );

    unlink_uufile( f );
    return *err ? -1 : 0;
}

int usys_ftruncate(int *err, uuprocess_t *u, int fd, off_t length)
{
    CHECK_FD(fd);
    struct uufile *f = GETF(fd);

    if( (!f->ops) || ( !f->ops->setsize ) )
    {
        *err = ENXIO; // or what?
        return -1;
    }

    *err = f->ops->setsize( f, length );
    return *err ? -1 : 0;
}


// -----------------------------------------------------------------------
// Dirs
// -----------------------------------------------------------------------

int usys_chdir( int *err, uuprocess_t *u,  const char *in_path )
{
    char path[FS_MAX_PATH_LEN];
    if( uu_absname(path, u->cwd_path, in_path ) )
    {
        return ENOENT;
    }

    SHOW_FLOW( 8, "in '%s' cd '%s' abs '%s'", in_path, u->cwd_path, path );

    uufile_t * f = uu_namei( path );
    if( f == 0 )
    {
        SHOW_ERROR( 1, "namei '%s' failed", path );
        *err = ENOENT;
        return -1;
    }

    if( !(f->flags & UU_FILE_FLAG_DIR) )
    {
        SHOW_ERROR( 1, "namei '%s' failed", path );
        *err = ENOTDIR;
        goto err;
    }


#if 0
    if( f->ops->stat == 0)
    {
        *err = ENOTDIR;
        goto err;
    }

    struct stat sb;
    *err = f->ops->stat( f, &sb );
    if( *err )
        goto err;

    if( sb.st_mode & _S_IFDIR)
    {
#endif
        if(u->cwd_file)
            unlink_uufile( u->cwd_file );

        u->cwd_file = f;
        //uu_absname( u->cwd_path, u->cwd_path, in_path );
        strlcpy( u->cwd_path, path, FS_MAX_PATH_LEN );
        SHOW_FLOW( 1, "cd to '%s'", path );
        return 0;
#if 0
    }

    *err = ENOTDIR;
#endif
err:
    unlink_uufile( f );
    return -1;
}

int usys_fchdir( int *err, uuprocess_t *u,  int fd )
{
    CHECK_FD(fd);
    struct uufile *f = GETF(fd);

    if( f->flags & UU_FILE_FLAG_DIR )
    {
        *err = ENOTDIR;
        goto err;
    }

#if 0
    if( f->ops->stat == 0)
    {
        *err = ENOTDIR;
        goto err;
    }

    struct stat sb;
    *err = f->ops->stat( f, &sb );
    if( *err )
        goto err;

    if( sb.st_mode & _S_IFDIR)
    {
#endif
        if(u->cwd_file)
            unlink_uufile( u->cwd_file );
        u->cwd_file = copy_uufile( f );

        u->cwd_path[0] = 0;
        if(u->cwd_file->ops->getpath)
            u->cwd_file->ops->getpath( u->cwd_file, u->cwd_path, FS_MAX_PATH_LEN );


        return 0;
#if 0
    }

    *err = ENOTDIR;
#endif
err:
    return -1;
}


int usys_getcwd( int *err, uuprocess_t *u, char *buf, int bufsize )
{
    ssize_t len = strlen(u->cwd_path);
    SHOW_FLOW( 9, "bs %d len %d", bufsize, len );
    if(bufsize < len+1)
    {
        *err = EINVAL;
        return -1;
    }

    *buf = 0;
#if 1
    strlcpy( buf, u->cwd_path, bufsize );
    return len+1;
#else
    if(!u->cwd)
    {
        strlcpy( buf, "/", bufsize );
        return 0;
    }

    //size_t ret =
    u->cwd->ops->getpath( u->cwd, buf, bufsize );

    return 0;
#endif
}



int usys_readdir(int *err, uuprocess_t *u, int fd, struct dirent *dirp )
{
    CHECK_FD(fd);
    struct uufile *f = GETF(fd);

    if( f->flags & UU_FILE_FLAG_DIR )
    {
        *err = ENOTDIR;
        return -1;
    }

    if( (!f->ops) || ( !f->ops->read ) )
    {
        *err = ENXIO; // or what?
        return -1;
    }

    int len = f->ops->read( f, dirp, sizeof(struct dirent) );

    if( len == 0 )
        return 0;

    if( len == sizeof(struct dirent) )
        return 1;

    *err = EIO;
    return -1;
}


int usys_pipe(int *err, uuprocess_t *u, int *fds )
{
    uufile_t *f1;
    uufile_t *f2;

    pipefs_make_pipe( &f1, &f2 );

    int fd1 = uu_find_fd( u, f1 );
    int fd2 = uu_find_fd( u, f2 );

    if( (fd1 < 0) || (fd2 < 0)  )
    {
        unlink_uufile( f1 );
        unlink_uufile( f2 );
        *err = EMFILE;
        return -1;
    }

    fds[0] = fd1;
    fds[1] = fd2;

    return 0;
}


// -----------------------------------------------------------------------
// others
// -----------------------------------------------------------------------


int usys_rm( int *err, uuprocess_t *u, const char *name )
{
    (void) u;
    uufile_t * f = uu_namei( name );
    if( f == 0 )
    {
        *err = ENOENT;
        return -1;
    }

    if( 0 == f->ops->unlink)
    {
        *err = ENXIO;
        return -1;
    }

    *err = f->ops->unlink( f );
    unlink_uufile( f );

    if( *err )
        return -1;

    return 0;
}

int usys_dup2(int *err, uuprocess_t *u, int src_fd, int dst_fd )
{
    // TODO lock fd[] access

    CHECK_FD_RANGE(dst_fd);
    CHECK_FD(src_fd);

    struct uufile *f = GETF(src_fd);

    if( u->fd[dst_fd] != 0)
        usys_close( err, u, dst_fd );

    link_uufile( f );
    u->fd[dst_fd] = f;

    return 0;
}

int usys_dup(int *err, uuprocess_t *u, int src_fd )
{
    // TODO lock fd[] access
    CHECK_FD(src_fd);

    struct uufile *f = GETF(src_fd);

    link_uufile( f );

    int fd = uu_find_fd( u, f  );

    if( fd < 0 )
    {
        unlink_uufile( f );
        *err = EMFILE;
        return -1;
    }

    return fd;
}

int usys_symlink(int *err, uuprocess_t *u, const char *src, const char *dst )
{
    (void) u;
    char rest[FS_MAX_PATH_LEN];

    uufs_t * fs = uu_findfs( src, rest );
    if( fs == 0 )
    {
        *err = ENOENT;
        return -1;
    }

    if(fs->symlink == 0)
        *err = ENXIO;
    else
        *err = fs->symlink( fs, rest, dst );
    return *err ? -1 : 0;
}


int usys_mkdir( int *err, uuprocess_t *u, const char *path )
{
    (void) u;
    char rest[FS_MAX_PATH_LEN];

    uufs_t * fs = uu_findfs( path, rest );
    if( fs == 0 )
    {
        *err = ENOENT;
        return -1;
    }

    if(fs->mkdir == 0)
        *err = ENXIO;
    else
        *err = fs->mkdir( fs, rest );
    return *err ? -1 : 0;
}




// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

// BUG - mutex!
int uu_find_fd( uuprocess_t *u, uufile_t *f  )
{
    int i;
    for( i = 0; i < MAX_UU_FD; i++ )
    {
        if( u->fd[i] == 0 )
        {
            u->fd[i] = f;
            return i;
        }
    }

    return -1;
}


#endif // HAVE_UNIX

