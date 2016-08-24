/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal structures.
 *
 *
**/

#ifndef CPFS_LOCAL_H
#define CPFS_LOCAL_H


#include "cpfs_types.h"
#include "cpfs_defs.h"

#include <errno.h>
#include <string.h>


struct cpfs_fs;



// On-disk supreblock structure
struct cpfs_sb
{
    uint32_t            sb_magic_0;

    uint32_t            ninode;                 // Number of inodes
    cpfs_blkno_t        itable_pos;             // Disk block where inode table starts? Need?
    cpfs_blkno_t        itable_end;             // First unused block of inode table, this one and rest is not initialized. Used for fast mkfs.

    cpfs_blkno_t        disk_size;
    cpfs_blkno_t        first_unallocated;      // Number of block at end of FS we didn't use at all. From this point up to the end all blocks are free. 0 if not used. Used for fast mkfs.
    cpfs_blkno_t        free_list;              // Head of free block list, or 0 if none

    cpfs_blkno_t        free_count;             // Number of free blocks in FS

    uint32_t            dirty;                  // Not closed correctly
};

//extern struct cpfs_sb fs_sb;


// On-disk directory entry structure
struct cpfs_dir_entry
{
    cpfs_ino_t          inode;
    char                name[CPFS_MAX_FNAME_LEN];
};


// On-disk i-node entry structure
struct cpfs_inode
{
    cpfs_fpos_t         fsize;
    uint32_t            nlinks; // allways 0 or 1 in this verstion, made for future extensions, if 0 - inode record is free.
    uint32_t            ftype; // nonzero = dir for now

    cpfs_time_t         ctime; // created
    cpfs_time_t         atime; // accessed
    cpfs_time_t         mtime; // modified
    cpfs_time_t         vtime; // version of file forked (not used, will mark time when this backup version of file is forked from main version)

    cpfs_blkno_t        acl;    // disk block with access control list, unused now
    cpfs_blkno_t        log;    // disk block of audit log list, not used now, will contain log of all operations to support transactions

    cpfs_blkno_t        blocks0[CPFS_INO_DIR_BLOCKS];
    cpfs_blkno_t        blocks1; // Block no of list of 1-level indirect blocks list
    cpfs_blkno_t        blocks2; // Block no of list of 2-level indirect blocks list
    cpfs_blkno_t        blocks3; // Block no of list of 3-level indirect blocks list
};

// curr size of inode is 344 bytes, we'll allocate 512 for any case

// On-disk free block list structure
// Contents of a block in a free block list
struct cpfs_freelist
{
    uint32_t            fl_magic;
    uint32_t            unused; // next fld is 64 bit, make sure it is aligned

    cpfs_blkno_t        next;
};

#define INDIR_CNT ((CPFS_BLKSIZE/sizeof(cpfs_blkno_t))-1)

// On-disk indirect data pointers block structure
// Contents of an indirect block
struct cpfs_indir
{
    uint32_t            ib_magic; // TODO Use me
    uint32_t            unused; // next fld is 64 bit, make sure it is aligned

    cpfs_blkno_t        child[INDIR_CNT];
};





// Free inodes cache size
#define FIC_SZ 256



// In-memory disk io buffer
struct cpfs_buf
{
    int                 lock;                   // used by some i/o right now, can't be freed

    char                used; 			// allocated, contains correct data for corresponding disk block
    char                write;                  // must be written on release
    //char                shared;                 // can be shared between users

    cpfs_blkno_t        blk;
    char                data[CPFS_BLKSIZE];
};

typedef struct cpfs_buf cpfs_buf_t;



struct cpfs_fid
{
    char                used;
    char                lock;

    struct cpfs_fs *	fs;
    cpfs_ino_t  	inode;
};

typedef struct cpfs_fid cpfs_fid_t;



// In-memory filesystem state
struct cpfs_fs
{
    // must be filled by caller

    int         	disk_id;                // Number of disk in disk subsystem, paramerer for disk IO functions
    cpfs_blkno_t 	disk_size;

    // private data

    struct cpfs_sb	sb;

    int                 inited;
    int                 mounted;

    cpfs_mutex          sb_mutex;               // Taken when modify superblock
    cpfs_mutex          freelist_mutex;         // Taken when modify free list
    cpfs_mutex          fic_mutex;              // Free inodes cache
    cpfs_mutex          buf_mutex;              // Disk buffers
    cpfs_mutex          fdmap_mutex;            // File descriptor map

    cpfs_ino_t 		free_inodes_cache[FIC_SZ];
    int 		fic_used;

    cpfs_blkno_t 	last_ino_blk;           // Last (used? free) block in inode section

    cpfs_buf_t          *buf;                   // Disk buffers
    int                 nbuf;

    //cpfs_fid_t          fdmap;                  // map of file descriptors
    //int                 nfdmap;
    //int                 fdmap_alloc;            // Last postion of allocator search

};

typedef struct cpfs_fs cpfs_fs_t;









errno_t                 cpfs_buf_init( cpfs_fs_t *fs );
void                    cpfs_clear_all_buf( cpfs_fs_t *fs );

errno_t                 cpfs_buf_lock( cpfs_fs_t *fs, cpfs_blkno_t blk, cpfs_buf_t **buf );
errno_t                 cpfs_buf_unlock( cpfs_fs_t *fs, cpfs_blkno_t blk, char write );



errno_t                 cpfs_init_sb( cpfs_fs_t *fs );
errno_t 		cpfs_mount_sb( cpfs_fs_t *fs );

void                    cpfs_sb_lock( cpfs_fs_t *fs );
errno_t                 cpfs_sb_unlock_write( cpfs_fs_t *fs ); // if returns error, sb is not written and IS NOT UNLOCKED
void                    cpfs_sb_unlock( cpfs_fs_t *fs );

errno_t                 cpfs_write_sb( cpfs_fs_t *fs ); // just for stop fs, don't use elsewhere


void                    fic_refill( cpfs_fs_t *fs );


// TODO multiple fs?


cpfs_blkno_t            cpfs_alloc_disk_block( cpfs_fs_t *fs );
void                    cpfs_free_disk_block( cpfs_fs_t *fs, cpfs_blkno_t blk );


errno_t                 cpfs_block_4_inode( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t *blk );
errno_t                 cpfs_find_block_4_file( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys ); // maps logical blocks to physical, block must be allocated
// allocates logical block, returns physical blk pos, block must NOT be allocated
// actually if block is allocated returns EEXIST and blk num in phys
errno_t                 cpfs_alloc_block_4_file( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys );
// returns 0 in any case, either if block found or allocated
errno_t                 cpfs_find_or_alloc_block_4_file( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys );


struct cpfs_inode *     cpfs_lock_ino( cpfs_fs_t *fs, cpfs_ino_t ino ); // makes sure that inode is in memory and no one modifies it
void                    cpfs_touch_ino( cpfs_fs_t *fs, cpfs_ino_t ino ); // marks inode as dirty, will be saved to disk on unlock
void                    cpfs_unlock_ino( cpfs_fs_t *fs, cpfs_ino_t ino ); // flushes inode to disk before unlocking it, if touched



//cpfs_ino_t            cpfs_alloc_inode( void );
errno_t                 cpfs_alloc_inode( cpfs_fs_t *fs, cpfs_ino_t *inode );
errno_t                 cpfs_free_inode( cpfs_fs_t *fs, cpfs_ino_t ino ); // deletes file

void                    cpfs_inode_init_defautls( cpfs_fs_t *fs, struct cpfs_inode *ii );



// TODO problem: linear scan can be very slow, need tree?

errno_t                 cpfs_alloc_dirent( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t file_ino ); // allocate a new dir entry in a dir
//errno_t                       cpfs_free_dirent( cpfs_ino_t dir_ino, const char *fname ); // free dir entry (write 0 to inode field)
errno_t                 cpfs_namei( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t *file_ino, int remove ); // find name


// Finds/reads/creates/updates dir entry
errno_t                 cpfs_dir_scan( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t *file_ino, int flags );

#define CPFS_DIR_SCAN_MUST_EXIST        (1<<1) // returns ENOENT if not exist
#define CPFS_DIR_SCAN_WRITE             (1<<2) // writes given ino to dir entry, not reads

// TODO dirent name cache - in memory hash? Not really as usual client software opens file once in its life?

// Parse path name and descend into the last directory, returning last part of path and inode of last directory
errno_t                 cpfs_descend_dir( cpfs_fs_t *fs, const char *path, const char **last, cpfs_ino_t *last_dir_ino );




void *                  cpfs_lock_blk(   cpfs_fs_t *fs, cpfs_blkno_t blk ); // makes sure that block is in memory 
void                    cpfs_touch_blk(  cpfs_fs_t *fs, cpfs_blkno_t blk ); // marks block as dirty, will be saved to disk on unlock
void                    cpfs_unlock_blk( cpfs_fs_t *fs, cpfs_blkno_t blk ); // flushes block to disk before unlocking it, if touched



// Internal read/wrire impl, used by user calls and internal directory io code

errno_t                 cpfs_ino_file_read  ( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_size_t pos, void *data, cpfs_size_t size );
errno_t                 cpfs_ino_file_write ( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_size_t pos, const void *data, cpfs_size_t size );

errno_t                 cpfs_inode_truncate( cpfs_fs_t *fs, cpfs_ino_t ino ); // free all data blocks for inode, set size to 0
errno_t                 cpfs_fsize( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_size_t *size );
errno_t                 cpfs_inode_update_fsize( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_size_t size );


// File descriptor map

errno_t                 cpfs_fdmap_init( void );

errno_t                 cpfs_fdmap_alloc( cpfs_fs_t *fs, cpfs_ino_t ino, int *fd );
errno_t                 cpfs_fdmap_free( int fd );

errno_t                 cpfs_fdmap_lock( int fd, cpfs_fid_t **fid );
errno_t                 cpfs_fdmap_unlock( int fd );

errno_t                 cpfs_fdmap_get( int fd, cpfs_ino_t *ino, cpfs_fs_t **fs );
int                     cpfs_fdmap_is_inode_used( cpfs_fs_t *fs, cpfs_ino_t ino );




#endif // CPFS_LOCAL_H
