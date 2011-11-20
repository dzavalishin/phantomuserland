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
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <unix/uufile.h>
#include <malloc.h>
#include <string.h>
#include <kernel/libkern.h>

// data sources
#include "svn_version.h"
#include <kernel/init.h>
#include <kernel/stats.h>
#include <thread_private.h>
#include <console.h>


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

#define R_CONST(__name, __val) \
  static size_t r_##__name( struct uufile *f, void *dest, size_t bytes) \
  {                                                                     \
      if(f->pos > 0)                                                    \
          return 0;                                                     \
                                                                        \
      int nc = strlcpy( dest, __val, bytes );                           \
      f->pos += nc;                                                     \
      return nc;                                                        \
  }


R_CONST(about, "Phantom ProcFS")
R_CONST(version, "Phantom " PHANTOM_VERSION_STR)

R_CONST(arch, arch_name)
R_CONST(board, board_name)

#define R_SETFUNC(__name) if( 0 == strcmp( #__name, filename ) ) impl = r_##__name

// TODO are we sure we can give it all out?
static size_t r_threads( struct uufile *f, void *dest, size_t bytes)
{
    phantom_thread_t out;
    tid_t t = get_next_tid( f->pos, &out);
    f->pos = t;

    if( t < 0 )
        return 0;

    int nc = umin( bytes, sizeof(out) );
    memcpy( dest, &out, nc );
    return nc;
}

static size_t r_stats( struct uufile *f, void *dest, size_t bytes)
{
    struct kernel_stats out;
    errno_t rc = get_stats_record( f->pos++, &out );

    if( rc )
        return 0;

    int nc = umin( bytes, sizeof(out) );
    memcpy( dest, &out, nc );
    return nc;
}


static size_t r_dmesg( struct uufile *f, void *dest, size_t bytes)
{
    int rd = dmesg_read_buf( dest, bytes, f->pos );
    if( rd <= 0 )
        return 0;

    f->pos += rd;
    return rd;
}



// Create a file struct for given path
static uufile_t *  proc_namei( uufs_t *fs, const char *filename)
{
    (void) fs;
    void *impl = 0;

    R_SETFUNC(about);
    R_SETFUNC(version);
    R_SETFUNC(arch);
    R_SETFUNC(board);
    R_SETFUNC(threads);
    R_SETFUNC(stats);
    R_SETFUNC(dmesg);



    if(impl == 0)        return 0;

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

