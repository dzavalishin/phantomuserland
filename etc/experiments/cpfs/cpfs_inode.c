/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Inode related code.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"

// maps logical blocks to physical, block must be allocated
errno_t
cpfs_find_block_4_file( cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys )
{
    // TODO assert( phys );
    // Read inode first
    struct cpfs_inode inode;
    struct cpfs_inode *inode_p = cpfs_lock_ino( ino );
    if( inode_p ) inode = *inode_p;
    cpfs_unlock_ino( ino );
    if( inode_p == 0 ) return EIO;

    if( logical*CPFS_BLKSIZE > inode.fsize ) return E2BIG; // TODO err?

    if( logical < CPFS_INO_DIR_BLOCKS )
    {
        *phys = inode.blocks0[logical];
        return 0;
    }


    // TODO write indirect blocks support!
    return E2BIG;
}



errno_t
cpfs_block_4_inode( cpfs_ino_t ino, cpfs_blkno_t *oblk )
{
    if( ino >= fs_sb.ninode ) return E2BIG;

    cpfs_blkno_t blk =  fs_sb.itable_pos + (ino/CPFS_INO_PER_BLK);
    //if( blk >= fs_sb.itable_end ) return E2BIG;

    *oblk = blk;
    return 0;
}



//
// Alloc/free inode
//

#define FIC_SZ 256
static cpfs_ino_t free_inodes_cache[FIC_SZ];
static int fic_used = 0;


errno_t
cpfs_alloc_inode( cpfs_ino_t *inode )
{
    // TODO
    //cpfs_mutex_lock( fic_mutex );

    if( fic_used <= 0 )
        fic_refill();

    if( fic_used <= 0 )
    {
        //cpfs_mutex_unlock( fic_mutex );
        return EMFILE;
    }

    *inode = free_inodes_cache[--fic_used];
    // TODO
    //cpfs_mutex_unlock( fic_mutex );
    return 0;
}


void
cpfs_free_inode( cpfs_ino_t ino ) // deletes file
{
    // TODO check that inode is not used right now!

    cpfs_inode_truncate( ino ); // free all data blocks for inode, set size to 0

    cpfs_log_error("free_inode: unimpl\n");

    // TODO
    //cpfs_mutex_lock( fic_mutex );

    if( fic_used < FIC_SZ )
    {
        free_inodes_cache[fic_used++] = ino;
    }

    // TODO
    //cpfs_mutex_unlock( fic_mutex );

}


static cpfs_blkno_t last_ino_blk = -1;

void
fic_refill(void)
{
    int iblocks = fs_sb.ninode*CPFS_INO_PER_BLK;
    cpfs_blkno_t ilast = fs_sb.itable_pos + iblocks;

    // TODO
    //cpfs_mutex_lock( fic_mutex );

    //
    // fast way - alloc from end of inode space
    //

    if( last_ino_blk == -1 )
    {
        //cpfs_sb_lock();
        last_ino_blk = fs_sb.itable_end;
    }

    // Each disk block has CPFS_INO_PER_BLK inodes. As long as we have place for CPFS_INO_PER_BLK inodes in
    // free_inodes_cache and there are unused blocks in inode space, refill free inode numbers list.
    while( (last_ino_blk < ilast) && (fic_used < (FIC_SZ+CPFS_INO_PER_BLK))  )
    {
        int i;
        for( i = 0; i < CPFS_INO_PER_BLK; i++ )
        {
            free_inodes_cache[fic_used++] = (last_ino_blk-fs_sb.itable_pos) + i;
        }

        last_ino_blk++;
    }

    // TODO
    //cpfs_mutex_unlock( fic_mutex );

    if( fic_used > 0 )
        return;

    // TODO long way - scan through inodes? use map?
}


void
cpfs_inode_truncate( cpfs_ino_t ino ) // free all data blocks for inode, set size to 0
{
    // Free all data blocks for inode

    struct cpfs_inode copy;

    struct cpfs_inode *inode =  cpfs_lock_ino( ino );
    cpfs_touch_ino( ino );

    copy = *inode;

    //inode->nlinks = 0; // free it - NO, we're just truncating

    memset( inode->blocks0, 0, sizeof(inode->blocks0) );
    inode->blocks1 = 0;
    inode->blocks2 = 0;
    inode->blocks3 = 0;

    cpfs_blkno_t nblk = inode->fsize / CPFS_BLKSIZE;
    if( inode->fsize % CPFS_BLKSIZE ) nblk++;

    cpfs_unlock_ino( ino );

    // done with inode, now free disk blocks

    cpfs_blkno_t blk;
    cpfs_blkno_t phys_blk;

    for( blk = 0; (blk < nblk) && (blk < CPFS_INO_DIR_BLOCKS); blk++ )
    {
        phys_blk = inode->blocks0[blk];
        cpfs_free_disk_block( blk );
    }

    // TODO free indirect blocks


}






