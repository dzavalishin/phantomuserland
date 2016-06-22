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

#define CPFS_INO_DIR_BLOCKS     32
#define CPFS_INO_REC_SIZE       512

#define CPFS_INO_PER_BLK        (CPFS_BLKSIZE/CPFS_INO_REC_SIZE)

