
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
#include <x86/phantom_page.h>


#include "fs_map.h"


struct __attribute__((packed)) fat_dir
{
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
    struct fat_bpb 		bpb;

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
	return phantom_sync_read_disk( p, buf, sector, count );
}











// TODO move to stdlib
static char * strupr(char *name)
{
	char *ret = name;

	for( ; *name; name++ )
	{
		if(islower(*name))
			*name = toupper(*name);
	}

	return ret;
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

    hdd_read( fs->p, fs->FirstDataSector, fs->sec_buf, 1);

    hexdump(fs->sec_buf, 512, "", 0);

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

static void get_first_data_sector(tiny_fat_t *fs)
{
    fs->FirstDataSector = (fs->bpb.ResvdSecCnt + (fs->bpb.NumFATs * fs->bpb.FATSz16));
    fs->FirstDataSector += fs->bpb.HiddSec;
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

    SHOW_FLOW( 1, "total sectors = %u (%ul Mb)", total_sectors, (total_sectors*512L)/(1024*1024L) );

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

    return 0;
}



errno_t fs_start_tiny_fat( phantom_disk_partition_t *p )
{
	tiny_fat_t *fs = calloc(sizeof(tiny_fat_t),1);

	fs->p = p;

	return check_for_FAT16(fs);

}


