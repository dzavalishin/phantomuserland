/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * FAT FS - based on ff code
 *
 *
**/

#if HAVE_UNIX


#define DEBUG_MSG_PREFIX "fat_ff_fs"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <unix/uufile.h>
//#include <unix/uuprocess.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <phantom_libc.h>

#include "../ff.h"



static errno_t fresult2errno(FRESULT fr);


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------


static size_t      fatff_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      fatff_write(   struct uufile *f, const void *src, size_t bytes);
static errno_t     fatff_stat( struct uufile *f, struct stat *dest );
static size_t      fatff_getpath( struct uufile *f, void *dest, size_t bytes);
static ssize_t     fatff_getsize( struct uufile *f);
static errno_t     fatff_setsize( struct uufile *f, size_t size);
static errno_t     fatff_seek(    struct uufile *f );
static errno_t     fatff_readdir( struct uufile *f, struct dirent *dirp );

//static void *      fatff_copyimpl( void *impl );



static struct uufileops fatff_fops =
{
    .read 	= fatff_read,
    .write 	= fatff_write,

    .readdir    = fatff_readdir,

    .seek       = fatff_seek,

    .getpath 	= fatff_getpath,
    .getsize 	= fatff_getsize,
    .setsize    = fatff_setsize,


//    .copyimpl   = fatff_copyimpl,

    .stat       = fatff_stat,
    //.ioctl      = fatff_ioctl,
};



// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------


static errno_t     fatff_open(struct uufile *, int create, int write);
static errno_t     fatff_close(struct uufile *);
static uufile_t *  fatff_namei(uufs_t *fs, const char *filename);
static uufile_t *  fatff_getRoot(uufs_t *fs);
static errno_t     fatff_dismiss(uufs_t *fs);
static errno_t     fatff_mkdir(struct uufs *fs, const char *path);


static struct uufs fatff_fs =
{
    .name       = "fatff",
    .open 	= fatff_open,
    .close 	= fatff_close,
    .namei 	= fatff_namei,
    .root 	= fatff_getRoot,
    .dismiss    = fatff_dismiss,
    .mkdir      = fatff_mkdir,

    .impl       = 0,
};



uufs_t *fatff_create_fs( FATFS *ffs )
{
    uufs_t *ret = calloc( 1, sizeof( uufs_t ) );

    memcpy( ret, &fatff_fs, sizeof( uufs_t ) );

    ret->impl = ffs;

    return ret;
}


// -----------------------------------------------------------------------
// FS methods
// -----------------------------------------------------------------------


static errno_t     fatff_open(struct uufile *f, int create, int write)
{
    (void) f;
    (void) create;
    (void) write;
    return 0;
}

static errno_t     fatff_close(struct uufile *f)
{
    FIL *fp = f->impl;

    SHOW_FLOW( 7, "fp %p fs %p dev %p", fp, fp->fs, fp->fs->dev );

    f_close( fp );

    return 0;
}

static uufile_t *  fatff_namei(uufs_t *fs, const char *filename)
{
    //bool isdir = 0;
    FATFS *ffs = fs->impl;
    FIL *fp = calloc( 1, sizeof(FIL) );
    DIR *dj = 0;

    FRESULT r = f_open ( ffs, fp, filename,
                         // TODO wrong. need real open's request here to handle create/open existing file
                         FA_READ|FA_WRITE
                       );

    if( r )
    {
        SHOW_FLOW( 7, "f_open %s res = %d", filename, r );
        free(fp);
        fp = 0;

        dj = calloc(1, sizeof(DIR));
        r = f_opendir ( ffs, dj, *filename ? filename : "/" );

        if( r )
        {
            free(dj);
            dj = 0;
            return 0;
        }
    }

    uufile_t *ret = create_uufile();

    ret->ops = &fatff_fops;
    ret->pos = 0;
    ret->fs = fs;
    ret->impl = fp ? (void *)fp : (void *)dj;

    if(!fp)
        ret->flags |= UU_FILE_FLAG_DIR;

    //ret->flags |= UU_FILE_FLAG_FREEIMPL;
    ret->flags |= UU_FILE_FLAG_OPEN; // TODO this is wrong and must be gone - open in open!

    set_uufile_name( ret, filename );

    return ret;
}

// Return a file struct for fs root
static uufile_t *  fatff_getRoot(uufs_t *fs)
{
    (void) fs;
    return 0;
    //return &fatff_root;
}

static errno_t  fatff_dismiss(uufs_t *fs)
{
    (void) fs;
    SHOW_ERROR0( 0, "not impl" );
    return 0;
}


static errno_t fatff_mkdir(struct uufs *fs, const char *path)
{
    FATFS *ffs = fs->impl;

    FRESULT r = f_mkdir( ffs, path );

    return fresult2errno(r);
}


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static errno_t       fatff_seek(    struct uufile *f )
{
    FIL *fp = f->impl;
    assert(fp);
    FRESULT r = f_lseek (fp,f->pos);
    return r ? EINVAL : 0;
}


static size_t      fatff_read(    struct uufile *f, void *dest, size_t bytes)
{
    if( f->flags & UU_FILE_FLAG_DIR )
        return -1;

    FRESULT r;
    unsigned int res;

    FIL *fp = f->impl;

    //r = f_lseek (fp,f->pos);
    //if( r )        return 0;

    r = f_read (
                fp, 		/* Pointer to the file object */
                dest,		/* Pointer to data buffer */
                bytes,		/* Number of bytes to read */
                &res		/* Pointer to number of bytes read */
               );

    if( r || res <= 0 )
        return 0;

    f->pos += res;
    return res;
}

static size_t      fatff_write(   struct uufile *f, const void *dest, size_t bytes)
{
    if( f->flags & UU_FILE_FLAG_DIR )
        return -1;

    FRESULT r;
    unsigned int res;

    FIL *fp = f->impl;

    r = f_write(fp, dest, bytes, &res );

    if( r || res <= 0 )
        return 0;

    f->pos += res;
    return res;
}

static errno_t     fatff_readdir( struct uufile *f, struct dirent *dirp )
{
    if( !(f->flags & UU_FILE_FLAG_DIR) )
        return ENOTDIR;

    DIR *dj = f->impl;

    FILINFO fi;
    char lname[FS_MAX_PATH_LEN];

    if( (f->pos++) == 0 )
    {
        // Make sure we start from start
        if( FR_OK != f_readdir ( dj, 0 ) )
            SHOW_ERROR( 0, "Can't rewind dir %p", f );
    }

    memset( &fi, 0, sizeof(fi) );
    fi.lfname = lname;
    fi.lfsize = sizeof(lname);
    memset( fi.lfname, 0, fi.lfsize );
    FRESULT r = f_readdir ( dj,	&fi );

    if(
       (fi.fsize == 0) &&
       (fi.fattrib == 0) &&
       (fi.lfname[0] == 0) &&
       (fi.fname[0] == 0)
      )
        return EIO;

    if( (r == FR_NO_FILE) || (dj->sect == 0) )
        return ENOENT; // End of dir

    if( r != FR_OK)
        return EIO; // Some other problem

    dirp->d_reclen = 0;
    dirp->d_ino = dj->sclust ? dj->sclust : ~0u ; // zero inode means unused entry, and zero cluster is root dir - replace with -1

    char *name = fi.lfname[0] ? fi.lfname : fi.fname;

    if( fi.fattrib & AM_DIR )
    {
        if( *name == 0 )
            name = ".";

        if( (name[0] == 1) && (name[1] == 0) )
            name = "..";
    }

    size_t nlen = strlen(name) + 1;
    if( nlen > sizeof(dirp->d_name) ) nlen = sizeof(dirp->d_name);

    strlcpy( dirp->d_name, name, nlen );
    dirp->d_namlen = nlen - 1;

    return 0;
}

static size_t      fatff_getpath( struct uufile *f, void *dest, size_t bytes)
{
    if( bytes == 0 ) return 0;
    // TODO get mountpoint
    // we keep name in impl
    strncpy( dest, f->impl, bytes );
    return strlen(dest);
}

// returns -1 for non-files
static ssize_t      fatff_getsize( struct uufile *f)
{
    if( !(f->flags & UU_FILE_FLAG_OPEN) )
        return -1;

    FIL *fp = f->impl;
    if( fp == 0 )
        return -1;

    return fp->fsize;
}

static errno_t fatff_setsize( struct uufile *f, size_t size)
{
    if( !(f->flags & UU_FILE_FLAG_OPEN) )
        return EINVAL;

    if( f->flags & UU_FILE_FLAG_DIR )
        return EISDIR;

    FIL *fp = f->impl;
    if( fp == 0 )
        return EINVAL;

    // mutex taken by caller
    FRESULT r = f_lseek ( fp, size );
    if( r ) return fresult2errno( r );

    return fresult2errno(f_truncate ( fp ));
}



static errno_t     fatff_stat( struct uufile *f, struct stat *dest )
{
    //FIL *fp = f->impl;
    FATFS *ffs = f->fs->impl;
    //const char *name = f->name;
    FILINFO fi;
    char lname[FS_MAX_PATH_LEN];
    fi.lfname = lname;

    const char *name = f->name;
    if( *name == 0 )
    {
        // f_stat fails to stat rootdir... FIXME
        memset( dest, 0, sizeof(struct stat) );

        dest->st_nlink = 1;
        dest->st_uid = -1;
        dest->st_gid = -1;

        dest->st_size = 1;

        dest->st_mode = 0555; // r-xr-xr-x
        dest->st_mode |= S_IFDIR;

        return 0;
    }

    SHOW_FLOW( 10, "stat '%s'", name );
    FRESULT r = f_stat ( ffs, name, &fi );
    SHOW_FLOW( 7, "stat lfname '%s' res = %d", lname, r );

    if(!r)
    {
        memset( dest, 0, sizeof(struct stat) );

        dest->st_nlink = 1;
        dest->st_uid = -1;
        dest->st_gid = -1;

        dest->st_size = fi.fsize;

        dest->st_mode = 0555; // r-xr-xr-x

        if(fi.fattrib & AM_DIR)            dest->st_mode |= S_IFDIR; else dest->st_mode |= _S_IFREG;

        if(!(fi.fattrib & AM_RDO))         dest->st_mode |= 0222;

        if( (fi.fattrib & AM_LFN) || (fi.fattrib & AM_VOL) )
            dest->st_mode &= ~ (_S_IFREG|S_IFDIR);
    }

    return fresult2errno(r);
}



/*
static void *      fatff_copyimpl( void *impl )
{
    void *dest = calloc( 1, sizeof(e2impl_t) );
    memcpy( dest, impl, sizeof(e2impl_t) );
    return dest;
}
*/



uufile_t *fatff_mount(errno_t *err, uufile_t *mount_point, uufile_t *device)
{
    (void) mount_point;
    (void) device;

    *err = ENXIO;
    return 0;
    /*

    uufile_t *ret = create_uufile();
    assert(ret);

    ret->ops = &fatff_fops;
    ret->pos = 0;
    ret->fs = &fatff_fs;
    ret->impl = calloc( 1, sizeof(f32_t) );
    ret->flags = UU_FILE_FLAG_MNT;

    assert(ret->impl);

    f32_t *f32 = ret->impl;

    f32->dev = device;

    if(!FAT32_InitFAT(f32,1))
    {
        destroy_uufile(ret);
        *err = EFTYPE;
        return 0;
    }

    FAT32_ShowFATDetails( f32 );

    return ret;
    */
}


errno_t fatff_umount(uufile_t *mount_point, uufile_t *device)
{
    (void) device;

    if( !(mount_point->flags & UU_FILE_FLAG_MNT) )
        return EBADF; // ?

    panic("fatff umount not impl");
}






static errno_t fresult2errno(FRESULT fr)
{
    switch(fr)
    {
    case FR_OK:         		return 0;
    case FR_DISK_ERR:                   return EIO;
    case FR_INT_ERR:                    return EFAULT;
    case FR_NOT_READY:                  return ENXIO;
    case FR_NO_FILE:                    return ENOENT;
    case FR_NO_PATH:                    return ENOENT;
    case FR_INVALID_NAME:               return ENOENT;
    case FR_DENIED:                     return EPERM;
    case FR_EXIST:                      return EEXIST;
    case FR_INVALID_OBJECT:             return EBADF;
    case FR_WRITE_PROTECTED:            return EACCES;
    case FR_INVALID_DRIVE:              return ENXIO;
    case FR_NOT_ENABLED:                return E2BIG;
    case FR_NO_FILESYSTEM:              return ENXIO;
    case FR_MKFS_ABORTED:               return ENXIO;
    case FR_TIMEOUT:                    return EAGAIN;
    case FR_LOCKED:                     return EBUSY;
    case FR_NOT_ENOUGH_CORE:            return ENOMEM;
    case FR_TOO_MANY_OPEN_FILES:        return EMFILE;
    }

    return ENODEV;
}






#endif // HAVE_UNIX

