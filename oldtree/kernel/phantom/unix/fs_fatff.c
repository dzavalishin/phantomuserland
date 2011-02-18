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





// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------


static size_t      fatff_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      fatff_write(   struct uufile *f, void *dest, size_t bytes);
//static errno_t     fatff_stat(    struct uufile *f, struct ??);
//static errno_t     fatff_ioctl(   struct uufile *f, struct ??);

static size_t      fatff_getpath( struct uufile *f, void *dest, size_t bytes);

// returns -1 for non-files
static ssize_t     fatff_getsize( struct uufile *f);

//static void *      fatff_copyimpl( void *impl );


static struct uufileops fatff_fops =
{
    .read 	= fatff_read,
    .write 	= fatff_write,

    .getpath 	= fatff_getpath,
    .getsize 	= fatff_getsize,

//    .copyimpl   = fatff_copyimpl,

    //.stat       = fatff_stat,
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
    return 0;
}

// Create a file struct for given path
static uufile_t *  fatff_namei(uufs_t *fs, const char *filename)
{
    FATFS *ffs = fs->impl;
    FIL *fp = calloc( 1, sizeof(FIL) );

    FRESULT r = f_open (
                      ffs,
                      fp,			/* Pointer to the blank file object */
                      filename,	/* Pointer to the file name */

                      // TODO wrong. need real open's request here to handle create/open existing fail
                      FA_READ|FA_WRITE			/* Access mode and file open mode flags */
                     );

    if( r )
    {
        free(fp);
        return 0;
    }

    uufile_t *ret = create_uufile();

    ret->ops = &fatff_fops;
    ret->pos = 0;
    ret->fs = fs;
    ret->impl = fp;

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

static size_t      fatff_write(   struct uufile *f, void *dest, size_t bytes)
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










#endif // HAVE_UNIX

