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

// mutex
#include <hal.h>


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static uufs_t *    find_mount( const char* name, char *namerest );

#if 0
static size_t      root_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      root_write(   struct uufile *f, const void *src, size_t bytes);
static errno_t     root_stat( struct uufile *f, struct stat *dest );
static size_t      root_getpath( struct uufile *f, void *dest, size_t bytes);
static ssize_t     root_getsize( struct uufile *f);
static void *      root_copyimpl( void *impl );


static struct uufileops root_fops =
{
    .read 	= root_read,
    .write 	= root_write,

    .getpath 	= root_getpath,
    .getsize 	= root_getsize,

    .copyimpl   = root_copyimpl,

    .stat       = root_stat,
    //.ioctl      = root_ioctl,
};
#endif



// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------


static errno_t     root_open(struct uufile *, int create, int write);
static errno_t     root_close(struct uufile *);
static uufile_t *  root_namei(uufs_t *fs, const char *filename);
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
//    .ops 	= &root_fops,
    .ops 	= &common_dir_fops,
    .pos        = 0,
    .fs         = &root_fs,
    .name       = "/",
    .flags      = UU_FILE_FLAG_DIR|UU_FILE_FLAG_NODESTROY,
};



// -----------------------------------------------------------------------
// FS methods
// -----------------------------------------------------------------------

static errno_t     root_open(struct uufile *f, int create, int write)
{
    (void) create;
    (void) write;
    (void) f;

    return 0;
}

static errno_t     root_close(struct uufile *f)
{
    (void) f;
    return 0;
}


// Create a file struct for given path
static uufile_t *  root_namei(uufs_t *_fs, const char *filename)
{
    char namerest[FS_MAX_PATH_LEN];

    (void) _fs;

    if( 0 == strcmp(filename, "/") )
        return copy_uufile( &root_root );
        //return &root_root;

    uufs_t * fs = find_mount( filename, namerest );

    if( fs == 0 )
        return 0;

    return fs->namei( fs, namerest );
}

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

#if 0

static size_t      root_read(    struct uufile *f, void *dest, size_t bytes)
{
    if(f->flags & UU_FILE_FLAG_DIR)
        return common_dir_read(f, dest, bytes);

    return -1;
}

static size_t      root_write(   struct uufile *f, const void *src, size_t bytes)
{
    (void) f;
    (void) src;
    (void) bytes;

    return -1;
}


static size_t      root_getpath( struct uufile *f, void *dest, size_t bytes)
{
    if( bytes == 0 ) return 0;
    // TODO get mountpoint
    strncpy( dest, f->name, bytes );
    return strlen(dest);
}

static ssize_t      root_getsize( struct uufile *f)
{
    (void) f;

    return -1;
}


static errno_t     root_stat( struct uufile *f, struct stat *dest )
{
    const char *name = f->name;

    SHOW_FLOW( 7, "stat %s", name );

    memset( dest, 0, sizeof(struct stat) );

    dest->st_nlink = 1;
    dest->st_uid = -1;
    dest->st_gid = -1;

    dest->st_size = 0;
    dest->st_mode = 0777; // rwxrwxrwx

    // I have dirs only
    dest->st_mode |= S_IFDIR;

    return 0;
}


static void *      root_copyimpl( void *impl )
{
#warning HACK, dir must install own ops and do house keeping there
    (void) impl;
    return impl;
}

#endif


// -----------------------------------------------------------------------
// mounts
// -----------------------------------------------------------------------


struct mount_point
{
    char        path[FS_MAX_MOUNT_PATH]; // fs path this fs is mounted on
    char        name[FS_MAX_MOUNT_PATH]; // fs name - user printable
    uufs_t      *fs;
};

static struct mount_point      	mount[FS_MAX_MOUNT];

static hal_mutex_t               mm;


// This is a very simple impl
static uufs_t * find_mount( const char* name, char *namerest )
{
    uufs_t *ret = 0;
    int maxlen = 0;
    const char *m_path;
    const char *m_name;

    // lock modifications!
    hal_mutex_lock( &mm );

    int i;
    for( i = 0; i < FS_MAX_MOUNT; i++ )
    {
        // unused
        if( mount[i].fs == 0 )
            continue;

        int mplen = strlen( mount[i].path );
        if( mplen <= 0 )
            continue;

        //SHOW_FLOW( 6, "find mount '%s'", name );
        if(
           ( 0 == strncmp( name, mount[i].path, mplen ) ) &&
           ((name[mplen] == '/') || (name[mplen] == '\0'))
          )
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

    hal_mutex_unlock( &mm );

    if( ret )
    {
        // Skip final /
        if( '/' == *(name+maxlen) )
            maxlen++;
        // part of name after the mount point
        strlcpy( namerest, name+maxlen, FS_MAX_PATH_LEN );

        SHOW_FLOW( 7, "got '%s' (%s) for '%s', rest = '%s'",
                   m_path, m_name,
                   name, namerest
                 );
    }

    return ret;
}

// TODO lock!
static
errno_t add_mount( const char* path, const char *name, uufs_t *fs )
{
    // NB! path must finish with /

    if( strlen( path ) >= FS_MAX_MOUNT_PATH-1 )
        return E2BIG;

    if( strlen( name ) >= FS_MAX_MOUNT_PATH-1 )
        return E2BIG;

    hal_mutex_lock( &mm );

    int i;
    for( i = 0; i < FS_MAX_MOUNT; i++ )
    {
        // unused
        if( mount[i].fs != 0 )
            continue;
        goto found;
    }
    hal_mutex_unlock( &mm );
    return ENFILE;

found:
    mount[i].fs = fs;
    strlcpy( mount[i].path, path, FS_MAX_MOUNT_PATH );
    strlcpy( mount[i].name, name, FS_MAX_MOUNT_PATH );

    //if( mount[i].path[strlen(mount[i].path) - 1] != '/' )
    //    strcat(mount[i].path, "/" );

    // Kill final slash
    int rlen = strlen(mount[i].path);
    if( mount[i].path[rlen - 1] == '/' )
        mount[i].path[rlen - 1] = 0;

    lookup_dir( &root_root, mount[i].path, 1, create_dir );

    hal_mutex_unlock( &mm );

    return 0;
}


static errno_t rm_mount( const char* name, int flags )
{
    (void) flags;

    errno_t rc = unlink_dir_name( &root_root, name );
    if( rc )
    {
        SHOW_ERROR( 1, "can't unlink %s", name );
        return rc;
    }

    hal_mutex_lock ( &mm );

    int i;
    for( i = 0; i < FS_MAX_MOUNT; i++ )
    {
        // unused
        if( mount[i].fs == 0 )
            continue;

        if( 0 == strcmp( mount[i].path, name ) )
        {
            mount[i].fs = 0;
            hal_mutex_unlock ( &mm );
            return 0;
        }

    }

    hal_mutex_unlock ( &mm );
    return ENOENT;
}

// -----------------------------------------------------------------------
// Init all the stuff
// -----------------------------------------------------------------------

void phantom_unix_fs_init()
{
    hal_mutex_init( &mm, "rootfs" );

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





errno_t auto_mount( const char *name, uufs_t *fs )
{
    static int am_index = 0;

    static char mpath[32];
    snprintf( mpath, sizeof(mpath), "/amnt%d", am_index ++ );

    return add_mount( mpath, name, fs );
}



// -----------------------------------------------------------------------
// Find fs by path
// -----------------------------------------------------------------------


// Find fs for path and return rest of path

uufs_t * uu_findfs(const char *filename, char *rest )
{
    if( filename[0] == '/' && filename[1] == 0 )
        return &root_fs;

    return find_mount( filename, rest );
}





#endif // HAVE_UNIX

