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
#include "cpfs_fsck.h"

//#include <errno.h>
#include "cpfs_errno.h"
#include <string.h>


struct cpfs_fs;

// Standard block header, all blocks must have one

typedef union cpfs_blk_header {

    struct
    {
        char fill[CPFF_BLK_HEADER_SIZE];
    };

    struct
    {
        uint32_t            magic;
    };

} cpfs_blk_header_t;



// On-disk supreblock structure
struct cpfs_sb
{
//    uint32_t            sb_magic_0;
    cpfs_blk_header_t   h;


    uint32_t            ninode;                 // Number of inodes
    cpfs_blkno_t        itable_pos;             // Disk block where inode table starts? Need?
    cpfs_blkno_t        itable_end;             // First unused block of inode table, this one and rest is not initialized. Used for fast mkfs.

    cpfs_blkno_t        disk_size;

    cpfs_blkno_t        first_unallocated;      // Number of block at end of FS we didn't use at all. From this point up to the end all blocks are free. 0 if not used. Used for fast mkfs.
    cpfs_blkno_t        free_list;              // Head of free block list, or 0 if none
    cpfs_blkno_t        free_tree;              // Head of free block tree, or 0 if none

    cpfs_blkno_t        free_count;             // Number of free blocks in FS

    uint32_t            dirty;                  // Not closed correctly
};







// On-disk directory entry structure
struct cpfs_dir_entry
{
    cpfs_ino_t          inode;
    char                name[CPFS_MAX_FNAME_LEN];
};




#define CPFS_MAX_INDIR 4

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
    cpfs_blkno_t        indir[ CPFS_MAX_INDIR ]; // indirect blocks list with growing level of indirection

    // -------------------------------------------------------------------------------------
    // NB - following is just in-memory state, ignore it on disk, will be inited on any load
#if CPFS_INODE_MUTEX
    cpfs_mutex          mutex;	// Inode/file/dir data access serialization
#endif
};

// curr size of inode is 344 bytes, we'll allocate 512 for any case






// On-disk free block list structure.
// Contents of a block in a free block list.
struct cpfs_freelist
{
    cpfs_blk_header_t   h;

    cpfs_blkno_t        next;
};



// On-disk free block tree structure.
// Contents of a tree block.
// NB! Leaf block must be filled with zeroes or 0xFF
struct cpfs_freetree
{
    cpfs_blk_header_t   h;

    cpfs_blkno_t        child[ CPFS_NODE_PTR_PER_BLK ]; 
};








// On-disk indirect data pointers block structure
// Contents of an indirect block
struct cpfs_indir
{
    cpfs_blk_header_t   h;

    cpfs_blkno_t        child[ CPFS_INDIRECT_PER_BLK ];
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




// Dile id (descriptor) - must go to OS code?
struct cpfs_fid
{
    char                used;
    char                lock;

    struct cpfs_fs *	fs;
    cpfs_ino_t  	inode;
};

typedef struct cpfs_fid cpfs_fid_t;




// unknown      - state of block is not known
// allocated    - block is allocated to file or indirect page (distinct?)
// freelist     - block is in free list or after sb.first_unallocated point
// freemap      - block is in free map (map block or leaf)
// inode        - block is in inode space
// superblk     - it is

typedef enum { bs_unknown, bs_allocated, bs_freelist, bs_freemap, bs_inode, bs_superblk } fsck_blkstate_t; // allocation state


// In-memory filesystem state
struct cpfs_fs
{
    // --------------------------------------------------------------------------------------------
    // Must be filled by caller

    int         	disk_id;                // Number of disk in disk subsystem, paramerer for disk IO functions
    cpfs_blkno_t 	disk_size;              // Disk size in blocks

    // --------------------------------------------------------------------------------------------
    // Private data follows

    struct cpfs_sb	sb;

    int                 inited;
    int                 mounted;

    // --------------------------------------------------------------------------------------------
    // Locks

    cpfs_mutex          sb_mutex;               // Taken when modify superblock - init in sb code
    cpfs_mutex          freelist_mutex;         // Taken when modify free list
    cpfs_mutex          fic_mutex;              // Free inodes cache
    cpfs_mutex          buf_mutex;              // Disk buffers
    cpfs_mutex          fdmap_mutex;            // File descriptor map - init in fdmap code
    cpfs_mutex          inode_mutex;            // Taken when work with any inode, TODO make inode buffers for concurrent inode io, use mutex to lock search
    cpfs_mutex          dir_mutex;              // Directory update/access mutex TODO hack, very inefficient - need per-inode locks for this!

    // --------------------------------------------------------------------------------------------
    // Inode allocation state

    cpfs_ino_t 		free_inodes_cache[FIC_SZ];
    int 		fic_used;

    cpfs_blkno_t 	last_ino_blk;           // Last (used? free) block in inode section

    // --------------------------------------------------------------------------------------------
    // Disk cache

    cpfs_buf_t          *buf;                   // Disk buffers
    int                 nbuf;

    //cpfs_fid_t          fdmap;                  // map of file descriptors
    //int                 nfdmap;
    //int                 fdmap_alloc;            // Last postion of allocator search

    // --------------------------------------------------------------------------------------------
    // Inode io - naive impl, one inode at time per FS

    char 		ino_lock_write;
    char 		ino_lock_used;
    cpfs_blkno_t 	ino_lock_curr_blk;
    cpfs_ino_t 		ino_lock_curr_ino;

    char 		ino_lock_data[CPFS_BLKSIZE];

    // --------------------------------------------------------------------------------------------
    // FSCK state

    int 		fsck_nWarn;
    int			fsck_nErr;

    fsck_blkstate_t 	*fsck_blk_state;        // Map of all blocks
    int                 fsck_rebuild_free;  	// Freelist corrupt, need to rebuild
};

typedef struct cpfs_fs cpfs_fs_t;


























errno_t                 cpfs_buf_init( cpfs_fs_t *fs );
errno_t			cpfs_buf_stop( cpfs_fs_t *fs );

void                    cpfs_clear_all_buf( cpfs_fs_t *fs );

errno_t                 cpfs_buf_lock( cpfs_fs_t *fs, cpfs_blkno_t blk, cpfs_buf_t **buf );
errno_t                 cpfs_buf_unlock( cpfs_fs_t *fs, cpfs_blkno_t blk, char write );



errno_t                 cpfs_init_sb( cpfs_fs_t *fs );
errno_t                 cpfs_stop_sb( cpfs_fs_t *fs );

errno_t 		cpfs_mount_sb( cpfs_fs_t *fs );

void                    cpfs_sb_lock( cpfs_fs_t *fs );
errno_t                 cpfs_sb_unlock_write( cpfs_fs_t *fs ); // if returns error, sb is not written and IS NOT UNLOCKED
void                    cpfs_sb_unlock( cpfs_fs_t *fs );

errno_t                 cpfs_write_sb( cpfs_fs_t *fs ); // just for stop fs, don't use elsewhere


void                    fic_refill( cpfs_fs_t *fs );




cpfs_blkno_t            cpfs_alloc_disk_block( cpfs_fs_t *fs );
void                    cpfs_free_disk_block( cpfs_fs_t *fs, cpfs_blkno_t blk );




errno_t                 cpfs_block_4_inode( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t *blk );
errno_t                 cpfs_find_block_4_file( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys ); // maps logical blocks to physical, block must be allocated
// allocates logical block, returns physical blk pos, block must NOT be allocated
// actually if block is allocated returns EEXIST and blk num in phys
errno_t                 cpfs_alloc_block_4_file( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys );
// returns 0 in any case, either if block found or allocated
errno_t                 cpfs_find_or_alloc_block_4_file( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys );

// inode machinery indirect block addressing workers
errno_t 		calc_indirect_positions( cpfs_fs_t *fs, cpfs_blkno_t indexes[CPFS_MAX_INDIR], int *start_index, cpfs_blkno_t logical );

//void cpfs_fsck_log(FILE *file,  cpfs_ino_t ino_in_blk, cpfs_blkno_t phys_blk, cpfs_inode inode);

struct cpfs_inode *     cpfs_lock_ino( cpfs_fs_t *fs, cpfs_ino_t ino ); // makes sure that inode is in memory and no one modifies it
void                    cpfs_touch_ino( cpfs_fs_t *fs, cpfs_ino_t ino ); // marks inode as dirty, will be saved to disk on unlock
void                    cpfs_unlock_ino( cpfs_fs_t *fs, cpfs_ino_t ino ); // flushes inode to disk before unlocking it, if touched

#if CPFS_INODE_MUTEX
// lock/unlock inode state/file data exclusive access mutex. used in directory update code mostly
void 			cpfs_ino_mutex_lock( struct cpfs_fs *fs, struct cpfs_inode *ip );
void 			cpfs_ino_mutex_unlock( struct cpfs_fs *fs, struct cpfs_inode *ip );
#endif // CPFS_INODE_MUTEX

//cpfs_ino_t            cpfs_alloc_inode( void );
errno_t                 cpfs_alloc_inode( cpfs_fs_t *fs, cpfs_ino_t *inode );
errno_t                 cpfs_free_inode( cpfs_fs_t *fs, cpfs_ino_t ino ); // deletes file

void                    cpfs_inode_init_defautls( cpfs_fs_t *fs, struct cpfs_inode *ii );

errno_t                 cpfs_free_indirect( cpfs_fs_t *fs, cpfs_blkno_t indir_blk, int depth );


errno_t                 cpfs_update_ino_atime( cpfs_fs_t *fs, cpfs_ino_t ino );
errno_t                 cpfs_update_ino_mtime( cpfs_fs_t *fs, cpfs_ino_t ino );





// TODO problem: linear scan can be very slow, need tree?

errno_t                 cpfs_alloc_dirent( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t file_ino ); // allocate a new dir entry in a dir
errno_t                 cpfs_free_dirent( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname ); // free dir entry (write 0 to inode field)
errno_t                 cpfs_namei( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t *file_ino ); // find name

// was errno_t                 cpfs_namei( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t *file_ino, int remove ); // find name

// General dir scan func
errno_t                 cpfs_scan_dir( cpfs_fs_t *fs, cpfs_ino_t dir_ino, dir_scan_func_t f, void *farg );


errno_t                 cpfs_is_dir( cpfs_fs_t *fs, cpfs_ino_t dir_ino, int *yesno); // Check if inode contains a directory
errno_t			cpfs_is_empty_dir( cpfs_fs_t *fs, cpfs_ino_t dir_ino );
errno_t                 cpfs_dir_has_entry( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *name );

errno_t			cpfs_dump_dir( cpfs_fs_t *fs, cpfs_ino_t dir_ino );


// Finds/reads/creates/updates dir entry
//errno_t                 cpfs_dir_scan( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t *file_ino, int flags );

#define CPFS_DIR_SCAN_MUST_EXIST        (1<<1) // returns ENOENT if not exist
#define CPFS_DIR_SCAN_WRITE             (1<<2) // writes given ino to dir entry, not reads

// TODO dirent name cache - in memory hash? Not really as usual client software opens file once in its life?

// Parse path name and descend into the last directory, returning last part of path and inode of last directory
errno_t                 cpfs_descend_dir( cpfs_fs_t *fs, const char *path, const char **last, cpfs_ino_t *last_dir_ino );




void *                  cpfs_lock_blk(   cpfs_fs_t *fs, cpfs_blkno_t blk ); // makes sure that block is in memory 
void                    cpfs_touch_blk(  cpfs_fs_t *fs, cpfs_blkno_t blk ); // marks block as dirty, will be saved to disk on unlock
void                    cpfs_unlock_blk( cpfs_fs_t *fs, cpfs_blkno_t blk ); // flushes block to disk before unlocking it, if touched



// Internal read/wrire impl, used by user calls and internal directory io code

errno_t                 cpfs_ino_file_read  ( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_size_t pos, void *data, cpfs_size_t size,  cpfs_blkno_t *phys_blk);
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

