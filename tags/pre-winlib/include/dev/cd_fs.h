/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * CD filesystem structure and fs driver internals.
 *
**/

#ifndef CD_FS_H
#define CD_FS_H

typedef struct iso_vol_date {
    char                year[4];                        // year                                         (*not* ASCIIZ)
    char                month[2];                       // month (1-12)                         (*not* ASCIIZ)
    char                day[2];                         // day of the month (1-31)      (*not* ASCIIZ)
    char                hour[2];                        // hour (00-23)                         (*not* ASCIIZ)
    char                minute[2];                      // minute (00-59)                       (*not* ASCIIZ)
    char                second[2];                      // second (00-59)                       (*not* ASCIIZ)
    char                hundrSecond[2];         // hundr. of a sec (00-99)      (*not* ASCIIZ)
    signed char         gmtOffset;                      // offset from GMT, in 15-minute intervals
} __packed iso_vol_date;


typedef struct iso_volume_descriptor {
    unsigned char       type;                   // ISO_VD_PRIMARY / ISO_VD_SUPPLEMENTARY
    char                id[5];                  // ISO_VD_ID
    unsigned char       undef1;                 // 1
    unsigned char       undef2;                 // 0
    unsigned char       systemID[32];           // System Identifier            (*not* ASCIIZ)
    unsigned char       volumeID[32];           // Volume Identifier            (*not* ASCIIZ)
    unsigned char       undef3[8];              // all 0
    unsigned long       numSectors[2];          // Total Number Of Sectors      (little+big endian)
    unsigned char       undef4[32];             // all 0
    unsigned short      volSetSize[2];          // Volume Set Size                      (little+big endian)
    unsigned short      volSeqNumber[2];        // Volume Sequence Number       (little+big endian)
    unsigned short      sectorSize[2];          // Sector Size                          (little+big endian)
    unsigned long       pathTblSize[2];         // Path Table Size (bytes)      (little+big endian)
    unsigned long       lePathTbl1Sector;       // First sector of LE Path Table
    unsigned long       lePathTbl2Sector;       // First sector of 2nd LE Path Table
    unsigned long       bePathTbl1Sector;       // First sector of BE Path Table
    unsigned long       bePathTbl2Sector;       // First sector of 2nd BE Path Table

    /* Root Directory Entry */
    unsigned char       rootDirEntry[34];       // To use, cast to iso_dir_entry

    char                volumeSetID[128];       // Volume Set Identifier        (*not* ASCIIZ)
    char                publisherID[128];       // Publisher Identifier         (*not* ASCIIZ)
    char                dataPreparerID[128];    // Data Preparer Identifier (*not* ASCIIZ)
    char                applicationID[128];     // Application Identifier       (*not* ASCIIZ)

    char                copyrightFile[37];      // Copyright File Identifier(*not* ASCIIZ)
    char                abstractFile[37];       // Abstract File Identifier     (*not* ASCIIZ)
    char                bibliogrFile[37];       // Bibliographical File Id.     (*not* ASCIIZ)

    iso_vol_date        dateCreation;           // Date and Time of volume creation
    iso_vol_date        dateModification;       // Date and Time of volume modification
    iso_vol_date        dateExpires;            // Date and Time that volume expires
    iso_vol_date        dateEffective;          // Date and Time that volume comes effective

    unsigned char       undef5;                 // 1
    unsigned char       undef6;                 // 0
    unsigned char       forAppUse[512];         // usually zeroes
    unsigned char       undef7[653];            // all 0
} __packed cd_vol_t;


typedef struct iso_dir_entry {
    unsigned char       recordLength;           // Total length of this record
    unsigned char       extAttrSecCount;        // Number of sectors in extended attr record
    unsigned long       dataStartSector[2];     // Sector # of 1st data sector for file/dir
    unsigned long       dataLength[2];          // Length of data for file/directory
    unsigned char       year;                   // Years since 1900
    unsigned char       month;                  // Month (1=jan, etc)
    unsigned char       day;                    // Day of the month (1-31)
    unsigned char       hour;                   // Hour (0-23)
    unsigned char       minute;                 // Minute (0-59)
    unsigned char       second;                 // Second (0-59)
    signed char         gmtOffset;              // GMT offset in 15 minutes intervals
    unsigned char       flags;                  // Flags
    unsigned char       unitSize;               // Unit size for interleaved file (0)
    unsigned char       gapSize;                // Gap size for interleaved file (0)
    unsigned short      volSeqNumber[2];        // Volume sequence number
    unsigned char       nameLength;             // Length of name
    char                name[32];               // Name + padding (nameLength [+1])
} __packed iso_dir_entry;

#define CD_ENTRY_FLAG_HIDDEN 	(1<<0)
#define CD_ENTRY_FLAG_DIR 	(1<<1)


typedef struct iso_path_entry {
    unsigned char       nameLength;             // Length of directory name
    unsigned char       extAttrSecCount;        // Number of sectors in extended attr record
    unsigned long       dataStartSector;        // Sector # of 1st data sector for directory
    unsigned short      parentDirRec;           // Record # of parent directory
    unsigned char       name[32];               // Name of this entry (nameLength bytes)
} __packed iso_path_entry;


// FS state

#include <disk.h>
#include <unix/uufile.h>
#include <kernel/disk_cache.h>



typedef struct
{
    bool                init;
    int                 joliet_level;

    phantom_disk_partition_t *p;

    cache_t             *cache;

    cd_vol_t 		volume_descr;
    iso_dir_entry 	root_dir; // TODO kill - is in vol descr!
}
cdfs_t;


typedef struct
{
    iso_dir_entry 	e;
}
cdfs_file_t;

uufs_t *cdfs_create_fs( cdfs_t *impl );


#define CD_SECT_SIZE 2048
#define CD_BYTES_TO_SECTORS(__b) (((__b)+CD_SECT_SIZE-1)/CD_SECT_SIZE)


#endif // CD_FS_H
