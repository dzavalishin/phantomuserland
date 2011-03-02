/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * FAT FS
 *
 *
**/

#if HAVE_UNIX


#define DEBUG_MSG_PREFIX "fat32fs"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <unix/uufile.h>
//#include <unix/uuprocess.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <phantom_libc.h>

#include "unix/fs_fat32.h"
#include "unix/fat32/define.h"
#include "unix/fat32/FAT32_Access.h"





// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

//static bool		init_ext2(e2impl_t *impl);


static size_t      fat32_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      fat32_write(   struct uufile *f, const void *src, size_t bytes);
//static errno_t     fat32_stat(    struct uufile *f, struct ??);
//static errno_t     fat32_ioctl(   struct uufile *f, struct ??);

static size_t      fat32_getpath( struct uufile *f, void *dest, size_t bytes);

// returns -1 for non-files
static ssize_t     fat32_getsize( struct uufile *f);

//static void *      fat32_copyimpl( void *impl );


static struct uufileops fat32_fops =
{
    .read 	= fat32_read,
    .write 	= fat32_write,

    .getpath 	= fat32_getpath,
    .getsize 	= fat32_getsize,

//    .copyimpl   = fat32_copyimpl,

    //.stat       = fat32_stat,
    //.ioctl      = fat32_ioctl,
};



// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------


static errno_t     fat32_open(struct uufile *, int create, int write);
static errno_t     fat32_close(struct uufile *);
static uufile_t *  fat32_namei(uufs_t *fs, const char *filename);
static uufile_t *  fat32_getRoot(uufs_t *fs);
static errno_t     fat32_dismiss(uufs_t *fs);


struct uufs fat32_fs =
{
    .name       = "fat32",
    .open 	= fat32_open,
    .close 	= fat32_close,
    .namei 	= fat32_namei,
    .root 	= fat32_getRoot,
    .dismiss    = fat32_dismiss,

    .impl       = 0,
};





// -----------------------------------------------------------------------
// FS methods
// -----------------------------------------------------------------------


static errno_t     fat32_open(struct uufile *f, int create, int write)
{
    (void) f;
    (void) create;
    (void) write;
    return 0;
}

static errno_t     fat32_close(struct uufile *f)
{
    (void) f;
    return 0;
}

// Create a file struct for given path
static uufile_t *  fat32_namei(uufs_t *fs, const char *filename)
{
    (void) fs;
    (void) filename;
    return 0;
/*
    uufile_t *ret = create_uufile();

    ret->ops = &fat32_fops;
    ret->pos = 0;
    ret->fs = &fat32_fs;
    ret->impl = calloc( 1, sizeof(e2impl_t) );

    return ret;
*/
}

// Return a file struct for fs root
static uufile_t *  fat32_getRoot(uufs_t *fs)
{
    (void) fs;
    return 0;
    //return &fat32_root;
}

static errno_t  fat32_dismiss(uufs_t *fs)
{
    (void) fs;
    // TODO impl
    return 0;
}


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static size_t      fat32_read(    struct uufile *f, void *dest, size_t bytes)
{
    (void) f;
    (void) dest;
    (void) bytes;
    return -1;
}

static size_t      fat32_write(   struct uufile *f, const void *dest, size_t bytes)
{
    (void) f;
    (void) dest;
    (void) bytes;
    return -1;
}

//static errno_t     fat32_stat(    struct uufile *f, struct ??);
//static errno_t     fat32_ioctl(   struct uufile *f, struct ??);

static size_t      fat32_getpath( struct uufile *f, void *dest, size_t bytes)
{
    if( bytes == 0 ) return 0;
    // TODO get mountpoint
    // we keep name in impl
    strncpy( dest, f->impl, bytes );
    return strlen(dest);
}

// returns -1 for non-files
static ssize_t      fat32_getsize( struct uufile *f)
{
    (void) f;
    return -1;
}

/*
static void *      fat32_copyimpl( void *impl )
{
    void *dest = calloc( 1, sizeof(e2impl_t) );
    memcpy( dest, impl, sizeof(e2impl_t) );
    return dest;
}
*/

uufile_t *fat32_mount(errno_t *err, uufile_t *mount_point, uufile_t *device)
{
    (void) mount_point;


    uufile_t *ret = create_uufile();
    assert(ret);

    ret->ops = &fat32_fops;
    ret->pos = 0;
    ret->fs = &fat32_fs;
    ret->impl = calloc( 1, sizeof(f32_t) );
    ret->flags = UU_FILE_FLAG_MNT;

    assert(ret->impl);

    f32_t *f32 = ret->impl;

    f32->dev = device;

    if(!FAT32_InitFAT(f32,1))
    {
        unlink_uufile(ret);
        *err = EFTYPE;
        return 0;
    }

    FAT32_ShowFATDetails( f32 );

    return ret;

}


errno_t fat32_umount(uufile_t *mount_point, uufile_t *device)
{
    (void) device;

    if( !(mount_point->flags & UU_FILE_FLAG_MNT) )
        return EBADF; // ?

    panic("fat 32 umount not impl");
}










#endif // HAVE_UNIX

