/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Dev FS
 *
 *
**/

#if HAVE_UNIX


#define DEBUG_MSG_PREFIX "devfs"
#include "debug_ext.h"
#define debug_level_flow 7
#define debug_level_error 10
#define debug_level_info 10

#include <unix/uufile.h>
#include <sys/libkern.h>
#include <dirent.h>
#include <malloc.h>
#include <string.h>
#include "device.h"



// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------


static size_t      dev_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      dev_write(   struct uufile *f, const void *dest, size_t bytes);
static errno_t     dev_stat(    struct uufile *f, struct stat *dest );
static int         dev_ioctl(   struct uufile *f, errno_t *err, int request, void *data, int dlen );
static size_t      dev_getpath( struct uufile *f, void *dest, size_t bytes);
static ssize_t     dev_getsize( struct uufile *f);

static void *      dev_copyimpl( void *impl );


static struct uufileops dev_fops =
{
    .read 	= dev_read,
    .write 	= dev_write,

    .getpath 	= dev_getpath,
    .getsize 	= dev_getsize,

    .copyimpl   = dev_copyimpl,

    .stat       = dev_stat,
    .ioctl      = dev_ioctl,
};


// -----------------------------------------------------------------------
// Console impl - temp
// -----------------------------------------------------------------------


static size_t      con_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      con_write(   struct uufile *f, const void *dest, size_t bytes);

// Console - hack
static struct uufileops con_fops =
{
    .read 	= con_read,
    .write 	= con_write,

    .getpath 	= dev_getpath,
    .getsize 	= dev_getsize,

    .copyimpl   = dev_copyimpl,

    .stat       = dev_stat,
    //.ioctl      = dev_ioctl,
};



// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------


static errno_t     dev_open(struct uufile *, int create, int write);
static errno_t     dev_close(struct uufile *);

// Create a file struct for given path
static uufile_t *  dev_namei(uufs_t *fs, const char *filename);

// Return a file struct for fs root
static uufile_t *  dev_getRoot(uufs_t *fs);

static errno_t     dev_dismiss(uufs_t *fs);


struct uufs dev_fs =
{
    .name       = "dev",
    .open 	= dev_open,
    .close 	= dev_close,
    .namei 	= dev_namei,
    .root 	= dev_getRoot,
    .dismiss    = dev_dismiss,

    .impl       = 0,
};


static struct uufile dev_root =
{
    .ops 	= &dev_fops,
    .pos        = 0,
    .fs         = &dev_fs,
    .name       = "/",
    .flags      = UU_FILE_FLAG_DIR|UU_FILE_FLAG_NODESTROY,
    .impl       = 0, // dir will create if needed
};



// -----------------------------------------------------------------------
// FS methods
// -----------------------------------------------------------------------


static errno_t     dev_open(struct uufile *f, int create, int write)
{
    (void) create;
    (void) write;
    (void) f;

    return 0;
}

static errno_t     dev_close(struct uufile *f)
{
    (void) f;
    return 0;
}

// Create a file struct for given path
static uufile_t *  dev_namei(uufs_t *fs, const char *filename)
{
    (void) fs;
    SHOW_FLOW( 7, "dev lookup '%s'", filename );

    //struct uufileops *ops = 0;
    uufile_t *ret = 0;

    if( 0 == strcmp( filename, "tty" ) )
    {
        ret = create_uufile();
        ret->ops = &con_fops;
        ret->pos = 0;
        ret->fs = &dev_fs;
        ret->name = strdup( filename );
        ret->impl = 0;
        //ret->flags =

        SHOW_FLOW( 8, "dev created '%s'", filename );

        return ret;
    }

    if( (0 == strcmp( filename, "" )) || 0 == strcmp( filename, "/" ) )
    {
        ret = copy_uufile( &dev_root );
    }


    if(ret == 0)
    {
#define NPART 10

        const char *oname[NPART];
        int olen[NPART];

        int nb = uu_break_path( filename, NPART, oname, olen );

        if( nb > 2 || nb < 0 )
            return 0;

        char namebuf[FS_MAX_PATH_LEN];

        strlcpy( namebuf, oname[0], imin( olen[0]+1, FS_MAX_PATH_LEN ) );
        uufile_t *busf = lookup_dir( &dev_root, namebuf, 0, create_dir );

        if( nb == 1 )
        {
            ret = busf;
        }
        else
        {
            strlcpy( namebuf, oname[1], imin( olen[1]+1, FS_MAX_PATH_LEN ) );
            uufile_t *devf = lookup_dir( busf,      namebuf, 0, create_uufile );
            ret = devf;
        }

        // Separate copy for pos
        //link_uufile( ret );
        ret = copy_uufile( ret );
    }

    SHOW_FLOW( 7, "dev found '%s'", filename );

    return ret;
}

// Return a file struct for fs root
static uufile_t *  dev_getRoot(uufs_t *fs)
{
    (void) fs;
    return &dev_root;
}


static errno_t     dev_dismiss(uufs_t *fs)
{
    (void) fs;
    return 0;
}


static errno_t     dev_stat( struct uufile *f, struct stat *dest )
{
    memset( dest, 0, sizeof(struct stat) );

    dest->st_nlink = 1;
    dest->st_uid = -1;
    dest->st_gid = -1;

    dest->st_size = 0;

    dest->st_mode = 0555; // r-xr-xr-x

    if(f->flags && UU_FILE_FLAG_DIR)
        dest->st_mode |= _S_IFDIR;
    else
        dest->st_mode |= _S_IFCHR;

    return 0;
}


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static size_t      dev_read(    struct uufile *f, void *dest, size_t bytes)
{
    if(f->flags & UU_FILE_FLAG_DIR)
    {
        SHOW_FLOW( 11, "Read dir for %d bytes, pos %d", bytes, f->pos );

        struct dirent r;
        if( bytes < sizeof(struct dirent) )
            return -1;


        char namebuf[FS_MAX_PATH_LEN];
        if( get_dir_entry_name( f, f->pos++, namebuf ) )
            return 0;

        SHOW_FLOW( 7, "Read dir pos %d = '%s'", f->pos-1, namebuf );

        r.d_ino = -1; // not 0 for some programs treat 0 as no entry
        r.d_reclen = 0;
        strlcpy( r.d_name, namebuf, FILENAME_MAX );
        r.d_namlen = strlen( r.d_name );

        *((struct dirent*)dest) = r;

        return sizeof(struct dirent);
    }

    SHOW_FLOW( 10, "Read dev %d bytes", bytes );

    phantom_device_t* dev = f->impl;
    if(dev == 0 || dev->dops.read == 0) return -1;
    return dev->dops.read( dev, dest, bytes );
}

static size_t      dev_write(   struct uufile *f, const void *src, size_t bytes)
{
    if(f->flags && UU_FILE_FLAG_DIR)
        return -1;

    phantom_device_t* dev = f->impl;
    if(dev == 0 || dev->dops.write == 0) return -1;
    return dev->dops.write( dev, src, bytes );
}


static size_t      dev_getpath( struct uufile *f, void *dest, size_t bytes)
{
    if( bytes == 0 ) return 0;
    // TODO get mountpoint
    strncpy( dest, f->name, bytes );
    return strlen(dest);
}

// returns -1 for non-files
static ssize_t      dev_getsize( struct uufile *f)
{
    (void) f;

    return -1;
}

static void *      dev_copyimpl( void *impl )
{
    // TODO dev refcount
    return impl; // Just a pointer to dev, can copy
}


static int         dev_ioctl(   struct uufile *f, errno_t *err, int request, void *data, int dlen )
{
    phantom_device_t* dev = f->impl;
    if(dev == 0 || dev->dops.ioctl == 0) return -1;
    *err = dev->dops.ioctl( dev, request, data, dlen );
    return *err ? -1 : 0;
}


// -----------------------------------------------------------------------
// Console impl - temp
// -----------------------------------------------------------------------

static size_t      con_read(    struct uufile *f, void *dest, size_t bytes)
{
    (void) f;

    int ret = bytes;

    SHOW_FLOW( 10, "Read tty for %d bytes", bytes );

    char *cp = dest;
    while(bytes--)
        *cp++ = getchar();

    return ret;
}

static size_t      con_write(   struct uufile *f, const void *src, size_t bytes)
{
    (void) f;

    int ret = bytes;

    const char *cp = src;
    while(bytes--)
        putchar( *cp++ );

    return ret;
}

// -----------------------------------------------------------------------
// Main dev map
// -----------------------------------------------------------------------



void devfs_register_dev( phantom_device_t* dev )
{
    const char *busname = dev->bus ? dev->bus->name : "nobus";

    SHOW_FLOW( 7, "Registering dev %s.%d on bus %s in devfs",
               dev->name, dev->seq_number,
               busname
               );

    char devname[FS_MAX_MOUNT_PATH];
    snprintf( devname, FS_MAX_MOUNT_PATH, "%s%d", dev->name, dev->seq_number );

    uufile_t *busf = lookup_dir( &dev_root, busname, 1, create_dir );
    uufile_t *devf = lookup_dir( busf,      devname, 1, create_uufile );

    if(busf->fs == 0)
    {
        busf->fs = &dev_fs;
        busf->ops = &dev_fops;
    }

    if(devf->impl)
        SHOW_ERROR( 0, "Replacing dev %s on bus %s?", devname, busname );

    devf->impl = dev;
    devf->ops = &dev_fops;
    devf->pos = 0;
    devf->fs = &dev_fs;
    devf->name = strdup( devname );
}


#else

#include "device.h"

void devfs_register_dev( phantom_device_t* dev )
{
    (void) dev;
}

#endif // HAVE_UNIX
