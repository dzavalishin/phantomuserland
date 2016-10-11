/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Settings.
 *
 *
**/

#ifndef CPFS_DEFS_H
#define CPFS_DEFS_H

#include "cpfs_types.h"


// -----------------------------------------------------------------------------------------
//
// NON-Tunable parameters and definitions
//
// -----------------------------------------------------------------------------------------


#define CPFS_MAX_PATH_LEN       2048 // ? Seems to be tunable
#define CPFS_MAX_FNAME_LEN       500


#define CPFS_MAX_FILES_PER_DIR  8192 // open/create timing depends on it - tunable?

#define CPFS_BLKSIZE            4096
#define CPFS_BLOCKSIZE CPFS_BLKSIZE

// TODO each block (incl data) must have header of this size
#define CPFF_BLK_HEADER_SIZE	96

// Number of direct mapped file block numbers in inode
#define CPFS_INO_DIR_BLOCKS     32

// How many block pointers there are in indirect block
#define CPFS_INDIRECT_PER_BLK	((CPFS_BLKSIZE-CPFF_BLK_HEADER_SIZE)/(sizeof(cpfs_blkno_t)))

#define CPFS_INO_REC_SIZE       512
#define CPFS_INO_PER_BLK        (CPFS_BLKSIZE/CPFS_INO_REC_SIZE)

#define CPFS_DIR_REC_SIZE       512
#define CPFS_DIR_PER_BLK        (CPFS_BLKSIZE/CPFS_DIR_REC_SIZE)


// How many leaf/node pointers there are in free tree block
#define CPFS_NODE_PTR_PER_BLK	((CPFS_BLKSIZE-CPFF_BLK_HEADER_SIZE)/(sizeof(cpfs_blkno_t)))


#define CPFS_SB_MAGIC           0xD0B0E0EF // superblock
#define CPFS_FL_MAGIC           0xBAD0BEEF // free list
#define CPFS_FT_MAGIC           0x0BADBEEF // free tree
#define CPFS_IB_MAGIC           0xBB00BB00 // indirect block list


#define CPFS_FTYPE_DIR          0040000         // Actually can be redefined, but is stored on disk

// -----------------------------------------------------------------------------------------
//
// Tunable parameters
//
// -----------------------------------------------------------------------------------------


// Controls disk io cache size
#define CPFS_MAX_CONCUR_IO      64



// -----------------------------------------------------------------------------------------
//
// Enable/disable parts of code
//
// -----------------------------------------------------------------------------------------


#define CPFS_UPDATE_ATIME 1

// temp, development
#define CPFS_INODE_MUTEX 0



#endif // CPFS_DEFS_H
