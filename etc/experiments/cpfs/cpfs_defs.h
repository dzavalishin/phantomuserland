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



#define CPFS_MAX_PATH_LEN       2048
#define CPFS_MAX_FNAME_LEN       500


#define CPFS_MAX_FILES_PER_DIR  8192 // open/create timing depends on it

#define CPFS_BLKSIZE            4096

// Number of direct mapped file block numbers in inode
#define CPFS_INO_DIR_BLOCKS     32

#define CPFS_INO_REC_SIZE       512
#define CPFS_INO_PER_BLK        (CPFS_BLKSIZE/CPFS_INO_REC_SIZE)

#define CPFS_DIR_REC_SIZE       512
#define CPFS_DIR_PER_BLK        (CPFS_BLKSIZE/CPFS_DIR_REC_SIZE)


#define CPFS_SB_MAGIC           0xD0B0E0EF // superblock
#define CPFS_FL_MAGIC           0xBAD0BEEF // free list
#define CPFS_IB_MAGIC           0xBB00BB00 // indirect block list



// TODO implement me - disk io cache
#define CPFS_MAX_CONCUR_IO      64

#define CPFS_FTYPE_DIR          0040000

#endif // CPFS_DEFS_H
