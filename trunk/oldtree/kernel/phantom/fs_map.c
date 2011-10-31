/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Filesystem map. Looks up disk partition and finds suitable handler.
 *
**/


#define DEBUG_MSG_PREFIX "fsmap"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 1

#include <phantom_libc.h>
#include <string.h>
#include <phantom_disk.h>
#include <kernel/page.h>

#include "fs_map.h"




typedef struct
{
    const char *name;
    errno_t (*probe_f)( phantom_disk_partition_t * );        	// FS probe func
    errno_t (*use_f)( phantom_disk_partition_t * );        	// FS activate func
} fs_probe_t;


#if HAVE_UNIX

errno_t fs_probe_ext2(phantom_disk_partition_t *p);
errno_t fs_start_ext2(phantom_disk_partition_t *p);

errno_t fs_start_cd(phantom_disk_partition_t *p);

//static errno_t fs_start_fat( phantom_disk_partition_t * );

errno_t fs_start_ff( phantom_disk_partition_t * );

#endif // HAVE_UNIX

errno_t fs_use_phantom(phantom_disk_partition_t *p);


static fs_probe_t fs_drivers[] =
{

    { "Phantom", 	fs_probe_phantom,	fs_use_phantom 	            },
#if HAVE_UNIX
    { "FAT", 		fs_probe_fat, 	 	fs_start_ff	            },

    { "Ext2",  		fs_probe_ext2, 	        fs_start_ext2               },
    { "CD",  		fs_probe_cd, 	 	fs_start_cd                 },
#endif // HAVE_UNIX

};



errno_t lookup_fs(phantom_disk_partition_t *p)
{
    char pname[128];
    partGetName( p, pname, sizeof(pname) );

    SHOW_INFO( 0, "Look for filesystems on partition %s", pname );
    unsigned int i;
    for( i = 0; i < sizeof(fs_drivers)/sizeof(fs_probe_t); i++ )
    {
        fs_probe_t *fp = &fs_drivers[i];

        SHOW_INFO( 0, "probe %s fs on %s", fp->name, pname );

        errno_t ret = fp->probe_f( p );
        if( ret ) continue;

        SHOW_INFO( 0, "%s file sysem found on partition %s", fp->name, pname );

        if(!fp->use_f)
        {
            SHOW_ERROR( 0, "%s file sysem is not implemented yet", fp->name );
            continue;
        }

        ret = fp->use_f( p );
        if( ret )
        {
            SHOW_ERROR( 0, "%s file sysem driver rejected partition %s", fp->name, pname );
            continue;
        }

        SHOW_INFO( 0, "%s file sysem driver occupies partition %s", fp->name, pname );
        return 0;
    }

    return EINVAL;
}




static disk_page_no_t sbpos[] = DISK_STRUCT_SB_OFFSET_LIST;
static int nsbpos = sizeof(sbpos)/sizeof(disk_page_no_t);

#define MAX_PFS_PARTS 10
static phantom_disk_partition_t *phantom_fs_partitions[MAX_PFS_PARTS];
static int n_phantom_fs_partitions = 0;

errno_t fs_probe_phantom(phantom_disk_partition_t *p)
{
    char buf[PAGE_SIZE];
    phantom_disk_superblock *sb = (phantom_disk_superblock *)&buf;

    int i;
    for( i = 0; i < nsbpos; i++ )
    {
        if( phantom_sync_read_block( p, buf, sbpos[i], 1 ) )
            continue;
        if( phantom_calc_sb_checksum( sb ) )
        {
            p->flags |= PART_FLAG_IS_PHANTOM_FSSB;
            return 0;
        }
    }

    return EINVAL;
}

errno_t fs_use_phantom(phantom_disk_partition_t *p)
{
    assert( p->flags | PART_FLAG_IS_PHANTOM_FSSB );

    char pname[128];
    partGetName( p, pname, sizeof(pname) );

    if(n_phantom_fs_partitions < MAX_PFS_PARTS)
    {
        phantom_fs_partitions[n_phantom_fs_partitions++] = p;
        return 0;
    }
    else
    {
        SHOW_ERROR( 0, "Too many Phantom disks, skip %s", pname);
        return EMFILE;
    }
}


phantom_disk_partition_t *select_phantom_partition(void)
{
    if(n_phantom_fs_partitions == 0)
        panic("no Phantom disk");

    if(n_phantom_fs_partitions == 1)
    {
        assert( phantom_fs_partitions[0] );

        char pname[128];
        partGetName( phantom_fs_partitions[0], pname, sizeof(pname) );

        SHOW_FLOW( 0, "Just one Phantom disks found (%s)", pname);
        return phantom_fs_partitions[0];
    }

    printf("Select Phantom disk to boot:\n");
    int i;
    for( i = 0; i < n_phantom_fs_partitions; i++ )
    {
        phantom_disk_partition_t *p = phantom_fs_partitions[i];
        if(p)
        {
            char pname[128];
            partGetName( p, pname, sizeof(pname) );

            printf("%2d: %s\n", i, pname );
        }
    }

    do {

        printf("Press digit (0-%d): \n", n_phantom_fs_partitions-1 );
        char c = getchar();

        if( c < '0' || c > '9' )
            continue;

        int n = c-'0';

        if( (n > MAX_PFS_PARTS) || (n < 0) )
            continue;

        phantom_disk_partition_t *p = phantom_fs_partitions[n];

        if( !p )
            continue;

        return p;

    } while(1);
}


#if HAVE_UNIX



errno_t fs_probe_fat(phantom_disk_partition_t *p )
{
    unsigned char buf[PAGE_SIZE];

    SHOW_FLOW( 0, "%s look for FAT", p->name );

    switch( p->type )
    {
    case 1: // FAT 12
    case 4: // FAT 16 below 32M
    case 6: // FAT 16 over 32M
    case 7: // ExFAT 64
        break;

    case 0x0B: // FAT32 non-LBA?!
        SHOW_FLOW( 0, "Warning: Part type is %d (non-LBA)", p->type );
    case 0x0C: // FAT32 LBA
    case 0x0E: // FAT16 LBA
        break;

    default:
        if(p->flags & PART_FLAG_IS_WHOLE_DISK)
            break;

        SHOW_ERROR( 1, "Not a FAT partition type 0x%X", p->type );
        return EINVAL;
    }


    if( phantom_sync_read_sector( p, buf, 0, 1 ) )
    {
        SHOW_ERROR( 0, "%s can't read sector 0", p->name );
        return EINVAL;
    }

    //hexdump( buf, 512, 0, 0 );

    if( (buf[0x1FE] != 0x55) || (buf[0x1FF] != 0xAA) )
    {
        SHOW_ERROR0( 1, "No magic" );
        return EINVAL;
    }

    u_int16_t blksize = *((u_int16_t *)(buf+0xb));
    if( 512 != blksize )
    {
        SHOW_ERROR( 1, "Blocksize is !512, %d", blksize );
        return EINVAL;
    }

    u_int8_t signature = *((u_int8_t *)(buf+0x26));
    SHOW_FLOW( 0, "signature is 0x%X", signature );
    switch(signature)
    {
    case 0x28:
    case 0x29: // DOS4
        break;

    default:
        SHOW_ERROR0( 1, "Unknown signature" );
        break;
    }

    u_int32_t serial = *((u_int32_t *)(buf+0x27));
    SHOW_FLOW( 0, "serial num is 0x%X", serial );

    // different FATs have it in different places.
#if 0

#define FAT_LABEL_LEN 12
    char label[FAT_LABEL_LEN];
    memset( label, FAT_LABEL_LEN, 0 );
    memcpy( label, buf+0x2B, FAT_LABEL_LEN-1 );

    SHOW_FLOW( 0, "label is %.*s", FAT_LABEL_LEN-1, label );

#define FAT_FSTYPE_LEN 9
    char fstype[FAT_FSTYPE_LEN];
    memset( fstype, FAT_FSTYPE_LEN, 0 );
    memcpy( fstype, buf+0x2B, FAT_FSTYPE_LEN-1 );

    SHOW_FLOW( 0, "fstype is %.*s", FAT_FSTYPE_LEN-1, fstype );

#endif

    return 0;
}






// -----------------------------------------------------------------------
// CDFS impl - move off!
// -----------------------------------------------------------------------





static char cd_marker[] = { 1, 67, 68, 48, 48, 49, 1 };


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

static void cd_dump_dir( iso_dir_entry *e );




static errno_t cd_read_sectors( phantom_disk_partition_t *p, void *buf, int cd_sector, size_t nsectors )
{
    SHOW_FLOW( 10, "CDFS disk read @ sect %d, nsect %d", cd_sector * 4, nsectors * 4 );
    return phantom_sync_read_sector( p, buf, cd_sector * 4, nsectors * 4 );
}



errno_t fs_start_cd(phantom_disk_partition_t *p)
{

    char buf[PAGE_SIZE];

    int cd_sector = 16;

    // Have some limit
    while(cd_sector < 64)
    {
        //if( phantom_sync_read_sector( p, buf, cd_sector * 4, 4 ) )            return EINVAL;
        if( cd_read_sectors( p, buf, cd_sector, 1 ) )
            return EINVAL;

        if( strncmp( buf, cd_marker, 7 ) || (buf[7] != 0) )
            return EINVAL;

        break;
    }

    SHOW_FLOW( 2, "CDFS marker found @ sector %d", cd_sector );

    cd_vol_t *vd = (void *)buf;

    size_t nsect = vd->numSectors[0];
    SHOW_INFO( 2, "CDFS %d sectors %d path tbl sz @ %d, sect size %d", nsect, vd->pathTblSize[0], vd->lePathTbl1Sector, vd->sectorSize[0] );

    if( vd->sectorSize[0] != 2048 )
    {
        SHOW_ERROR( 0, "CDFS sect size %d != 2048, not supported", vd->sectorSize[0] );
        return EINVAL;
    }

    iso_dir_entry *rootdir = (void *)&vd->rootDirEntry;

    cdfs_t  *impl = calloc(1, sizeof(cdfs_t));
    if(impl==0)
        return ENOMEM;

    impl->p = p;
    impl->volume_descr = *vd;
    impl->root_dir = *rootdir;

    uufs_t * fs = cdfs_create_fs( impl );
    if( !fs )
    {
        SHOW_ERROR( 0, "can't create uufs for %s", p->name );
    }

    if( fs && auto_mount( p->name, fs ) )
    {
        SHOW_ERROR( 0, "can't automount %s", p->name );
    }


    cd_dump_dir( rootdir );

    if(0)
    {
        char *fn = "docs/install.txt";
        //char *fn = "a/b/c/d/autorun.inf";

        iso_dir_entry ret;
        errno_t err = cd_scan_dir( p, rootdir, fn, &ret );

        size_t file_bytes = ret.dataLength[0];

        SHOW_INFO( 0, "cd_scan_dir() = %d, sz = %d", err, file_bytes );

        if( err == 0 )
        {
            size_t file_sectors = CD_BYTES_TO_SECTORS(file_bytes);

            char buf[file_sectors*CD_SECT_SIZE];

            err = cd_read_file( p, &ret, 0, file_sectors, buf );

            SHOW_INFO( 0, "cd_read_file( p, e, 0, %d sect, buf) = %d", file_sectors, err );
            if( 0 == err )
            {
                printf("((%.*s))\n", file_bytes, buf );
            }

        }

    }

    return 0;
}


static void cd_dump_dir( iso_dir_entry *e )
{
    SHOW_INFO( 0, "CDFS dir @ sect %d, sz %d", e->dataStartSector[0], e->dataLength[0] );
    //SHOW_INFO( 0, "CDFS dir unitSize %d, gapSize %d", e->unitSize, e->gapSize );

}


static errno_t cd_scan_dir_sect( phantom_disk_partition_t *p, void *entries, const char *name_to_find, const char *path_rest, iso_dir_entry *found_entry )
{
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
                    return cd_scan_dir( p, e, path_rest, found_entry );
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


errno_t cd_scan_dir( phantom_disk_partition_t *p, iso_dir_entry *e, const char *path_to_find, iso_dir_entry *found_entry )
{
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
        errno_t err = cd_read_sectors( p, buf, sect++, 1 );
        left -= CD_SECT_SIZE;

        if( err )
            SHOW_ERROR( 0, "CDFS dir sect read err %d != 2048, not supported", err );
        else
        {
            err = cd_scan_dir_sect( p, buf, name_to_find, path_rest, found_entry );
            if( 0 == err ) return err;
        }
    }

    return ENOENT;
}

errno_t cd_read_file( phantom_disk_partition_t *p, iso_dir_entry *e, u_int32_t start_sector, size_t nsect, void *buf )
{
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

    errno_t err = cd_read_sectors( p, buf, sect, nsect );

    if( err )
    {
        SHOW_ERROR( 1, "CDFS file sect read err %d", err );
        return err;
    }

    return 0;
}



#endif // HAVE_UNIX










