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
#include <stdint.h>



typedef uint64_t cpfs_blkno_t; // disk block number
typedef uint64_t cpfs_ino_t;   // number of inode
typedef uint32_t cpfs_direntno_t; // number of a dir entry in a directory 'file'
typedef uint64_t cpfs_fpos_t;  // file position or size
typedef uint64_t cpfs_time_t;  // file c/m/a time - TODO units, 0 time?
typedef uint64_t cpfs_size_t;  // mem size


typedef int errno_t;

struct cpfs_fs;
struct cpfs_dir_entry;

struct cpfs_stat
{
    cpfs_fpos_t         fsize;
    uint32_t            nlinks; // allways 0 or 1 in this verstion, made for future extensions, if 0 - inode record is free.
    uint32_t            ftype; // nonzero = dir for now

    cpfs_time_t         ctime; // created
    cpfs_time_t         atime; // accessed
    cpfs_time_t         mtime; // modified
    //cpfs_time_t         vtime; // version of file forked (not used, will mark time when this backup version of file is forked from main version)

    char _fill[256]; // let us expand later
};

typedef struct cpfs_stat cpfs_stat_t;


// TODO dir_scan_remove, dir_scan_update
typedef enum { dir_scan_continue, dir_scan_success, dir_scan_error } dir_scan_ret_t;

typedef dir_scan_ret_t (*dir_scan_func_t)( struct cpfs_fs *fs, struct cpfs_dir_entry *de, void *farg );


#endif // CPFS_TYPES_H

