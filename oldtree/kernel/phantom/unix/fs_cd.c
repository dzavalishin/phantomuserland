/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ISO9660 (CD) FS
 *
 *
**/

#if HAVE_UNIX


#define DEBUG_MSG_PREFIX "cd_fs"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10


#include "fs_map.h"

#include <unix/uufile.h>
//#include <unix/uuprocess.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <phantom_libc.h>
#include <kernel/page.h>
#include <dev/cd_fs.h>




static errno_t cd_scan_dir( cdfs_t *impl, iso_dir_entry *e, const char *path_to_find, iso_dir_entry *found_entry );
static errno_t cd_read_file( cdfs_t *impl, iso_dir_entry *e, u_int32_t start_sector, size_t nsect, void *buf );


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

    iso_dir_entry found_entry;

    if( (*filename == 0) || 0 == strcmp( filename, "/" ) )
    {
        found_entry = impl->root_dir;
    }
    else
    {
        errno_t err = cd_scan_dir( impl, &impl->root_dir, filename, &found_entry );
        if( err )
        {
            SHOW_ERROR( 11, "scan dir err %d", err );
            // TODO add *errno parameter
            return 0;
        }
    }

    cdfs_file_t *fi = calloc(1, sizeof(cdfs_file_t));
    if( fi == 0 )
        // TODO add *errno parameter
        return 0;

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
        err = cd_read_file( impl, &fi->e, sect, 1, buf );
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
        err = cd_read_file( impl, &fi->e, sect, nsect, dest );
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
        err = cd_read_file( impl, &fi->e, sect, 1, buf );
        if( err ) return res;

        size_t part_len = bytes;
        assert(part_len < CD_SECT_SIZE);

        memcpy( dest, buf, part_len );

        bytes  -= part_len;
        dest   += part_len; (void) dest;
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




// -----------------------------------------------------------------------
// Low level impl
// -----------------------------------------------------------------------




//static char cd_marker[] = { 1, 67, 68, 48, 48, 49, 1 };
static char cd_marker[] = { 1, 'C', 'D', '0', '0', '1', 1 };


errno_t fs_probe_cd(phantom_disk_partition_t *p)
{

    char buf[PAGE_SIZE];
    //phantom_disk_superblock *sb = (phantom_disk_superblock *)&buf;

    int cd_sector = 16;

    // Have some limit
    while(cd_sector < 64)
    {
        if( phantom_sync_read_sector( p, buf, cd_sector * 4, 4 ) )
            return EINVAL;

        if( strncmp( buf, cd_marker, 7 ) || (buf[7] != 0) )
            return EINVAL;

        SHOW_FLOW( 3, "CDFS marker found @ sector %d", cd_sector );
        return 0;
    }


    return EINVAL;
}

#include <dev/cd_fs.h>
#include <unix/uufile.h>

#define CD_CACHE 1

enum  so_vd_enum_s
{
  ISO_VD_BOOT_RECORD = 0, ISO_VD_PRIMARY = 1, ISO_VD_SUPPLEMENTARY = 2, ISO_VD_PARITION = 3,
  ISO_VD_END = 255
};

static errno_t cd_read_sectors( cdfs_t *impl, void *buf, int cd_sector, size_t nsectors )
{
    phantom_disk_partition_t *p = impl->p;

    SHOW_FLOW( 10, "CDFS disk read @ sect %d, nsect %d", cd_sector * 4, nsectors * 4 );

#if CD_CACHE
    if( impl->cache )
    {
        if( 0 == cache_get_multiple( impl->cache, cd_sector, nsectors, buf ) )
            return 0;
    }

    errno_t rc = phantom_sync_read_sector( p, buf, cd_sector * 4, nsectors * 4 );

    if( impl->cache && !rc )
        cache_put_multiple( impl->cache, cd_sector, nsectors, buf );

    return rc;
#else
    return phantom_sync_read_sector( p, buf, cd_sector * 4, nsectors * 4 );
#endif
}



errno_t fs_start_cd(phantom_disk_partition_t *p)
{
    char buf[PAGE_SIZE];

    int cd_sector;
    cd_vol_t vd;
    int joliet_level = 0;
    bool primary_vd_found = 0;

    cdfs_t  *impl = calloc(1, sizeof(cdfs_t));
    if(impl==0)
        return ENOMEM;

    impl->p = p;

#if CD_CACHE
    if( impl->cache == 0 )
        impl->cache = cache_init( CD_SECT_SIZE );
#endif
    // Have some limit
    for(cd_sector = 16; cd_sector < 64; cd_sector++)
    {
        if( cd_read_sectors( impl, buf, cd_sector, 1 ) )
        {
            free( impl );
            return EINVAL;
        }

        // Not VD at all
        if( strncmp( buf+1, cd_marker+1, 5 ) )
            continue;

        // Primary VD
        //if( (0 == strncmp( buf, cd_marker, 7 )) || (buf[7] != 0) )
        if( buf[0] == ISO_VD_PRIMARY )
        {
            vd = *(cd_vol_t *)buf;
            primary_vd_found = 1;
            SHOW_FLOW( 1, "CDFS primary vol desc found @ sector %d", cd_sector );
            continue;
        }

        // Supplementary VD
        if( buf[0] == ISO_VD_SUPPLEMENTARY )
        {
            // JOLIET extension: test escape sequence for level of UCS-2 characterset
            if (buf[88] == 0x25 && buf[89] == 0x2f)
            {
                switch(buf[90])
                {
                case 0x40: joliet_level = 1; break;
                case 0x43: joliet_level = 2; break;
                case 0x45: joliet_level = 3; break;
                }

                SHOW_INFO( 1, "ISO9660 Extensions: Microsoft Joliet Level %d", joliet_level );

                //if (vol->joliet_level > 0) InitNode(&(vol->rootDirRec), &buf[156], NULL, 0);
            }

            continue;
        }

        if( *(unsigned char *)buf == ISO_VD_END) // ISO_VD_END
            break;
    }

    if(!primary_vd_found)
    {
        SHOW_ERROR0( 0, "CDFS primary vol desc not found" );
    retinval:
        if( impl->cache  )
            cache_destroy( impl->cache );
        free(impl);
        return EINVAL;
    }

    size_t nsect = vd.numSectors[0];
    SHOW_INFO( 2, "CDFS %d sectors %d path tbl sz @ %d, sect size %d", nsect, vd.pathTblSize[0], vd.lePathTbl1Sector, vd.sectorSize[0] );

    if( vd.sectorSize[0] != 2048 )
    {
        SHOW_ERROR( 0, "CDFS sect size %d != 2048, not supported", vd.sectorSize[0] );
        //return EINVAL;
        goto retinval;
    }

    iso_dir_entry *rootdir = (void *)&vd.rootDirEntry;
    SHOW_FLOW( 7, "CDFS root dir @ sect %d, sz %d", rootdir->dataStartSector[0], rootdir->dataLength[0] );

    impl->volume_descr = vd;
    impl->root_dir = *rootdir;

    uufs_t * fs = cdfs_create_fs( impl );
    if( !fs )
    {
        SHOW_ERROR( 0, "can't create uufs for %s", p->name );
    }

    if( fs && auto_mount( p->name, fs, 0, 0, AUTO_MOUNT_FLAG_AUTORUN ) )
    {
        SHOW_ERROR( 0, "can't automount %s", p->name );
    }


    if(0)
    {
        char *fn = "docs/install.txt";
        //char *fn = "a/b/c/d/autorun.inf";

        iso_dir_entry ret;
        errno_t err = cd_scan_dir( impl, rootdir, fn, &ret );

        size_t file_bytes = ret.dataLength[0];

        SHOW_INFO( 0, "cd_scan_dir() = %d, sz = %d", err, file_bytes );

        if( err == 0 )
        {
            size_t file_sectors = CD_BYTES_TO_SECTORS(file_bytes);

            char buf[file_sectors*CD_SECT_SIZE];

            err = cd_read_file( impl, &ret, 0, file_sectors, buf );

            SHOW_INFO( 0, "cd_read_file( p, e, 0, %d sect, buf) = %d", file_sectors, err );
            if( 0 == err )
            {
                printf("((%.*s))\n", file_bytes, buf );
            }

        }

    }

    return 0;
}




static errno_t cd_scan_dir_sect( cdfs_t *impl, void *entries, const char *name_to_find, const char *path_rest, iso_dir_entry *found_entry )
{
    //phantom_disk_partition_t *p = impl->p;

    int left = CD_SECT_SIZE;
    while( left > 0 )
    {
        iso_dir_entry *e = entries;

        left -= e->recordLength;
        entries += e->recordLength;

        char name[33];

        size_t nlen = e->nameLength + 1;
        if( nlen > sizeof(name) - 1 ) nlen = sizeof(name) - 1;

        strlcpy( name, e->name, nlen );

        //if( e->nameLength == 0 ) continue; // .
        if( (e->nameLength == 1) && (name[0] == 0) ) continue; // .
        if( (e->nameLength == 1) && (name[0] == 1) ) continue; // ..

        if( (e->recordLength == 0) || (e->nameLength == 0) )
        {
            //SHOW_ERROR0( 0, "CDFS dir entry rec len == 0" );
            //return EINVAL;
            break;
        }

        SHOW_FLOW( 9, "name '%16s' type %b sect %d",
                   name,
                   e->flags, "\020\1Hidden\2Dir\3Assoc\4RecFormat\5Perm\10NotFinal",
                   e->dataStartSector[0]
                 );

        if( stricmp( name_to_find, name ) == 0 )
        {
            SHOW_FLOW( 4, "found '%s'", name_to_find );
            if( path_rest == 0 )
            {
                if(found_entry)
                    *found_entry = *e;
                return 0;
            }
            else
            {
                if( e->flags & CD_ENTRY_FLAG_DIR )
                {
                    SHOW_FLOW( 5, "descend, look for '%s'", path_rest );
                    return cd_scan_dir( impl, e, path_rest, found_entry );
                }
                else
                {
                    return ENOTDIR;
                }
            }
        }

    }

    return ENOENT;
}


errno_t cd_scan_dir( cdfs_t *impl, iso_dir_entry *e, const char *path_to_find, iso_dir_entry *found_entry )
{
    //phantom_disk_partition_t *p = impl->p;

    SHOW_FLOW( 7, "CDFS dir @ sect %d, sz %d, look for '%s'", e->dataStartSector[0], e->dataLength[0], path_to_find );

    int sect = e->dataStartSector[0];
    int left = e->dataLength[0];

    const char *name_to_find;
    const char *path_rest;

    size_t namelen = 1;

    const char *slash = index( path_to_find, '/' );
    if( slash == 0 )
    {
        name_to_find = path_to_find;
        path_rest = 0;
    }
    else
    {
        namelen = slash-path_to_find;
    }

    char name[namelen+1];

    if( slash )
    {
        strlcpy( name, path_to_find, namelen+1 );
        name_to_find = name;
        path_rest = slash+1;
    }

    SHOW_FLOW( 8, "name = '%s', rest = '%s'", name_to_find, path_rest );

    char buf[CD_SECT_SIZE];

    while( left > 0 )
    {
        errno_t err = cd_read_sectors( impl, buf, sect++, 1 );
        left -= CD_SECT_SIZE;

        if( err )
            SHOW_ERROR( 0, "CDFS dir sect read err %d != 2048, not supported", err );
        else
        {
            err = cd_scan_dir_sect( impl, buf, name_to_find, path_rest, found_entry );
            if( 0 == err ) return err;
        }
    }

    return ENOENT;
}

errno_t cd_read_file( cdfs_t *impl, iso_dir_entry *e, u_int32_t start_sector, size_t nsect, void *buf )
{
    //phantom_disk_partition_t *p = impl->p;

    SHOW_FLOW( 4, "CDFS file @ sect %d, sz %d", e->dataStartSector[0], e->dataLength[0] );

    int file_start_sect = e->dataStartSector[0];
    size_t file_size = e->dataLength[0];

    size_t file_sectors = CD_BYTES_TO_SECTORS(file_size);

    if( start_sector + nsect > file_sectors )
        nsect = file_sectors - start_sector;

    if( nsect <= 0 )
        return EINVAL;

    // Reasonable limit?
    if( nsect > 1024 ) return EINVAL;

    int sect = file_start_sect+start_sector;

    SHOW_FLOW( 9, "CDFS file read @ sect %d, nsect %d", sect, nsect );

    errno_t err = cd_read_sectors( impl, buf, sect, nsect );

    if( err )
    {
        SHOW_ERROR( 1, "CDFS file sect read err %d", err );
        return err;
    }

    return 0;
}






#endif // HAVE_UNIX

