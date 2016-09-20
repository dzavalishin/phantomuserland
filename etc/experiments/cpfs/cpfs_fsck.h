/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * FSCK structures.
 *
 *
 **/

#ifndef CPFS_FSCK_H
#define CPFS_FSCK_H

#include "cpfs_types.h"
#include "cpfs_defs.h"


// FSCK makes table of all referred disk blocks (in data block zone, not counting sb/inode table blocks

//struct fsck_used_blk {
//    cpfs_blkno_t blk; // used disk block number
//    cpfs_ino_t ino; // number of referring inode
//    cpfs_blkno_t referring_blk; // block number of indirect block referring this block, or 0 if this is direct block referenced from inode
//    int pos_in_blk; // position of this block reference in indirect blk or in inode
//};
//
//
//// FSCK makes table of all used inodes
//
//struct fsck_used_blk {
//    cpfs_ino_t ino; // number of used inode
//    cpfs_ino_t ino; // number of referring inode
//
//    struct fsck_used_blk *parent;
//    struct fsck_used_blk *next;
//};

//typedef struct fsck_node {
//    cpfs_blkno_t block; //disk block. 
//    int inode_offset; // offset of ino in block = ino % CPFS_INO_PER_BLK. sdir=0 -> inode_offset=0
//    cpfs_ino_t inode; //num inode = (block-1)*CPFS_INO_PER_BLK +offset
//    int isdir;
//    
//    //cpfs_blkno_t* ref_blocks; //blocks0 + blocks are taken from indir-array*
//    //cpfs_ino_t* inodes; //array of referenced inodes
//    
//    struct fsck_node* parent;
//    struct fsck_node** children;
//    int children_size;
//} fsck_node_t;
//
//extern fsck_node_t* fsck_node_list;

#endif // CPFS_FSCK_H

