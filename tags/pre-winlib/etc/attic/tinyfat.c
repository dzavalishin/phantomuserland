#if 0
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Filesystem map. Looks up disk partition and finds suitable handler.
 *
 **/


#define DEBUG_MSG_PREFIX "fat"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>
#include <phantom_types.h>
#include <string.h>
#include <phantom_disk.h>
#include <kernel/page.h>
//#include <x86/phantom_page.h>


#include "fs_map.h"

struct tiny_fat;

struct __attribute__((packed)) fat_dir
{
    struct tiny_fat *fs; // owning fs

    u_int8_t  filename[11];
    u_int8_t  attribute;
    u_int8_t  reserved;
    u_int8_t  creation;
    u_int16_t time;
    u_int16_t date;
    u_int16_t last_accessed_date;
    u_int16_t hi_cluster;
    u_int16_t last_mod_time;
    u_int16_t last_mod_date;
    u_int16_t lo_cluster;
    u_int32_t size;

    u_int32_t phys_cluster;
    u_int32_t buf_size;
    u_int32_t sect_count;
} __attribute__((packed));


static void fat_dump_dirent( struct fat_dir *f )
{
    printf("Name     : %.11s\n", f->filename );
    printf("Cluster  : %d\n", f->phys_cluster );
    printf("LoCluster: %d\n", f->lo_cluster );
    printf("HiCluster: %d\n", f->hi_cluster );
    printf("Attrs    : %b\n", f->attribute, "\020\1RO\2HIDDEN\3SYSTEM\4VOL\5DIR\6ARCH\7DEV\10UNUSED" );
}




struct __attribute__((packed)) fat_bpb
{
    u_int8_t  	JMP[3];
    u_int8_t  	OEMName[8];
    u_int16_t 	BytsPerSec;
    u_int8_t  	SecPerClus;
    u_int16_t 	ResvdSecCnt;
    u_int8_t  	NumFATs;
    u_int16_t 	RootEntCnt;
    u_int16_t 	TotSec16;
    u_int8_t  	Media;
    u_int16_t 	FATSz16;
    u_int16_t 	SecPerTrk;
    u_int16_t 	NumHeads;
    u_int32_t 	HiddSec;
    u_int32_t 	TotSec32;
} __attribute__((packed));


struct tiny_fat
{
    u_int8_t * 			sec_buf;
    u_int16_t 			RootDirSectors;
    u_int16_t 			FirstDataSector;

    u_int32_t			sectPerDir;
    u_int32_t			sectFats;

    u_int32_t			max_cluster_no;

    struct fat_bpb 		bpb;

    off_t				*fat; // FAT array, preprocessed

    phantom_disk_partition_t *p;
};

typedef struct tiny_fat tiny_fat_t;



void list_files(tiny_fat_t *fs);
struct fat_dir * open_file(tiny_fat_t *fs, const char * const filename);
errno_t close_file(struct fat_dir * dir);
//void check_for_FAT16(tiny_fat_t *fs);



static errno_t hdd_read(phantom_disk_partition_t *p, int sector, void *buf, int count)
{
    SHOW_FLOW( 7, "read sect %d", sector );
    return phantom_sync_read_sector( p, buf, sector, count );
}



static errno_t fat_read_cluster( tiny_fat_t *fs, off_t nCluster, void *buf )
{
    int sect = fs->bpb.ResvdSecCnt + fs->sectFats + fs->sectPerDir + ((nCluster - 2) * fs->bpb.SecPerClus);
    return hdd_read(fs->p, sect, buf, fs->bpb.SecPerClus);
}

static off_t fat_read_fat( tiny_fat_t *fs, off_t cluster_no )
{
    if(cluster_no < 2 || ((u_int32_t)cluster_no) > fs->max_cluster_no)
        return -1;

    return fs->fat[cluster_no];
}

/*!
 *
 * fs				- filesystem
 * dir				- dir entry for file
 * cluster_offset	- position of cluster in file
 *
 * Returns: corresponding number of cluster on disk.
 */
static off_t fat_select_cluster( tiny_fat_t *fs, struct fat_dir *dir, off_t cluster_offset )
{
    u_int32_t chain = dir->lo_cluster | (dir->hi_cluster << 16);

    while( cluster_offset-- )
    {
        if( chain < 2 || chain > fs->max_cluster_no )
            return -1;

        chain = fat_read_fat( fs, chain );
    }

    return chain;
}

static errno_t fat_read( tiny_fat_t *fs, struct fat_dir *dir, off_t pos, size_t size, void *buf )
{
    size_t cluster_bytes = fs->bpb.SecPerClus*fs->bpb.BytsPerSec;

    char temp_buf[cluster_bytes];

    while(size > 0)
    {
        off_t cluster_offset = pos/cluster_bytes;
        off_t cluster_byte_offset = cluster_offset*cluster_bytes;

        off_t nCluster = fat_select_cluster( fs, dir, cluster_offset );
        if( nCluster < 0 )
            return EINVAL;

        errno_t ret = fat_read_cluster( fs, nCluster, temp_buf );
        if(ret) return ret;

        size_t got_bytes = cluster_bytes;
        off_t in_cluster_offset = pos-cluster_byte_offset;
        got_bytes -= in_cluster_offset;

        pos  += got_bytes;
        size -= got_bytes;

        memcpy( buf, temp_buf+in_cluster_offset, got_bytes );
    }

    return 0;
}










static u_int8_t * format_FAT_filename(const char * const filename, const u_int32_t i)
{
    u_int8_t * name = (u_int8_t*)malloc(12);

    memset(name, ' ', 11);
    name[11] = 0;

    memcpy(name, (u_int8_t*)filename, i);
    memcpy(name + 8, (u_int8_t*)filename + i + 1, (11-i+1));

    strupr((char*)name);

    return name;
}

static struct fat_dir * find_file(tiny_fat_t *fs, const u_int8_t * const name)
{
    u_int32_t i;
    //block_t block;

    struct fat_dir * dir;
    dir = (struct fat_dir*)malloc(sizeof(struct fat_dir));

    dir->fs = fs;

    hdd_read(fs->p, fs->FirstDataSector, fs->sec_buf, 1);

    for(i = 0; i < 512; i += 32)
    {
        memcpy((u_int8_t*)dir, fs->sec_buf + i, 32);
        if(dir->filename[0] > 0 && dir->filename[0] != 0xE5 && dir->attribute != 0x0F)
        {
            if(!memcmp(dir->filename, name, 11))
            {
                /* this is a programmer-friendly setup here.
                 these values can be used later without having to re-crunch these same numbers */
                dir->phys_cluster = (fs->FirstDataSector + fs->RootDirSectors - 2 + dir->lo_cluster);
                dir->buf_size = dir->size + (512 - (dir->size % 512));
                dir->sect_count = (dir->size / 512)+1;
                return (struct fat_dir*)dir;
            }

            //block = 0;
        }
    }

    free((u_int8_t*)dir);
    return (struct fat_dir*)(0);
}

struct fat_dir * open_file(tiny_fat_t *fs, const char * const filename)
{
    u_int32_t i;
    u_int8_t *name;

    for(i = 0; i < 11; i++)
    {
        if(filename[i] == '.')
        {
            if(!i || i == 10)
            {
                return (struct fat_dir*)(0);
            }

            name = format_FAT_filename(filename, i);
            return find_file(fs, name);
        }

        if(i >= 10){return (struct fat_dir*)(0);}
    }

    return (struct fat_dir*)(0);
}

errno_t close_file(struct fat_dir * dir)
{
    if(!dir){
        return ENXIO;
    }

    free(dir);

    return 0;
}

void list_files(tiny_fat_t *fs)
{
    u_int32_t i;

    struct fat_dir * dir = (struct fat_dir*)malloc(sizeof(struct fat_dir));

    dir->fs = fs;

    hdd_read( fs->p, fs->FirstDataSector, fs->sec_buf, 1);

    //hexdump(fs->sec_buf, 512, "", 0);

    //u_int8_t * fname = (u_int8_t*)malloc(12);
    u_int8_t fname[12];

    for(i = 0; i < 512; i += 32)
    {
        memcpy((u_int8_t*)dir, fs->sec_buf + i, 32);
        if(dir->filename[0] > 0 && dir->filename[0] != 0xE5 && dir->attribute != 0x0F)
        {
            memcpy(fname, dir->filename, 11);
            fname[11] = 0;

            printf("FAT: name = \"%s\", attributes = %x, size = %u, cluster = %x \n", fname, dir->attribute,
                   dir->size, dir->lo_cluster);
        }
    }

    free(dir);
    //free(fname);
}


static errno_t read_fat_sector(tiny_fat_t *fs, off_t sec_offset, void *buf)
{
    off_t sector = fs->bpb.ResvdSecCnt+sec_offset;
    return hdd_read(fs->p, sector, buf, 1);
}

#define ENT_PER_SECT 256

static errno_t read_all_fat(tiny_fat_t *fs)
{
    char buf[fs->bpb.BytsPerSec];

    size_t fat_sectors = fs->bpb.FATSz16;

    int total_entries = fat_sectors * ENT_PER_SECT;

    if( fs->fat == 0 )
        fs->fat = malloc(total_entries*sizeof(off_t));

    assert(fs->fat);

    SHOW_FLOW0( 1, "Read FAT" );

    size_t fat_sector = 0;
    for( ; fat_sector < fat_sectors; fat_sector++ )
    {
        errno_t ret = read_fat_sector(fs, fat_sector, buf );
        if( ret ) return ret;

        int entry;
        for(entry = 0; entry < ENT_PER_SECT; entry++ )
            fs->fat[entry+fat_sector*ENT_PER_SECT] = buf[entry] & 0xFFFFu;
    }

    SHOW_FLOW0( 1, "Read FAT done" );

    off_t *fep = fs->fat;
    while(total_entries-- > 0)
    {
        printf("%4x ", *fep++ );
        if(! (total_entries % 16))
        {
            printf("\n");

            if( total_entries  < 16 )
                goto out;

            int skip = 0;
            for( ; skip < 16; skip++  )
            {
                if(fep[skip])
                    goto out;
            }

            total_entries -= 16;
            fep += 16;
        }
    out:
        ;
    }

    return 0;
}

static void get_first_data_sector(tiny_fat_t *fs)
{
    fs->FirstDataSector = (fs->bpb.ResvdSecCnt + (fs->bpb.NumFATs * fs->bpb.FATSz16));
    //fs->FirstDataSector += fs->bpb.HiddSec;
    SHOW_FLOW( 2, "bpb.NumFATs     = %d", fs->bpb.NumFATs );
    SHOW_FLOW( 2, "bpb.FATSz16     = %d", fs->bpb.FATSz16 );
    SHOW_FLOW( 2, "bpb.ResvdSecCnt = %d", fs->bpb.ResvdSecCnt );
    SHOW_FLOW( 2, "bpb.HiddSec     = %d", fs->bpb.HiddSec );
    SHOW_FLOW( 2, "FirstDataSector = %d", fs->FirstDataSector );
}

static void get_root_directory_sectors(tiny_fat_t *fs)
{
    fs->RootDirSectors = (((fs->bpb.RootEntCnt * 32) + (fs->bpb.BytsPerSec - 1)) / fs->bpb.BytsPerSec);
    SHOW_FLOW( 2, "RootEntCnt = %d", fs->bpb.RootEntCnt );
    SHOW_FLOW( 2, "RootDirSectors = %d", fs->RootDirSectors );
}

static void parse_FAT16(tiny_fat_t *fs)
{
    get_root_directory_sectors(fs);
    get_first_data_sector(fs);

    fs->sectPerDir = (fs->bpb.RootEntCnt * 32 / fs->bpb.BytsPerSec);
    fs->sectFats = (fs->bpb.NumFATs * fs->bpb.FATSz16);

    SHOW_FLOW0( 1, "FAT: listing all files...\n");
    list_files(fs);
}

errno_t check_for_FAT16(tiny_fat_t *fs)
{


    /* sec_buf should *not* be memfree'd as we need to keep the
     root directory in memory for quick use */
    fs->sec_buf = (u_int8_t*)malloc(512);
    hdd_read( fs->p, 0, fs->sec_buf, 1);
    memcpy((u_int8_t*)&(fs->bpb), fs->sec_buf, sizeof(struct fat_bpb));

    int total_sectors = 0;

    //if(fs->bpb.TotSec32)
    if(fs->bpb.TotSec16 == 0)
    {
        //printf("FAT: FAT32 FS detected \n");
        SHOW_ERROR0( 1, "no support for FAT32 (yet), aborting...");
        total_sectors = fs->bpb.TotSec32;
        return ENXIO;
    }
    else
    {
        if(fs->bpb.TotSec16 < 4096)
        {
            printf("FAT: FAT12 FS detected \n");
        }
        else
        {
            printf("FAT: FAT16 FS detected \n");
        }
        total_sectors = fs->bpb.TotSec16;
    }

    SHOW_FLOW( 1, "total sectors = %u (%u Mb)", total_sectors, (int)(total_sectors*512L)/(1024*1024L) );

    fs->max_cluster_no = total_sectors/fs->bpb.SecPerClus;

    char oem[12];
    memset(oem, sizeof(oem), 0);
    memcpy(oem, fs->bpb.OEMName, 8);
    SHOW_FLOW( 1, "OEM = \"%s\"", oem);

    SHOW_FLOW( 1, "BytesPerSec = %u", fs->bpb.BytsPerSec);
    SHOW_FLOW( 1, "NumHeads = %u", fs->bpb.NumHeads);

    if(fs->bpb.Media == 0xF8)
    {
        SHOW_FLOW0( 1, "Media = Fixed Disk");
    }
    else if(fs->bpb.Media == 0xF0)
    {
        SHOW_FLOW0( 1, "Media = 3.5-inch Floppy Disk");
    }
    else
    {
        SHOW_FLOW0( 1, "Media = UNKNOWN");
    }


    parse_FAT16(fs);
    errno_t ret = read_all_fat(fs);
    if(ret)
        return ret;

    //struct fat_dir *f = open_file( fs, "BOOTLOG.TXT" );
    struct fat_dir *f = open_file( fs, "MSDOS.SYS" );

    if(!f)
        SHOW_ERROR0(0, "Can't open FAT file");
    else
    {
        int size = f->size;
        int maxs = 256;

        if( size > maxs ) size = maxs;

        fat_dump_dirent( f );
#if 0
        char buf[size];
        errno_t ret = fat_read( fs, f, 0, size, buf );
        if(ret)
            SHOW_ERROR(0, "Can't read FAT file, %d", ret);
        else
            hexdump(buf, size, "", 0);
#endif
    }
    return 0;
}



errno_t fs_start_tiny_fat( phantom_disk_partition_t *p )
{
    tiny_fat_t *fs = calloc(sizeof(tiny_fat_t),1);

    fs->p = p;

    return check_for_FAT16(fs);

}











#include <unix/uufile.h>



size_t      fat16_read( struct uufile *f, void *dest, size_t bytes)
{
    struct fat_dir *dir = f->impl;

    errno_t ret = fat_read( dir->fs, dir, f->pos, bytes, dest );
    if( ret )
        return -ret;

    f->pos += bytes;

    return bytes;
}

size_t      fat16_write( struct uufile *f, void *dest, size_t bytes)
{
    tiny_fat_t *fs = f->impl;

    (void) fs;
    (void) dest;
    (void) bytes;
    return -1;
}



#endif


