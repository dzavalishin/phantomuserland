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
#define debug_level_flow 10
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
//static errno_t     fatff_stat(    struct uufile *f, struct ??);
//static errno_t     fatff_ioctl(   struct uufile *f, struct ??);

static size_t      fatff_getpath( struct uufile *f, void *dest, size_t bytes);

// returns -1 for non-files
static ssize_t     fatff_getsize( struct uufile *f);

//static void *      fatff_copyimpl( void *impl );

static errno_t     fatff_stat( struct uufile *f, struct stat *dest );


static struct uufileops fatff_fops =
{
    .read 	= fatff_read,
    .write 	= fatff_write,

    .getpath 	= fatff_getpath,
    .getsize 	= fatff_getsize,

//    .copyimpl   = fatff_copyimpl,

    .stat       = fatff_stat,
    //.ioctl      = fatff_ioctl,
};



// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------


//static uufile_t *  fatff_open(const char *name, int create, int write);
static errno_t     fatff_open(struct uufile *, int create, int write);
static errno_t     fatff_close(struct uufile *);

// Create a file struct for given path
static uufile_t *  fatff_namei(uufs_t *fs, const char *filename);

// Return a file struct for fs root
static uufile_t *  fatff_getRoot(uufs_t *fs);
static errno_t     fatff_dismiss(uufs_t *fs);


static struct uufs fatff_fs =
{
    .name       = "fatff",
    .open 	= fatff_open,
    .close 	= fatff_close,
    .namei 	= fatff_namei,
    .root 	= fatff_getRoot,
    .dismiss    = fatff_dismiss,

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
    //FIL *fp = f->impl;
    //FATFS *ffs = f->fs->impl;

    (void) f;
    (void) create;
    (void) write;
    return 0;
}

static errno_t     fatff_close(struct uufile *f)
{
    FIL *fp = f->impl;
    //FATFS *ffs = f->fs->impl;

    f_close( fp );

    if( f->impl )
    {
        free(f->impl);
        f->impl = 0;
    }

    unlink_uufile( f );
    return 0;
}

// Create a file struct for given path
static uufile_t *  fatff_namei(uufs_t *fs, const char *filename)
{
    FATFS *ffs = fs->impl;
    FIL *fp = calloc( 1, sizeof(FIL) );

    FRESULT r = f_open ( ffs, fp, filename,
                         // TODO wrong. need real open's request here to handle create/open existing fail
                         FA_READ|FA_WRITE
                       );

    if( r )
    {
        SHOW_FLOW( 1, "f_open %s res = %d", filename, r );
        free(fp);
        return 0;
    }

    uufile_t *ret = create_uufile();

    ret->ops = &fatff_fops;
    ret->pos = 0;
    ret->fs = fs;
    ret->impl = fp;

    set_uufile_name( ret, filename );

    return ret;
}

// Return a file struct for fs root
static uufile_t *  fatff_getRoot(uufs_t *fs)
{
    return 0;
    //return &fatff_root;
}

static errno_t  fatff_dismiss(uufs_t *fs)
{
    // free( fs->impl );
    // TODO impl
    return 0;
}


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static size_t      fatff_read(    struct uufile *f, void *dest, size_t bytes)
{
    FIL *fp = f->impl;
    //FATFS *ffs = f->fs->impl;

    FRESULT r = f_lseek (
                         fp,		/* Pointer to the file object */
                         f->pos		/* File pointer from top of file */
                        );

    if( r )
        return 0;

    unsigned int res;

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
    (void) f;
    (void) dest;
    (void) bytes;
    return -1;
}

//static errno_t     fatff_stat(    struct uufile *f, struct ??);
//static errno_t     fatff_ioctl(   struct uufile *f, struct ??);

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
    (void) f;
    return -1;
}



static errno_t     fatff_stat( struct uufile *f, struct stat *dest )
{
    //FIL *fp = f->impl;
    FATFS *ffs = f->fs->impl;
    const char *name = f->name;
    FILINFO fi;

    SHOW_FLOW( 1, "stat %s", name );
    FRESULT r = f_stat ( ffs, name, &fi );
    SHOW_FLOW( 1, "stat res = %d", r );

    if(!r)
    {
        memset( dest, 0, sizeof(struct stat) );

        dest->st_nlink = 1;
        dest->st_uid = -1;
        dest->st_gid = -1;

        dest->st_size = fi.fsize;

        dest->st_mode = 0555; // r-xr-xr-x

        if(fi.fattrib & AM_DIR)            dest->st_mode |= S_IFDIR; else dest->st_mode = _S_IFREG;

        if(!(fi.fattrib & AM_RDO))         dest->st_mode |= 0222;

        if( (fi.fattrib & AM_LFN) || (fi.fattrib & AM_VOL) )
            dest->st_mode &= ~ (_S_IFREG||S_IFDIR);

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

