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
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <unix/uufile.h>
//#include <unix/uuprocess.h>
#include <malloc.h>
#include <string.h>
#include "device.h"



// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------


static size_t      dev_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      dev_write(   struct uufile *f, const void *dest, size_t bytes);
//static errno_t     dev_stat(    struct uufile *f, struct ??);
//static errno_t     dev_ioctl(   struct uufile *f, struct ??);

static size_t      dev_getpath( struct uufile *f, void *dest, size_t bytes);

// returns -1 for non-files
static ssize_t     dev_getsize( struct uufile *f);

static void *      dev_copyimpl( void *impl );


static struct uufileops dev_fops =
{
    .read 	= dev_read,
    .write 	= dev_write,

    .getpath 	= dev_getpath,
    .getsize 	= dev_getsize,

    .copyimpl   = dev_copyimpl,

    //.stat       = dev_stat,
    //.ioctl      = dev_ioctl,
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

    //.stat       = dev_stat,
    //.ioctl      = dev_ioctl,
};



// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------


//static uufile_t *  dev_open(const char *name, int create, int write);
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
    .flags      = UU_FILE_FLAG_DIR,
    .impl       = 0, // dir will create if needed
};



// -----------------------------------------------------------------------
// FS methods
// -----------------------------------------------------------------------


static errno_t     dev_open(struct uufile *f, int create, int write)
{
	(void) create;
	(void) write;

    link_uufile( f );
    return 0;
}

static errno_t     dev_close(struct uufile *f)
{
    unlink_uufile( f );
    return 0;
}

// Create a file struct for given path
static uufile_t *  dev_namei(uufs_t *fs, const char *filename)
{
    (void) fs;

    struct uufileops *ops = 0;

    if( 0 == strcmp( filename, "tty" ) )
        ops = &con_fops;

    if(ops == 0)
        return 0;

    uufile_t *ret = create_uufile();

    ret->ops = ops;
    ret->pos = 0;
    ret->fs = &dev_fs;
    ret->name = strdup( filename );
    ret->impl = 0;

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


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static size_t      dev_read(    struct uufile *f, void *dest, size_t bytes)
{
    phantom_device_t* dev = f->impl;
    if(dev->dops.read == 0) return -1;
    return dev->dops.read( dev, dest, bytes );
}

static size_t      dev_write(   struct uufile *f, const void *src, size_t bytes)
{
    phantom_device_t* dev = f->impl;
    if(dev->dops.write == 0) return -1;
    return dev->dops.write( dev, src, bytes );
}

//static errno_t     dev_stat(    struct uufile *f, struct ??);
//static errno_t     dev_ioctl(   struct uufile *f, struct ??);

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
    return impl; // Just a pointer to dev, can copy
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
    uufile_t *devf = lookup_dir( busf, devname, 1, create_uufile );

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

