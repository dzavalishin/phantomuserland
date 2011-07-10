/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Block io - partitions/hierarchical processing modules.
 *
 *
**/

#ifndef DISK_H
#define DISK_H

#include <errno.h>
#include <kernel/pool.h>
#include <hal.h>
#include "pager_io_req.h"

// -----------------------------------------------------------------------
// New handle based stuff
// -----------------------------------------------------------------------


typedef struct partition_handle
{
    pool_handle_t       h;
} partition_handle_t;

partition_handle_t      dpart_create(partition_handle_t base, long shift, long size);

// These work with 4096 byte blocks
errno_t dpart_read_block( partition_handle_t h, void *to, long blockNo, int nBlocks );
errno_t dpart_write_block( partition_handle_t h, const void *from, long blockNo, int nBlocks );

// These work with native sized (512byte) sectors 
errno_t dpart_read_sector( partition_handle_t h, void *to, long sectorNo, int nSectors );
errno_t dpart_write_sector( partition_handle_t h, const void *from, long sectorNo, int nSectors );


/** Start async disk io operation */
void dpart_enqueue( partition_handle_t h, pager_io_request *rq );
void dpart_release_async( pool_handle_t h );


// -----------------------------------------------------------------------
// Old stuff
// -----------------------------------------------------------------------


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

    partition_handle_t self; // my handle - to release it on async io done

    partition_handle_t baseh;
    struct phantom_disk_partition *base; //

    void        *specific;      // Specific data (following methods know how to handle)

    // NB!! pager_io_request's disk block no is IGNORED! 
    errno_t     (*asyncIo)( struct phantom_disk_partition *p, pager_io_request *rq );

    // Attempt to remove this request from queue, if possible
    // Snapshot code relies on it a lot
    errno_t     (*dequeue)( struct phantom_disk_partition *p, pager_io_request *rq );

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


phantom_disk_partition_t *phantom_create_partition_struct(phantom_disk_partition_t *base, long shift, long size) __attribute__((deprecated));

errno_t phantom_register_disk_drive(phantom_disk_partition_t *p);



// These work with 4096 byte blocks
errno_t phantom_sync_read_block( phantom_disk_partition_t *, void *to, long blockNo, int nBlocks );
errno_t phantom_sync_write_block( phantom_disk_partition_t *, const void *from, long blockNo, int nBlocks );

// These work with native sized (512byte) sectors 
errno_t phantom_sync_read_sector( phantom_disk_partition_t *, void *to, long sectorNo, int nSectors );
errno_t phantom_sync_write_sector( phantom_disk_partition_t *, const void *from, long sectorNo, int nSectors );


/** Start async disk io operation */
void disk_enqueue( phantom_disk_partition_t *p, pager_io_request *rq );

/** Attempt to remove request from queue */
errno_t disk_dequeue( struct phantom_disk_partition *p, pager_io_request *rq );


//! Return partition for paging (Phantom "FS")
phantom_disk_partition_t *select_phantom_partition(void);

void dump_partition(phantom_disk_partition_t *p);

#endif// DISK_H
