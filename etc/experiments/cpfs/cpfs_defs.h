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


#define CPFS_SB_MAGIC           0xBAD0BEEF

// TODO implement me - disk io cache
#define CPFS_MAX_CONCUR_IO      64

// TODO use Unix binary value here
#define CPFS_FTYPE_DIR          01000

