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


#define DEBUG_MSG_PREFIX "cd_fs"
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
#include <dev/cd_fs.h>

//#include "../ff.h"





// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------


static size_t      cdfs_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      cdfs_write(   struct uufile *f, const void *src, size_t bytes);
static errno_t     cdfs_stat( struct uufile *f, struct stat *dest );
static size_t      cdfs_getpath( struct uufile *f, void *dest, size_t bytes);
static ssize_t     cdfs_getsize( struct uufile *f);
static errno_t     cdfs_seek(    struct uufile *f );
static errno_t     cdfs_readdir( struct uufile *f, struct dirent *dirp );

//static void *      cdfs_copyimpl( void *impl );



static struct uufileops cdfs_fops =
{
    .read 	= cdfs_read,
    .write 	= cdfs_write,

    .readdir    = cdfs_readdir,

    .seek       = cdfs_seek,

    .getpath 	= cdfs_getpath,
    .getsize 	= cdfs_getsize,

//    .copyimpl   = cdfs_copyimpl,

    .stat       = cdfs_stat,
    //.ioctl      = cdfs_ioctl,
};



// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------


static errno_t     cdfs_open(struct uufile *, int create, int write);
static errno_t     cdfs_close(struct uufile *);
static uufile_t *  cdfs_namei(uufs_t *fs, const char *filename);
static uufile_t *  cdfs_getRoot(uufs_t *fs);
static errno_t     cdfs_dismiss(uufs_t *fs);


static struct uufs cdfs_fs =
{
    .name       = "cdfs",
    .open 	= cdfs_open,
    .close 	= cdfs_close,
    .namei 	= cdfs_namei,
    .root 	= cdfs_getRoot,
    .dismiss    = cdfs_dismiss,

    .impl       = 0,
};



uufs_t *cdfs_create_fs( cdfs_t *impl )
{
    uufs_t *ret = calloc( 1, sizeof( uufs_t ) );

    memcpy( ret, &cdfs_fs, sizeof( uufs_t ) );

    ret->impl = impl;

    return ret;
}


// -----------------------------------------------------------------------
// FS methods
// -----------------------------------------------------------------------


static errno_t     cdfs_open(struct uufile *f, int create, int write)
{
    if(create||write) return EROFS;

    (void) f;
    //cdfs_file_t *fi = f->impl;

    //if(fi->e.flags & CD_ENTRY_FLAG_DIR)        return EISDIR;

    return 0;
}

static errno_t     cdfs_close(struct uufile *f)
{
    (void) f;

    return 0;
}

static uufile_t *  cdfs_namei(uufs_t *fs, const char *filename)
{
    cdfs_t *impl = fs->impl;

    cdfs_file_t *fi = calloc(1, sizeof(cdfs_file_t));
    if( fi == 0 )
        return 0;

    iso_dir_entry found_entry;

    if( (*filename == 0) || 0 == strcmp( filename, "/" ) )
    {
        found_entry = impl->root_dir;
    }
    else
    {
        errno_t err = cd_scan_dir( impl->p, &impl->root_dir, filename, &found_entry );
        if( err )
        {
            SHOW_ERROR( 11, "scan dir err %d", err );
            // TODO add *errno parameter
            return 0;
        }
    }

    fi->e = found_entry;

    uufile_t *ret = create_uufile();

    ret->ops = &cdfs_fops;
    ret->pos = 0;
    ret->fs = fs;
    ret->impl = fi;
    ret->flags |= UU_FILE_FLAG_FREEIMPL;
    //ret->flags |= UU_FILE_FLAG_OPEN; // TODO this is wrong and must be gone - open in open!

    if(fi->e.flags & CD_ENTRY_FLAG_DIR)
        ret->flags |= UU_FILE_FLAG_DIR;

    set_uufile_name( ret, filename );

    return ret;
}

// Return a file struct for fs root
static uufile_t *  cdfs_getRoot(uufs_t *fs)
{
    (void) fs;
    return 0;
    //return &cdfs_root;
}

static errno_t  cdfs_dismiss(uufs_t *fs)
{
    (void) fs;
    // free( fs->impl );
    // TODO impl
    return 0;
}


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static errno_t       cdfs_seek(    struct uufile *f )
{
    (void) f;
    //FIL *fp = f->impl;
    //assert(fp);
    //FRESULT r = f_lseek (fp,f->pos);
    //return r ? EINVAL : 0;
    return 0;
}


static size_t      cdfs_read(    struct uufile *f, void *dest, size_t bytes)
{
    unsigned int 	res = 0;
    unsigned int 	sect;
    errno_t 		err;
    char 		buf[CD_SECT_SIZE];

    cdfs_file_t *fi = f->impl;
    cdfs_t *impl = f->fs->impl;

    //r = f_lseek (fp,f->pos);

    size_t shift = f->pos % CD_SECT_SIZE;
    sect = f->pos / CD_SECT_SIZE;

    // Have partial sector at start?
    if( shift )
    {
        err = cd_read_file( impl->p, &fi->e, sect, 1, buf );
        if( err ) return -1;

        size_t part_len = bytes;
        if( part_len > CD_SECT_SIZE-shift )
            part_len = CD_SECT_SIZE-shift;

        memcpy( dest, buf+shift, part_len );
        bytes  -= part_len;
        dest   += part_len;
        f->pos += part_len;
        res    += part_len;
        sect++;
    }

    // Now do some complete sectors
    size_t nsect = bytes/CD_SECT_SIZE;
    if( nsect > 0 )
    {
        err = cd_read_file( impl->p, &fi->e, sect, nsect, dest );
        if( err ) return res;

        bytes  -= nsect*CD_SECT_SIZE;
        dest   += nsect*CD_SECT_SIZE;
        f->pos += nsect*CD_SECT_SIZE;
        res    += nsect*CD_SECT_SIZE;
        sect   += nsect;
    }

    // Have partial sector at end?
    if( bytes )
    {
        err = cd_read_file( impl->p, &fi->e, sect, 1, buf );
        if( err ) return res;

        size_t part_len = bytes;
        assert(part_len < CD_SECT_SIZE);

        memcpy( dest, buf, part_len );

        bytes  -= part_len;
        dest   += part_len;
        f->pos += part_len;
        res    += part_len;
        sect++;
    }

    assert( bytes == 0 );

    return res;
}


static errno_t     cdfs_readdir( struct uufile *f, struct dirent *dirp )
{
    iso_dir_entry e;
    size_t lastpos = f->pos;

    size_t ret = cdfs_read( f, &e, sizeof(iso_dir_entry) );

    if( (ret <= 0) || (e.recordLength == 0) || (e.nameLength == 0) )
        return ENOENT;

    f->pos = lastpos + e.recordLength;

    dirp->d_reclen = 0;
    dirp->d_ino = e.dataStartSector[0];

    if( e.nameLength == 1 )
    {
        // .
        if( e.name[0] == 0 ) 
        {
            strcpy( dirp->d_name, "." );
            dirp->d_namlen = 1;
            return 0;
        }

        // ..
        if( e.name[0] == 1 ) // .
        {
            strcpy( dirp->d_name, ".." );
            dirp->d_namlen = 2;
            return 0;
        }
    }

    size_t nlen = e.nameLength + 1;
    if( nlen > sizeof(dirp->d_name) ) nlen = sizeof(dirp->d_name);

    strlcpy( dirp->d_name, e.name, nlen );
    dirp->d_namlen = nlen - 1;
    return 0;
}


static size_t      cdfs_write(   struct uufile *f, const void *dest, size_t bytes)
{
    (void) f;
    (void) dest;
    (void) bytes;
    return -1;
}


static size_t      cdfs_getpath( struct uufile *f, void *dest, size_t bytes)
{
    if( bytes == 0 ) return 0;
    // TODO get mountpoint
    // we keep name in impl
    strncpy( dest, f->impl, bytes );
    return strlen(dest);
}

// returns -1 for non-files
static ssize_t      cdfs_getsize( struct uufile *f)
{
    if( !(f->flags & UU_FILE_FLAG_OPEN) )
        return -1;

    cdfs_file_t *fi = f->impl;
    if( fi == 0 )
        return -1;

    return fi->e.dataLength[0];
}



static errno_t     cdfs_stat( struct uufile *f, struct stat *dest )
{
    cdfs_file_t *fi = f->impl;
    //cdfs_t *impl = f->fs->impl;

    SHOW_FLOW( 10, "stat '%s'", f->name );

    memset( dest, 0, sizeof(struct stat) );

    dest->st_nlink = 1;
    dest->st_uid = -1;
    dest->st_gid = -1;

    dest->st_size = fi->e.dataLength[0];

    dest->st_mode = 0555; // r-xr-xr-x

    if(fi->e.flags & CD_ENTRY_FLAG_DIR)
        dest->st_mode |= S_IFDIR;
    else
        dest->st_mode |= _S_IFREG;

    return 0;
}



/*
static void *      cdfs_copyimpl( void *impl )
{
    void *dest = calloc( 1, sizeof(e2impl_t) );
    memcpy( dest, impl, sizeof(e2impl_t) );
    return dest;
}
*/



uufile_t *cdfs_mount(errno_t *err, uufile_t *mount_point, uufile_t *device)
{
    (void) mount_point;
    (void) device;

    *err = ENXIO;
    return 0;
}


errno_t cdfs_umount(uufile_t *mount_point, uufile_t *device)
{
    (void) device;

    if( !(mount_point->flags & UU_FILE_FLAG_MNT) )
        return EBADF; // ?

    panic("cdfs umount not impl");
}










#endif // HAVE_UNIX

