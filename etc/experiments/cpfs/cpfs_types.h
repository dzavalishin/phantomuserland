/*
typedef unsigned int uint32_t;
typedef unsigned int u_int32_t;

typedef unsigned long uint64_t;

*/

#include <stdio.h>
#include <stdlib.h>



typedef uint64_t cpfs_blkno_t; // disk block number
typedef uint64_t cpfs_ino_t;   // number of inode
typedef uint32_t cpfs_direntno_t; // number of a dir entry in a directory 'file'
typedef uint64_t cpfs_fpos_t;  // file position or size
typedef uint64_t cpfs_time_t;  // file c/m/a time - TODO units, 0 time?
typedef uint64_t cpfs_size_t;  // mem size


typedef int errno_t;


