/**
 *
 * Phantom OS Unix Box
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Root fs, mounts
 *
 *
**/

#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "unix"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <unix/uufile.h>
#include <unix/uuprocess.h>
#include <malloc.h>
#include <string.h>
#include <phantom_libc.h>



// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static uufs_t *    find_mount( const char* name, char *namerest );


static size_t      root_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      root_write(   struct uufile *f, void *dest, size_t bytes);
//static errno_t     root_stat(    struct uufile *f, struct ??);
//static errno_t     root_ioctl(   struct uufile *f, struct ??);

static size_t      root_getpath( struct uufile *f, void *dest, size_t bytes);

// returns -1 for non-files
static ssize_t     root_getsize( struct uufile *f);

static void *      root_copyimpl( void *impl );


static struct uufileops root_fops =
{
    .read 	= root_read,
    .write 	= root_write,

    .getpath 	= root_getpath,
    .getsize 	= root_getsize,

    .copyimpl   = root_copyimpl,

    //.stat       = root_stat,
    //.ioctl      = root_ioctl,
};




// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------


static errno_t     root_open(struct uufile *, int create, int write);
static errno_t     root_close(struct uufile *);

// Create a file struct for given path
static uufile_t *  root_namei(uufs_t *fs, const char *filename);

// Return a file struct for fs root
static uufile_t *  root_getRoot(uufs_t *fs);
static errno_t     root_dismiss(uufs_t *fs);


struct uufs root_fs =
{
    .name       = "root",
    .open 	= root_open,
    .close 	= root_close,
    .namei 	= root_namei,
    .root 	= root_getRoot,
    .dismiss    = root_dismiss,

    .impl       = 0,
};


static struct uufile root_root =
{
    .ops 	= &root_fops,
    .pos        = 0,
    .fs         = &root_fs,
    .name       = "/",
};



// -----------------------------------------------------------------------
// FS methods
// -----------------------------------------------------------------------


static errno_t     root_open(struct uufile *f, int create, int write)
{
	(void) create;
	(void) write;

    link_uufile( f );
    return 0;
}

static errno_t     root_close(struct uufile *f)
{
    unlink_uufile( f );
    return 0;
}


// Create a file struct for given path
static uufile_t *  root_namei(uufs_t *_fs, const char *filename)
{
    char namerest[FS_MAX_PATH_LEN];

    (void) _fs;

    uufs_t * fs = find_mount( filename, namerest );

    if( fs == 0 )
        return 0;

    return fs->namei( fs, namerest );
}

// Return a file struct for fs root
static uufile_t *  root_getRoot(uufs_t *fs)
{
    (void) fs;
    return &root_root;
}


static errno_t     root_dismiss(uufs_t *fs)
{
    (void) fs;
    // TODO impl
    return 0;
}


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static size_t      root_read(    struct uufile *f, void *dest, size_t bytes)
{
	(void) f;
	(void) dest;
	(void) bytes;

    return -1;
}

static size_t      root_write(   struct uufile *f, void *dest, size_t bytes)
{
	(void) f;
	(void) dest;
	(void) bytes;

    return -1;
}

//static errno_t     root_stat(    struct uufile *f, struct ??);
//static errno_t     root_ioctl(   struct uufile *f, struct ??);

static size_t      root_getpath( struct uufile *f, void *dest, size_t bytes)
{
    if( bytes == 0 ) return 0;
    // TODO get mountpoint
    strncpy( dest, f->name, bytes );
    return strlen(dest);
}

// returns -1 for non-files
static ssize_t      root_getsize( struct uufile *f)
{
	(void) f;

    return -1;
}

static void *      root_copyimpl( void *impl )
{
	(void) impl;
    return 0; //strdup(impl);
}




// -----------------------------------------------------------------------
// mounts
// -----------------------------------------------------------------------


struct mount_point
{
    char        path[FS_MAX_MOUNT_PATH]; // fs path this fs is mounted on
    char        name[FS_MAX_MOUNT_PATH]; // fs name - user printable
    uufs_t      *fs;
};

static struct mount_point      mount[FS_MAX_MOUNT];

// This is a very simple impl
static uufs_t * find_mount( const char* name, char *namerest )
{
    uufs_t *ret = 0;
    int maxlen = 0;
    const char *m_path;
    const char *m_name;

    int i;
    for( i = 0; i < FS_MAX_MOUNT; i++ )
    {
        // unused
        if( mount[i].fs == 0 )
            continue;

        int mplen = strlen( mount[i].path );
        if( mplen <= 0 )
            continue;

        if( 0 == strncmp( name, mount[i].path, mplen ) )
        {
            if( mplen > maxlen )
            {
                maxlen = mplen;
                ret = mount[i].fs;
                m_path = mount[i].path;
                m_name = mount[i].name;
            }
        }
    }

    // part of name after the mount point
    strncpy( namerest, name+maxlen, FS_MAX_PATH_LEN );

    SHOW_FLOW( 2, "got '%s' (%s) for '%s', rest = '%s'",
               m_path,
               m_name,
               name, namerest
               );

    return ret;
}

// TODO lock!
static errno_t add_mount( const char* path, const char *name, uufs_t *fs )
{
    // NB! path must finish with /

    if( strlen( path ) >= FS_MAX_MOUNT_PATH-1 )
        return E2BIG;

    if( strlen( name ) >= FS_MAX_MOUNT_PATH-1 )
        return E2BIG;

    int i;
    for( i = 0; i < FS_MAX_MOUNT; i++ )
    {
        // unused
        if( mount[i].fs != 0 )
            continue;
        goto found;
    }
    return ENFILE;

found:
    mount[i].fs = fs;
    strncpy( mount[i].path, path, FS_MAX_MOUNT_PATH );
    strncpy( mount[i].name, name, FS_MAX_MOUNT_PATH );

    if( mount[i].path[strlen(mount[i].path) - 1] != '/' )
        strcat(mount[i].path, "/" );

    return 0;
}


static errno_t rm_mount( const char* name, int flags )
{
	(void) flags;


    int i;
    for( i = 0; i < FS_MAX_MOUNT; i++ )
    {
        // unused
        if( mount[i].fs == 0 )
            continue;

        if( strcmp( mount[i].path, name ) )
        {
            mount[i].fs = 0;
            return 0;
        }

        if( strcmp( mount[i].name, name ) )
        {
            mount[i].fs = 0;
            return 0;
        }
    }

    return ENOENT;
}

// -----------------------------------------------------------------------
// Init all the stuff
// -----------------------------------------------------------------------

void phantom_unix_fs_init()
{
    add_mount( "/dev", 		"devfs", 	&dev_fs );
    add_mount( "/proc", 	"procfs", 	&proc_fs );
    add_mount( "udp://", 	"udpfs", 	&udp_fs );
    add_mount( "tcp://", 	"tcpfs", 	&tcp_fs );
}


// -----------------------------------------------------------------------
// mount syscalls
// -----------------------------------------------------------------------


int usys_mount( int *err, uuprocess_t *u, const char *source, const char *target, const char *fstype, int flags, const void *data )
{
    (void) u;
    (void) flags;
    (void) fstype;


    uufs_t *fs = 0;

    if( strcmp( data, "procfs" ) )
        fs = &proc_fs;

    if( strcmp( data, "devfs" ) )
        fs = &dev_fs;

    // TODO connect to real FS code and devices

    if( fs == 0 )
    {
        *err = ENOTBLK;
        return -1;
    }

    *err = add_mount( target, source, fs );
    return *err ? -1 : 0;
}

int usys_umount(int *err, uuprocess_t *u, const char *target, int flags )
{
    (void) u;

    *err = rm_mount( target, flags );
    return *err ? -1 : 0;
}

#endif // HAVE_UNIX

