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


static errno_t find_or_create_indirect( cpfs_fs_t *fs, cpfs_blkno_t *base, cpfs_blkno_t displ, int indirection, cpfs_blkno_t *phys, int create, int *created );



// maps logical blocks to physical, block must be allocated
errno_t
cpfs_find_block_4_file( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys )
{
    cpfs_assert( phys != 0 );

    // Read inode first
    struct cpfs_inode inode;
    struct cpfs_inode *inode_p = cpfs_lock_ino( fs, ino );
    if( inode_p ) inode = *inode_p;
    cpfs_unlock_ino( fs, ino );
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
cpfs_alloc_block_4_file( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys )
{
    cpfs_assert( phys != 0 );

    // Read inode first
    struct cpfs_inode *ip = cpfs_lock_ino( fs, ino );
    if( ip == 0 )
    {
        cpfs_unlock_ino( fs, ino );
        return EIO;
    }

    if( logical < CPFS_INO_DIR_BLOCKS )
    {
        if( ip->blocks0[logical] )
        {
            *phys = ip->blocks0[logical];
            cpfs_unlock_ino( fs, ino );
            return EEXIST;
        }

        cpfs_blkno_t new = cpfs_alloc_disk_block( fs );
        if( !new )
        {
            cpfs_unlock_ino( fs, ino );
            return ENOSPC;
        }

        *phys = ip->blocks0[logical] = new;
        cpfs_touch_ino( fs, ino );
        cpfs_unlock_ino( fs, ino );
        return 0;
    }

    cpfs_unlock_ino( fs, ino );
    // TODO write indirect blocks support!
    return E2BIG;

}

errno_t
cpfs_find_or_alloc_block_4_file( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys )
{
    errno_t rc = cpfs_alloc_block_4_file( fs, ino, logical, phys );
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

static errno_t find_or_create_indirect( cpfs_fs_t *fs, cpfs_blkno_t *base, cpfs_blkno_t displ, int indirection, cpfs_blkno_t *phys, int create, int *created )
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
        iblk = cpfs_alloc_disk_block( fs );
        if( !iblk )
            return ENOSPC;

        *base = iblk;
        *created = 1;
        icreated = 1;
    }

    // Now we have indirect page
    struct cpfs_indir * ib = cpfs_lock_blk( fs, iblk );
    if( ib == 0 )
    {
        cpfs_unlock_blk( fs, iblk );
        return EIO;
    }

    if( icreated )
    {
        memset( ib, 0, sizeof( *ib ) );
        ib->ib_magic = CPFS_IB_MAGIC;
        cpfs_touch_blk( fs, iblk );
    }

    if( indirection == 1 )
    {
        // just process
        cpfs_assert( displ < INDIR_CNT );

        if( ib->child[displ] != 0 )
        {
            *phys = ib->child[displ];
            cpfs_unlock_blk( fs, iblk );
            return 0;
        }

        cpfs_unlock_blk( fs, iblk );

        if( !create )
        {
            return ENOENT;
        }

        // Allocate indirect page
        fblk = cpfs_alloc_disk_block( fs );
        if( !fblk )
            return ENOSPC;

        ib = cpfs_lock_blk( fs, iblk );
        if( ib == 0 )
        {
            cpfs_unlock_blk( fs, iblk );
            cpfs_free_disk_block( fs, fblk );
            return EIO;
        }

        ib->child[displ] = fblk;
        cpfs_touch_blk( fs, iblk );
        cpfs_unlock_blk( fs, iblk );
        *phys = ib->child[displ];
        return 0;
    }




    return EINVAL;
}



//
// Disk blk num containing inode
//


errno_t
cpfs_block_4_inode( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t *oblk )
{
    if( ino >= fs->sb.ninode )
    {
        cpfs_log_error("cpfs_block_4_inode: ino (%d) >= fs->sb.ninode (%d)", ino, fs->sb.ninode );
        return E2BIG;
    }

    cpfs_blkno_t blk =  fs->sb.itable_pos + (ino/CPFS_INO_PER_BLK);
    //if( blk >= fs->sb.itable_end ) return E2BIG;

    *oblk = blk;
    return 0;
}



//
// Alloc/free inode
//


errno_t
cpfs_alloc_inode( cpfs_fs_t *fs, cpfs_ino_t *inode )
{
    cpfs_mutex_lock( fs->fic_mutex );

    if( fs->fic_used <= 0 )
        fic_refill( fs );

    if( fs->fic_used <= 0 )
    {
        cpfs_mutex_unlock( fs->fic_mutex );
        return EMFILE;
    }

    *inode = fs->free_inodes_cache[--fs->fic_used];
    cpfs_mutex_unlock( fs->fic_mutex );
    return 0;
}


errno_t
cpfs_free_inode( cpfs_fs_t *fs, cpfs_ino_t ino ) // deletes file
{
    errno_t rc;
    // TODO check that inode is not used right now!

    rc = cpfs_inode_truncate( fs, ino ); // free all data blocks for inode, set size to 0
    if( rc ) return rc;

    struct cpfs_inode *inode_p = cpfs_lock_ino( fs, ino );

    if( !inode_p )
        return EIO;

    if( inode_p->nlinks <= 0 )
    {
        cpfs_log_error("free_inode: attempt to free inode with 0 links, %d", inode_p->nlinks );
        cpfs_unlock_ino( fs, ino );
        return ENOENT;
    }

    inode_p->nlinks--;

    cpfs_unlock_ino( fs, ino );


    // Put inode num to in-memory inode free list
    cpfs_mutex_lock( fs->fic_mutex );

    if( fs->fic_used < FIC_SZ )
    {
        fs->free_inodes_cache[fs->fic_used++] = ino;
        //cpfs_mutex_unlock( fs->fic_mutex );
        //return 0;
    }

    cpfs_mutex_unlock( fs->fic_mutex );
    return 0;
}


//static cpfs_blkno_t last_ino_blk = -1;

void
fic_refill( cpfs_fs_t *fs )
{
    int iblocks = fs->sb.ninode*CPFS_INO_PER_BLK;
    cpfs_blkno_t ilast = fs->sb.itable_pos + iblocks;

    cpfs_mutex_lock( fs->fic_mutex );

    //
    // fast way - alloc from end of inode space
    //

    if( fs->last_ino_blk == -1 )
    {
        //cpfs_sb_lock();
        fs->last_ino_blk = fs->sb.itable_end;
    }

    // Each disk block has CPFS_INO_PER_BLK inodes. As long as we have place for CPFS_INO_PER_BLK inodes in
    // free_inodes_cache and there are unused blocks in inode space, refill free inode numbers list.

    // NB! Inode table growth code assumes we eat inode space block by block

    //while( (fs->last_ino_blk < ilast) && (fic_used < (FIC_SZ+CPFS_INO_PER_BLK))  )
    if( (fs->last_ino_blk < ilast) && (fs->fic_used < (FIC_SZ+CPFS_INO_PER_BLK))  )
    {
        int i;
        for( i = 0; i < CPFS_INO_PER_BLK; i++ )
        {
            fs->free_inodes_cache[fs->fic_used++] = (fs->last_ino_blk-fs->sb.itable_pos) + i;
        }

        fs->last_ino_blk++;
    }

    cpfs_mutex_unlock( fs->fic_mutex );

    if( fs->fic_used > 0 )
        return;

    // TODO long way - scan through inodes? use map?
}


errno_t
cpfs_inode_truncate( cpfs_fs_t *fs, cpfs_ino_t ino ) // free all data blocks for inode, set size to 0
{
    // Free all data blocks for inode

    struct cpfs_inode copy;

    struct cpfs_inode *inode =  cpfs_lock_ino( fs, ino );
    cpfs_touch_ino( fs, ino );

    copy = *inode;

    if( inode->nlinks <= 0 )
    {
        cpfs_log_error("cpfs_inode_truncate: attempt to truncate inode with 0 links, %d", inode->nlinks );
        return ENOENT;
    }

    memset( inode->blocks0, 0, sizeof(inode->blocks0) );
    inode->blocks1 = 0;
    inode->blocks2 = 0;
    inode->blocks3 = 0;

    cpfs_blkno_t nblk = inode->fsize / CPFS_BLKSIZE;
    if( inode->fsize % CPFS_BLKSIZE ) nblk++;

    cpfs_unlock_ino( fs, ino );

    // done with inode, now free disk blocks

    cpfs_blkno_t blk;
    cpfs_blkno_t phys_blk;

    for( blk = 0; (blk < nblk) && (blk < CPFS_INO_DIR_BLOCKS); blk++ )
    {
        phys_blk = inode->blocks0[blk];
        cpfs_free_disk_block( fs, blk );
    }

    // TODO free indirect blocks

    return 0;
}



errno_t
cpfs_fsize( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_size_t *size )
{
    // asser size

    // read in inode to find out file len

    //struct cpfs_inode inode;
    struct cpfs_inode *inode_p = cpfs_lock_ino( fs, ino );

    if( inode_p ) *size = inode_p->fsize;

    cpfs_unlock_ino( fs, ino );

    if( inode_p == 0 ) return EIO;
    return 0;
}


errno_t
cpfs_inode_update_fsize( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_size_t size )
{
    struct cpfs_inode *inode_p = cpfs_lock_ino( fs, ino );

    if( inode_p == 0 )
    {
        cpfs_unlock_ino( fs, ino ); // TODO check if we need to unlock anything if lock failed
        return EIO;
    }

    if( size > inode_p->fsize )
    {
        inode_p->fsize = size;
        cpfs_touch_ino( fs, ino );
    }

    cpfs_unlock_ino( fs, ino );

    return 0;

}



/**
 *
 * Fill inode structure with reasonable default values. Assume it is created.
 *
**/
void
cpfs_inode_init_defautls( cpfs_fs_t *fs, struct cpfs_inode *ii )
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

