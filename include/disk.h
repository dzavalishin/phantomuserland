#ifndef DISK_H
#define DISK_H

#include <errno.h>
#include <hal.h>
#include "pager_io_req.h"

#define MAX_DISK_PARTITIONS 64

#define PARTITION_NAME_LEN 64

struct phantom_disk_partition
{
    int         block_size;     // Block size, bytes
    long        shift;          // From start of base, blocks
    long        size;           // Partition size, blocks

    char        name[PARTITION_NAME_LEN];       // Partition name
    char        label[PARTITION_NAME_LEN];      // Partition label or additional info

    int         flags;
    int         type;           // 0-0xFF is PC part types

    struct phantom_disk_partition *base; //

    void        *specific;      // Specific data (following methods know how to handle)

#if !IO_RQ_SLEEP
    errno_t     (*syncRead)( struct phantom_disk_partition *p, void *to, long blockNo, int nBlocks );
    errno_t     (*syncWrite)( struct phantom_disk_partition *p, const void *from, long blockNo, int nBlocks );
#endif

// NB!! pager_io_request's disk block no is IGNORED! 
    errno_t     (*asyncIo)( struct phantom_disk_partition *p,  pager_io_request *rq );
    //errno_t     (*asyncWrite)( struct phantom_disk_partition *p, pager_io_request *rq );

};


typedef struct phantom_disk_partition phantom_disk_partition_t;



/** Partition pretends to have Phantom FS */
#define PART_FLAG_IS_PHANTOM_TYPE 	0x0001

/** Partition really has Phantom FS superblock */
#define PART_FLAG_IS_PHANTOM_FSSB 	0x0002




/** Partition is marked as bootable */
#define PART_FLAG_IS_BOOTABLE 		0x0010

/** Partition is, in turn, subdivided and is not supposed to be accessed per se */
#define PART_FLAG_IS_DIVIDED 		0x0020

/** Partition is whole disk */
#define PART_FLAG_IS_WHOLE_DISK		0x0040


phantom_disk_partition_t *phantom_create_partition_struct(phantom_disk_partition_t *base, long shift, long size);

errno_t phantom_register_disk_drive(phantom_disk_partition_t *p);



// These work with 4096 byte blocks
errno_t phantom_sync_read_block( phantom_disk_partition_t *, void *to, long blockNo, int nBlocks );
errno_t phantom_sync_write_block( phantom_disk_partition_t *, const void *from, long blockNo, int nBlocks );

// These work with native sized (512byte) sectors 
errno_t phantom_sync_read_sector( phantom_disk_partition_t *, void *to, long sectorNo, int nSectors );
errno_t phantom_sync_write_sector( phantom_disk_partition_t *, const void *from, long sectorNo, int nSectors );


/** Start async disk io operation */
void disk_enqueue( phantom_disk_partition_t *p, pager_io_request *rq );



#endif// DISK_H
