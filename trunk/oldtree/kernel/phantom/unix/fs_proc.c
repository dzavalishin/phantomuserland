/**
 *
 * Phantom OS Unix Box
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Proc FS
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



// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------


static size_t      proc_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      proc_write(   struct uufile *f, const void *dest, size_t bytes);
static size_t      proc_getpath( struct uufile *f, void *dest, size_t bytes);
static ssize_t     proc_getsize( struct uufile *f);
static void *      proc_copyimpl( void *impl );


static struct uufileops proc_fops =
{
    .read 	= proc_read,
    .write 	= proc_write,

    .getpath 	= proc_getpath,
    .getsize 	= proc_getsize,

    .copyimpl   = proc_copyimpl,

    //.stat       = proc_stat,
    //.ioctl      = proc_ioctl,
};





// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------

static errno_t     proc_open(struct uufile *, int create, int write);
static errno_t     proc_close(struct uufile *);
static uufile_t *  proc_namei(uufs_t *fs, const char *filename);
static uufile_t *  proc_getRoot(uufs_t *fs);
static errno_t     proc_dismiss(uufs_t *fs);


struct uufs proc_fs =
{
    .name       = "proc",
    .open 	= proc_open,
    .close 	= proc_close,
    .namei 	= proc_namei,
    .root 	= proc_getRoot,
    .dismiss    = proc_dismiss,

    .impl       = 0,
};


static struct uufile proc_root =
{
    .ops 	= &proc_fops,
    .pos        = 0,
    .fs         = &proc_fs,
    .impl       = 0,
    .flags      = UU_FILE_FLAG_NODESTROY|UU_FILE_FLAG_DIR,
};



// -----------------------------------------------------------------------
// FS methods
// -----------------------------------------------------------------------


static errno_t     proc_open(struct uufile *f, int create, int write)
{
    (void) f;
    (void) create;
    (void) write;

    return 0;
}

static errno_t     proc_close(struct uufile *f)
{
    (void) f;
    return 0;
}

static size_t r_about( struct uufile *f, void *dest, size_t bytes)
{
    (void) f;

    strncpy( dest, "Phantom ProcFS", bytes );
    return strlen(dest);
}


// Create a file struct for given path
static uufile_t *  proc_namei( uufs_t *fs, const char *filename)
{
    (void) fs;
    void *impl = 0;

    if( strcmp( filename, "about" ) )
        impl = r_about;

    if(impl == 0)
        return 0;

    uufile_t *ret = create_uufile();
    ret->ops = &proc_fops;
    ret->fs = &proc_fs;
    ret->impl = impl;

    set_uufile_name( ret, filename );

    return ret;
}

// Return a file struct for fs root
static uufile_t *  proc_getRoot(uufs_t *fs)
{
    (void) fs;

    return &proc_root;
}

static errno_t     proc_dismiss(uufs_t *fs)
{
    (void) fs;
    // TODO impl
    return 0;
}


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static size_t      proc_read(    struct uufile *f, void *dest, size_t bytes)
{
    size_t (*rf)( struct uufile *f, void *dest, size_t bytes) = f->impl;

    if(rf == 0)
        return -1;

    return rf( f, dest, bytes);
}

static size_t      proc_write(   struct uufile *f, const void *dest, size_t bytes)
{
    (void) f;
    (void) dest;
    (void) bytes;

    return -1;
}


static size_t      proc_getpath( struct uufile *f, void *dest, size_t bytes)
{
    if( bytes == 0 ) return 0;
    // TODO get mountpoint
    // we keep name in impl
    strncpy( dest, f->name, bytes );
    return strlen(dest);
}

// returns -1 for non-files
static ssize_t      proc_getsize( struct uufile *f)
{
    (void) f;

    return -1;
}

static void *      proc_copyimpl( void *impl )
{
    return impl; 
}





#endif // HAVE_UNIX

