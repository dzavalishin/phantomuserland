/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Data types.
 *
 *
**/




#ifndef CPFS_TYPES_H
#define CPFS_TYPES_H



#include <stdio.h>
#include <stdlib.h>



typedef uint64_t cpfs_blkno_t; // disk block number
typedef uint64_t cpfs_ino_t;   // number of inode
typedef uint32_t cpfs_direntno_t; // number of a dir entry in a directory 'file'
typedef uint64_t cpfs_fpos_t;  // file position or size
typedef uint64_t cpfs_time_t;  // file c/m/a time - TODO units, 0 time?
typedef uint64_t cpfs_size_t;  // mem size


typedef int errno_t;

struct cpfs_fs;
struct cpfs_stat;


#endif // CPFS_TYPES_H

