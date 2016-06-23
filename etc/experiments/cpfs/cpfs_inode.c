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


static errno_t find_or_create_indirect( cpfs_blkno_t *base, cpfs_blkno_t displ, int indirection, cpfs_blkno_t *phys, int create, int *created );



// maps logical blocks to physical, block must be allocated
errno_t
cpfs_find_block_4_file( cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys )
{
    cpfs_assert( phys != 0 );

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


// allocates logical block, returns physical blk pos, block must NOT be allocated
// actually if block is allocated returns eexist and blk num in phys
errno_t
cpfs_alloc_block_4_file( cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys )
{
    cpfs_assert( phys != 0 );

    // Read inode first
    struct cpfs_inode *ip = cpfs_lock_ino( ino );
    if( ip == 0 )
    {
        cpfs_unlock_ino( ino );
        return EIO;
    }

    if( logical < CPFS_INO_DIR_BLOCKS )
    {
        if( ip->blocks0[logical] )
        {
            *phys = ip->blocks0[logical];
            cpfs_unlock_ino( ino );
            return EEXIST;
        }

        cpfs_blkno_t new = cpfs_alloc_disk_block();
        if( !new )
        {
            cpfs_unlock_ino( ino );
            return ENOSPC;
        }

        *phys = ip->blocks0[logical] = new;
        cpfs_touch_ino( ino );
        cpfs_unlock_ino( ino );
        return 0;
    }

    cpfs_unlock_ino( ino );
    // TODO write indirect blocks support!
    return E2BIG;

}

errno_t
cpfs_find_or_alloc_block_4_file( cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys )
{
    errno_t rc = cpfs_alloc_block_4_file( ino, logical, phys );
    if( rc == EEXIST ) rc = 0;
    return rc;
}


//
// Indirect blocks support
//
// base contains 0 or block num of indirect block. If it is 0 and create is nonzero, block will be created.
// displ contains logical block number relative to start of our part of map, ie from 0 to max size of our indirect part
// indirection is level of indirection. If 1, we can't go any further, *base contains page with phys block numbers
// create - instructs us to allocate block if unallocated
// created set to nonzero if we did create *base
//

static errno_t find_or_create_indirect( cpfs_blkno_t *base, cpfs_blkno_t displ, int indirection, cpfs_blkno_t *phys, int create, int *created )
{
    int icreated = 0;

    cpfs_assert( base != 0 );
    cpfs_assert( phys != 0 );
    cpfs_assert( created != 0 );

    // we're empty and can't create, fail
    if( (*base == 0) && !create )
        return ENOENT;

    cpfs_blkno_t iblk = *base;
    cpfs_blkno_t fblk;

    if( *base == 0 )
    {
        // Allocate indirect page
        iblk = cpfs_alloc_disk_block();
        if( !iblk )
            return ENOSPC;

        *base = iblk;
        *created = 1;
        icreated = 1;
    }

    // Now we have indirect page
    struct cpfs_indir * ib = cpfs_lock_blk( iblk );
    if( ib == 0 )
    {
        cpfs_unlock_blk( iblk );
        return EIO;
    }

    if( icreated )
    {
        memset( ib, 0, sizeof( *ib ) );
        ib->ib_magic = CPFS_IB_MAGIC;
        cpfs_touch_blk( iblk );
    }

    if( indirection == 1 )
    {
        // just process
        cpfs_assert( displ < INDIR_CNT );

        if( ib->child[displ] != 0 )
        {
            *phys = ib->child[displ];
            cpfs_unlock_blk( iblk );
            return 0;
        }

        cpfs_unlock_blk( iblk );

        if( !create )
        {
            return ENOENT;
        }

        // Allocate indirect page
        fblk = cpfs_alloc_disk_block();
        if( !fblk )
            return ENOSPC;

        ib = cpfs_lock_blk( iblk );
        if( ib == 0 )
        {
            cpfs_unlock_blk( iblk );
            cpfs_free_disk_block( fblk );
            return EIO;
        }

        ib->child[displ] = fblk;
        cpfs_touch_blk( iblk );
        cpfs_unlock_blk( iblk );
        *phys = ib->child[displ];
        return 0;
    }




    return EINVAL;
}



//
// Disk blk num containing inode
//


errno_t
cpfs_block_4_inode( cpfs_ino_t ino, cpfs_blkno_t *oblk )
{
    if( ino >= fs_sb.ninode )
    {
        cpfs_log_error("cpfs_block_4_inode: ino (%d) >= fs_sb.ninode (%d)", ino, fs_sb.ninode );
        return E2BIG;
    }

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
cpfs_mutex fic_mutex;

errno_t
cpfs_alloc_inode( cpfs_ino_t *inode )
{
    cpfs_mutex_lock( fic_mutex );

    if( fic_used <= 0 )
        fic_refill();

    if( fic_used <= 0 )
    {
        cpfs_mutex_unlock( fic_mutex );
        return EMFILE;
    }

    *inode = free_inodes_cache[--fic_used];
    cpfs_mutex_unlock( fic_mutex );
    return 0;
}


void
cpfs_free_inode( cpfs_ino_t ino ) // deletes file
{
    // TODO check that inode is not used right now!

    cpfs_inode_truncate( ino ); // free all data blocks for inode, set size to 0

    // TODO free inode!
    cpfs_log_error("free_inode: unimpl\n");

    cpfs_mutex_lock( fic_mutex );

    if( fic_used < FIC_SZ )
    {
        free_inodes_cache[fic_used++] = ino;
        cpfs_mutex_unlock( fic_mutex );
        return;
    }

    cpfs_mutex_unlock( fic_mutex );
}


static cpfs_blkno_t last_ino_blk = -1;

void
fic_refill(void)
{
    int iblocks = fs_sb.ninode*CPFS_INO_PER_BLK;
    cpfs_blkno_t ilast = fs_sb.itable_pos + iblocks;

    cpfs_mutex_lock( fic_mutex );

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

    // NB! Inode table growth code assumes we eat inode space block by block

    //while( (last_ino_blk < ilast) && (fic_used < (FIC_SZ+CPFS_INO_PER_BLK))  )
    if( (last_ino_blk < ilast) && (fic_used < (FIC_SZ+CPFS_INO_PER_BLK))  )
    {
        int i;
        for( i = 0; i < CPFS_INO_PER_BLK; i++ )
        {
            free_inodes_cache[fic_used++] = (last_ino_blk-fs_sb.itable_pos) + i;
        }

        last_ino_blk++;
    }

    cpfs_mutex_unlock( fic_mutex );

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



errno_t
cpfs_fsize( cpfs_ino_t ino, cpfs_size_t *size )
{
    // asser size

    // read in inode to find out file len

    //struct cpfs_inode inode;
    struct cpfs_inode *inode_p = cpfs_lock_ino( ino );

    if( inode_p ) *size = inode_p->fsize;

    cpfs_unlock_ino( ino );

    if( inode_p == 0 ) return EIO;
    return 0;
}


errno_t
cpfs_inode_update_fsize( cpfs_ino_t ino, cpfs_size_t size )
{
    struct cpfs_inode *inode_p = cpfs_lock_ino( ino );

    if( inode_p == 0 )
    {
        cpfs_unlock_ino( ino ); // TODO check if we need to unlock anything if lock failed
        return EIO;
    }

    if( size > inode_p->fsize )
    {
        inode_p->fsize = size;
        cpfs_touch_ino( ino );
    }

    cpfs_unlock_ino( ino );

    return 0;

}



/**
 *
 * Fill inode structure with reasonable default values. Assume it is created.
 *
**/
void
cpfs_inode_init_defautls( struct cpfs_inode *ii )
{
    ii->fsize = 0;
    ii->nlinks = 1;
    ii->ftype = 0;

    ii->acl = 0; // unused now
    ii->log = 0; // unused now

    memset( ii->blocks0, 0, sizeof(ii->blocks0) );
    ii->blocks1 = 0;
    ii->blocks2 = 0;
    ii->blocks3 = 0;

    ii->ctime = cpfs_get_current_time();
    ii->atime = ii->ctime;
    ii->mtime = ii->ctime;
    ii->vtime = 0;

}

