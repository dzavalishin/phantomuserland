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


#define INDIR 1


static errno_t find_or_create_indirect( cpfs_fs_t *fs, cpfs_blkno_t *base, cpfs_blkno_t *indexes, int indirection, cpfs_blkno_t *phys, int create, int *created );
static void do_fic_refill( cpfs_fs_t *fs );




// ----------------------------------------------------------------------------
//
// Find physical (on-disk) block address for logical 9sequential) file block
// number.
//
// ----------------------------------------------------------------------------


// maps logical blocks to physical, block must be allocated
errno_t
cpfs_find_block_4_file( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys )
{
    errno_t rc = 0;

    cpfs_assert( phys != 0 );

/*
    if( TRACE ) trace(1, "%*s > cpfs_find_block_4_file. ino=%lld, logical=%lld\n", TRACE, " ", ino, logical); 
*/
    // Read inode first
    struct cpfs_inode inode;
    struct cpfs_inode *inode_p = cpfs_lock_ino( fs, ino );
    if( inode_p ) inode = *inode_p;
    cpfs_unlock_ino( fs, ino );
    
    if( inode_p == 0 ) return EIO;

    if( logical*CPFS_BLKSIZE > inode.fsize )
    {
        cpfs_log_error("cpfs_find_block_4_file: logical*CPFS_BLKSIZE (%d) >= inode.fsize (%d)", logical*CPFS_BLKSIZE, inode.fsize );
        return E2BIG; // TODO err?
    }

    if( logical < CPFS_INO_DIR_BLOCKS )
    {
        *phys = inode.blocks0[logical];
/*
        if( TRACE ) trace(0, "%*s < cpfs_find_block_4_file.  ino=%lld, logical=%lld, phys=%d\n", TRACE-TRACE_TAB, " ", ino, logical, (int )*phys );
*/
        return 0;
    }
#if INDIR
    else
    {
        errno_t 	rc;
        cpfs_blkno_t 	indexes[CPFS_MAX_INDIR] = { 0, 0, 0, 0 };
        int 		start_index = CPFS_MAX_INDIR;

        rc = calc_indirect_positions( fs, indexes, &start_index, logical );
        if( rc ) goto fail;

        cpfs_assert( start_index != CPFS_MAX_INDIR );

        int indirection = CPFS_MAX_INDIR - start_index;
        //printf("cpfs_find_block_4_file calc_indir( %lld ) -> indirection=%d, start_index=%d, indexes = %lld, %lld, %lld, %lld\n", logical, indirection, start_index, indexes[0], indexes[1], indexes[2], indexes[3] );

        int created = 0;

        rc = find_or_create_indirect( fs, inode.indir+indirection-1, indexes+start_index, indirection, phys, 0, &created );
        if( rc ) goto fail;

        //if( created )
        //    cpfs_touch_ino( fs, ino );
        cpfs_assert( !created );
        //cpfs_unlock_ino( fs, ino );
        return 0;
    }
#endif // INDIR

    rc = E2BIG;
#if INDIR
fail:
#endif // INDIR
    cpfs_log_error("cpfs_find_block_4_file: no indirect support");
    // TODO write indirect blocks support!
    return rc;
}




// ----------------------------------------------------------------------------
//
// Allocate data block for file.
//
// ----------------------------------------------------------------------------


// allocates logical block, returns physical blk pos, block must NOT be allocated
// actually if block is allocated returns eexist and blk num in phys
errno_t
cpfs_alloc_block_4_file( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys )
{
    errno_t 	rc;

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

        cpfs_assert( new != 0 );

        *phys = ip->blocks0[logical] = new;
        cpfs_touch_ino( fs, ino );
        cpfs_unlock_ino( fs, ino );
        return 0;
    }
#if INDIR
    else
    {
        cpfs_blkno_t 	indexes[CPFS_MAX_INDIR] = { 0, 0, 0, 0 };
        int 		start_index = CPFS_MAX_INDIR;

        rc = calc_indirect_positions( fs, indexes, &start_index, logical );
        if( rc ) goto fail;

        cpfs_assert( start_index != CPFS_MAX_INDIR );

        //printf("cpfs_alloc_block_4_file calc_indir( %lld ) -> start_index=%d, indexes = %lld, %lld, %lld, %lld\n", logical, start_index, indexes[0], indexes[1], indexes[2], indexes[3] );
        int indirection = CPFS_MAX_INDIR - start_index;
        //printf("cpfs_find_block_4_file calc_indir( %lld ) -> indirection=%d, start_index=%d, indexes = %lld, %lld, %lld, %lld\n", logical, indirection, start_index, indexes[0], indexes[1], indexes[2], indexes[3] );

        int created;

        rc = find_or_create_indirect( fs, ip->indir+indirection-1, indexes+start_index, indirection, phys, 1, &created );
        if( rc ) goto fail;

        if( created )
            cpfs_touch_ino( fs, ino );
        cpfs_unlock_ino( fs, ino );
        return 0;

    }
#endif // INDIR

    rc = E2BIG;
#if INDIR
fail:
#endif // INDIR
    cpfs_unlock_ino( fs, ino );
    // TODO write indirect blocks support!
    cpfs_log_error("cpfs_alloc_block_4_file: no indirect support, logical = %lld", logical );
    return rc;

}

errno_t
cpfs_find_or_alloc_block_4_file( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t logical, cpfs_blkno_t *phys )
{
    errno_t rc = cpfs_alloc_block_4_file( fs, ino, logical, phys );
    if( rc == EEXIST ) rc = 0;
    return rc;
}

#if INDIR


//
// Indirect blocks support 
//
// base contains 0 or block num of indirect block. If it is 0 and create is nonzero, block will be created.
// indexes contains logical block numbers for corresponding indirection
// indirection is level of indirection. If 1, we can't go any further, *base contains page with phys block numbers - TODO right?
// create - instructs us to allocate block if unallocated
// created set to nonzero if we did create *base
//

static errno_t find_or_create_indirect( cpfs_fs_t *fs, cpfs_blkno_t *base, cpfs_blkno_t *indexes, int indirection, cpfs_blkno_t *phys, int create, int *created )
{
    errno_t rc;

    int icreated = 0;

    cpfs_assert( base != 0 );
    cpfs_assert( indexes != 0 );
    cpfs_assert( phys != 0 );
    cpfs_assert( created != 0 );

    // we're empty and can't create, fail
    if( (*base == 0) && !create )
        return ENOENT;

    cpfs_blkno_t iblk = *base; // indirect block
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
        ib->h.magic = CPFS_IB_MAGIC;
        cpfs_touch_blk( fs, iblk );
    }





    if( indirection == 1 )
    {
        // just process
        cpfs_assert( *indexes < CPFS_INDIRECT_PER_BLK );

        // Have?
        if( ib->child[*indexes] != 0 )
        {
            *phys = ib->child[*indexes];
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

        ib->child[*indexes] = fblk;
        cpfs_touch_blk( fs, iblk );
        cpfs_unlock_blk( fs, iblk );
        *phys = ib->child[*indexes];

        return 0;
    }

    // Call me recursively for the next level of indirection

    rc = find_or_create_indirect( fs, &(ib->child[*indexes]), indexes + 1, indirection - 1, phys, create, created );

    if( (!rc) && (*created) )
        cpfs_touch_blk( fs, iblk );

    cpfs_unlock_blk( fs, iblk );

    return rc;
}


#endif // INDIR


//
// Disk blk num containing inode
//


errno_t
cpfs_block_4_inode( cpfs_fs_t *fs, cpfs_ino_t ino, cpfs_blkno_t *oblk )
{
    if( ino >= fs->sb.ninode )
    {
        cpfs_log_error("cpfs_block_4_inode: ino (%lld) >= fs->sb.ninode (%lld)", ino, fs->sb.ninode );
        return E2BIG;
    }

    cpfs_blkno_t blk =  fs->sb.itable_pos + (ino/CPFS_INO_PER_BLK);
    //if( blk >= fs->sb.itable_end ) return E2BIG;

    *oblk = blk;
/*
    if( TRACE ) trace(0, "%*s inode=%lld -> phblock=%llu \n", TRACE-TRACE_TAB, " ", ino, blk);   
*/
    return 0;
}



//
// Alloc/free inode
//

// ----------------------------------------------------------------------------
//
// Allocate inode
//
// ----------------------------------------------------------------------------



errno_t
cpfs_alloc_inode( cpfs_fs_t *fs, cpfs_ino_t *inode )
{
    int locked = 0;
    if( fs->fic_used <= 0 ) {
        do_fic_refill(fs); // locks fic_mutex
        locked = 1;
    }

    if( fs->fic_used <= 0 ) {
        cpfs_mutex_unlock(fs->fic_mutex);
        locked = 0;
        return ENOSPC; //EMFILE;
    }

    if( !locked ) {
        cpfs_mutex_lock(fs->fic_mutex);
        locked = 1;
    }

    *inode = fs->free_inodes_cache[--fs->fic_used];

    //printf("alloc inode %lld\n", *inode);
    
    {
        struct cpfs_inode *rdi = cpfs_lock_ino( fs, *inode );
        cpfs_touch_ino( fs, *inode );
        cpfs_inode_init_defautls( fs, rdi );
        rdi->nlinks = 1;
        cpfs_unlock_ino( fs, *inode );
    }

    cpfs_mutex_unlock( fs->fic_mutex );
    return 0;
}






// ----------------------------------------------------------------------------
//
// Free inode (actually delete file data)
//
// ----------------------------------------------------------------------------

errno_t
cpfs_free_inode( cpfs_fs_t *fs, cpfs_ino_t ino )
{
    errno_t rc;
    struct cpfs_inode *inode_p;

    cpfs_assert( ino != 0 );
    //printf("free inode %lld\n", ino);

    // TODO test case for this
    int used = cpfs_fdmap_is_inode_used( fs, ino );
    if( used )
    {
        cpfs_log_error("free_inode: attempt to free inode %d used in fdmap", ino );
        return EWOULDBLOCK;
    }

    // Check if inode is active

    inode_p = cpfs_lock_ino( fs, ino );

    if( !inode_p )
        return EIO;

    if( inode_p->nlinks <= 0 )
    {
        cpfs_log_error("free_inode: attempt to free inode %d with zero links", ino );
        cpfs_unlock_ino( fs, ino );
        return ENOENT;
    }
    //printf("inode %lld links %d\n", ino, inode_p->nlinks );
    cpfs_unlock_ino( fs, ino );


    // Free inode data

    rc = cpfs_inode_truncate( fs, ino ); // free all data blocks for inode, set size to 0
    if( rc ) return rc;


    // Free inode record

    inode_p = cpfs_lock_ino( fs, ino );

    if( !inode_p )
        return EIO;

    if( inode_p->nlinks <= 0 )
    {
        cpfs_log_error("free_inode: attempt to free inode %d with zero links", ino );
        cpfs_unlock_ino( fs, ino );
        return ENOENT;
    }

    inode_p->nlinks--;

    cpfs_touch_ino( fs, ino );
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







// ----------------------------------------------------------------------------
//
// Refill free inode cache structure - TODO incomplete, scan for free inodes
//
// ----------------------------------------------------------------------------



// TODO mutex!
// NB! Leaves fic_mutex locked so that caller can be sure nobody stole freed inode

static void
do_fic_refill( cpfs_fs_t *fs )
{
    int iblocks = fs->sb.ninode / CPFS_INO_PER_BLK;
    cpfs_blkno_t ilast = fs->sb.itable_pos + iblocks;

    cpfs_mutex_lock( fs->fic_mutex );

    //
    // fast way - alloc from end of inode space
    //

    if( ((long long int)fs->last_ino_blk) == -1 )
    {
        //cpfs_sb_lock();
        fs->last_ino_blk = fs->sb.itable_end;
    }

    // Each disk block has CPFS_INO_PER_BLK inodes. As long as we have place for CPFS_INO_PER_BLK inodes in
    // free_inodes_cache and there are unused blocks in inode space, refill free inode numbers list.

    // NB! Inode table growth code assumes we eat inode space block by block

    if( (fs->last_ino_blk < ilast) && (fs->fic_used < (FIC_SZ+CPFS_INO_PER_BLK))  )
    {
        int i;
        for( i = 0; i < CPFS_INO_PER_BLK; i++ )
        {
            cpfs_ino_t ino = (fs->last_ino_blk - fs->sb.itable_pos) * CPFS_INO_PER_BLK + i;

            cpfs_assert( ino < fs->sb.ninode );

            //printf("refill inodes add %lld\n", ino);
            //fs->free_inodes_cache[fs->fic_used++] = (fs->last_ino_blk-fs->sb.itable_pos) + i;
            fs->free_inodes_cache[fs->fic_used++] = ino;
        }

        fs->last_ino_blk++;
    }

    //cpfs_mutex_unlock( fs->fic_mutex );

    if( fs->fic_used > 0 )
        return;

    // TODO long way - scan through inodes? use map?
}


void
fic_refill( cpfs_fs_t *fs )
{
    do_fic_refill( fs );
    cpfs_mutex_unlock( fs->fic_mutex );
}



// ----------------------------------------------------------------------------
//
// Free all data blocks for inode (delete file data)
//
// ----------------------------------------------------------------------------


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
        cpfs_log_error("cpfs_inode_truncate: attempt to truncate inode %d with zero links", ino );
        return ENOENT;
    }

    memset( inode->blocks0, 0, sizeof(inode->blocks0) );
    memset( inode->indir, 0, sizeof(inode->indir) );
    //inode->blocks1 = 0;
    //inode->blocks2 = 0;
    //inode->blocks3 = 0;

    cpfs_blkno_t nblk = inode->fsize / CPFS_BLKSIZE;
    if( inode->fsize % CPFS_BLKSIZE ) nblk++;

    cpfs_unlock_ino( fs, ino );

    // done with inode, now free disk blocks

    // NB! Can't use inode below, just copy!

    cpfs_blkno_t blk;
    cpfs_blkno_t phys_blk;

    for( blk = 0; (blk < nblk) && (blk < CPFS_INO_DIR_BLOCKS); blk++ )
    {
        phys_blk = copy.blocks0[blk];

        if( phys_blk == 0 )
        {
            cpfs_log_error("cpfs_inode_truncate: attempt to free blk 0 (%lld block of file)", phys_blk, blk );
            continue;
        }

        cpfs_free_disk_block( fs, phys_blk );
    }


    errno_t rc;
    int ilev;

    for( ilev = 0; ilev < CPFS_MAX_INDIR-1; ilev++ )
    {
        rc = cpfs_free_indirect( fs, copy.indir[ilev], ilev+1 );
        if( rc )
            cpfs_log_error("cpfs_inode_truncate: fail to free indirect blocks lev %d for ino %lld", ilev+1, ino );
    }

    return 0;
}



// ----------------------------------------------------------------------------
//
// Return file size
//
// ----------------------------------------------------------------------------




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


// ----------------------------------------------------------------------------
//
// Update (grow) file size field in inode
//
// ----------------------------------------------------------------------------




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
    (void) fs;

    ii->fsize = 0;
    ii->nlinks = 1;
    ii->ftype = 0;

    ii->acl = 0; // unused now
    ii->log = 0; // unused now

    memset( ii->blocks0, 0, sizeof(ii->blocks0) );
    memset( ii->indir, 0, sizeof(ii->indir) );
    //ii->blocks1 = 0;
    //ii->blocks2 = 0;
    //ii->blocks3 = 0;

    ii->ctime = cpfs_get_current_time();
    ii->atime = ii->ctime;
    ii->mtime = ii->ctime;
    ii->vtime = 0;
}



// ----------------------------------------------------------------------------
//
// Return inode data (file info)
//
// ----------------------------------------------------------------------------




errno_t cpfs_file_stat( struct cpfs_fs *fs, const char *name, void * user_id_data, struct cpfs_stat *stat )
{
    const char *last;
    cpfs_ino_t last_dir_ino;
    errno_t rc;

    rc = cpfs_os_access_rights_check( fs, cpfs_r_stat, user_id_data, name );
    if( rc ) return rc;

    cpfs_assert( stat != 0 );

    rc = cpfs_descend_dir( fs, name, &last, &last_dir_ino );
    if( rc ) return rc;

    cpfs_ino_t ret;
    rc = cpfs_namei( fs, last_dir_ino, last, &ret );


    struct cpfs_inode *ip = cpfs_lock_ino( fs, ret );

    if( ip == 0 )
    {
        cpfs_unlock_ino( fs, ret );
        return EIO;
    }

    stat->fsize         = ip->fsize;
    stat->ftype         = ip->ftype;
    stat->nlinks        = ip->nlinks;

    stat->ctime         = ip->ctime;
    stat->atime         = ip->atime;
    stat->mtime         = ip->mtime;

    cpfs_unlock_ino( fs, ret );

    return 0;
}

errno_t cpfs_fd_stat( int file_id, struct cpfs_stat *stat )
{
    cpfs_ino_t ino;
    cpfs_fs_t *fs;

    errno_t rc = cpfs_fdmap_get( file_id, &ino, &fs );
    if( rc ) return rc;

    cpfs_assert( stat != 0 );

    struct cpfs_inode *ip = cpfs_lock_ino( fs, ino );

    if( ip == 0 )
    {
        cpfs_unlock_ino( fs, ino );
        return EIO;
    }

    stat->fsize         = ip->fsize;
    stat->ftype         = ip->ftype;
    stat->nlinks        = ip->nlinks;

    stat->ctime         = ip->ctime;
    stat->atime         = ip->atime;
    stat->mtime         = ip->mtime;

    cpfs_unlock_ino( fs, ino );

    return 0;
}

// TODO merge same code - stat by inode


// ----------------------------------------------------------------------------
//
// Lock/unlock inode mutex - have exclusive access to inode state or data
//
// ----------------------------------------------------------------------------

#if CPFS_INODE_MUTEX

void cpfs_ino_mutex_lock( struct cpfs_fs *fs, struct cpfs_inode *ip )
{
    (void) fs;

    cpfs_mutex_unlock( ip->mutex );
}


void cpfs_ino_mutex_unlock( struct cpfs_fs *fs, struct cpfs_inode *ip )
{
    (void) fs;

    cpfs_mutex_unlock( ip->mutex );
}

#endif // CPFS_INODE_MUTEX


