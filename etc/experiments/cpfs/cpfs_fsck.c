/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * File System Check.
 *
**/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>

#include <stdarg.h>

#include "cpfs.h"
#include "cpfs_local.h"


typedef enum { msg, warn, err } severity_t;

static void fslog( cpfs_fs_t *fs, severity_t severity, const char *fmt, ... );

static errno_t fsck_sb( cpfs_fs_t *fs, int fix );
static void fsck_scan_dirs( cpfs_fs_t *fs );




errno_t
cpfs_fsck( cpfs_fs_t *fs, int fix )
{
    errno_t rc;

    cpfs_assert( fs->disk_size < INT_MAX ); // TODO document

    fs->fsck_nWarn = 0;
    fs->fsck_nErr = 0;
    fs->fsck_rebuild_free = 0;

    fs->fsck_blk_state = calloc( sizeof( fsck_blkstate_t ), fs->disk_size );
    if( 0 == fs->fsck_blk_state ) return ENOMEM;

    rc = fsck_sb( fs, fix );
    if( rc ) goto error;

    fsck_scan_dirs( fs );

    printf("FSCK done, %d warnings, %d errors\n", fs->fsck_nWarn, fs->fsck_nErr);

    return (fs->fsck_nErr > 0) ? EINVAL : 0;

error:
    cpfs_log_error( "FSCK error: %d", rc );
    return rc;
}

// TODO fsck is not finished

static errno_t
fsck_sb( cpfs_fs_t *fs, int fix )
{
    //errno_t rc;

    printf("FSCK - ckeck superblock\n");

    struct cpfs_sb      *sb = cpfs_lock_blk( fs, 0 );
    if( sb == 0 ) return EIO;



    if( sb->h.magic != CPFS_SB_MAGIC )
    {
        fslog( fs, err, "SB magic 0 wrong, %x", sb->h.magic );
    }


    if( sb->ninode < 1024 )
    {
        fslog( fs, err, "SB ninode too small %d", sb->ninode );
    }

    if( sb->itable_pos != 1 )
    {
        fslog( fs, err, "SB itable_pos != 1, %lld", sb->itable_pos );
    }

    if( sb->itable_end < sb->itable_pos )
    {
        fslog( fs, err, "SB itable_end < itable_pos, %lld", sb->itable_end );
    }

    int ino_table_blkno = CPFS_INO_PER_BLK * sb->ninode;

    if( sb->itable_end >= sb->itable_pos+ino_table_blkno )
    {
        fslog( fs, err, "SB sb->itable_end >= sb->itable_pos+ino_table_blkno, %lld", sb->itable_end );
    }

    if( sb->disk_size != fs->disk_size )
    {
        fslog( fs, err, "SB disk_size != fs->disk_size, %lld", sb->disk_size );
    }


    /*
    sb->first_unallocated = ino_table_blkno+sb->itable_pos;
    sb->free_list = 0;

    sb->free_count = disk_size;
    sb->free_count -= 1; // sb
    sb->free_count -= ino_table_blkno;

    // sanity check
    if( sb->free_count != disk_size - sb->first_unallocated )
        cpfs_panic("sb->free_count (%lld) != disk_size (%lld) - sb->first_unallocated (%lld)", (long long)sb->free_count, (long long)disk_size, (long long)sb->first_unallocated);

    if( sb->first_unallocated >= disk_size )
    {
        cpfs_unlock_blk( fs, sb_blk );
        return EINVAL;
    }


    */
    //cpfs_touch_blk( fs, sb_blk ); // marks block as dirty, will be saved to disk on unlock
    cpfs_unlock_blk( fs, 0 );
    /*
    // temp init global superblock copy for lock_ino to work
    fs->sb = *sb;


    cpfs_ino_t root_dir = 0;

    // Init inode 0 as root dir

    struct cpfs_inode *rdi = cpfs_lock_ino( fs, root_dir );
    cpfs_touch_ino( fs, root_dir );
    cpfs_inode_init_defautls( fs, rdi );
    rdi->ftype = CPFS_FTYPE_DIR;
    rdi->nlinks = 1;
    cpfs_unlock_ino( fs, root_dir );

    // de-init!
    memset( &fs->sb, 0, sizeof( fs->sb ) );
    */





    return 0;

}






void fslog( cpfs_fs_t *fs, severity_t severity, const char *fmt, ... )
{
    switch( severity )
    {
    case msg:
        printf( "FSCK Info: ");
        break;

    case warn:
        printf( "FSCK Warn: ");
        fs->fsck_nWarn++;
        break;

    case err:
        printf( "FSCK Err:  ");
        fs->fsck_nErr++;
        break;
    }

    va_list ptr;
    va_start(ptr, fmt);
    vprintf(fmt, ptr);
    va_end(ptr);

    printf( "\n");
}




// -----------------------------------------------------------------------
//
// Scan dir tree
//
// -----------------------------------------------------------------------
#define FSCK_VERBOSE 0

static errno_t fsck_scan_dir( cpfs_fs_t *fs, cpfs_ino_t dir, size_t depth );

static dir_scan_ret_t de_subscan( cpfs_fs_t *fs, struct cpfs_dir_entry *de, void *farg )
{
    errno_t rc;
    size_t depth = (size_t)farg;

    if( de->inode == 0 )    return dir_scan_continue;

    //if( 0 == strcmp( (const char*)farg, de->name ) )        return dir_scan_error;

    if(FSCK_VERBOSE)
    {
        int i;
        for( i = depth; i; i-- )
            printf("\t");
        printf("%s", de->name);
    }

    int isdir = 0;
    rc = cpfs_is_dir( fs, de->inode, &isdir );

    if( isdir )
    {
        if(FSCK_VERBOSE) printf("/\n");
        rc = fsck_scan_dir( fs, de->inode, depth+1 );
        if( rc ) return dir_scan_error;
    }
    else
    {
        if(FSCK_VERBOSE) printf("\n");
    }


    return dir_scan_continue;
}



errno_t
fsck_scan_dir( cpfs_fs_t *fs, cpfs_ino_t dir, size_t depth )
{
    errno_t rc = cpfs_scan_dir( fs, dir, de_subscan, (void *)depth );
    return rc;
}

static void fsck_scan_dirs( cpfs_fs_t *fs )
{
    printf("fsck scan dirs\n");
    fsck_scan_dir( fs, 0, 0 );
}



// -----------------------------------------------------------------------
//
// Update/check block map
//
// -----------------------------------------------------------------------


errno_t fsck_update_block_map( cpfs_fs_t *fs, cpfs_blkno_t blk, fsck_blkstate_t state )
{
    cpfs_assert( blk < fs->disk_size ); // TODO or superblock disk size? Or shall we check sb before?

    if( fs->fsck_blk_state[blk] != bs_unknown )
    {
        if( (state == bs_freelist) || (state == bs_freemap) )
        {
            fslog( fs, err, "block %lld state was %d, attempt to set free\n", blk, fs->fsck_blk_state[blk] );

            fs->fsck_blk_state[blk] = state;
            fs->fsck_rebuild_free = 1;
            return EBUSY;
        }
        else
        if( (fs->fsck_blk_state[blk] == bs_freelist) || (fs->fsck_blk_state[blk] == bs_freemap) )
        {
            fslog( fs, err, "block %lld state was free, attempt to set %d\n", blk, state );

            fs->fsck_blk_state[blk] = state;
            fs->fsck_rebuild_free = 1;
            return EBUSY;
        }
        else
        {
            fslog( fs, err, "block %lld state %d, attempt to set %d\n", blk, fs->fsck_blk_state[blk], state );
            // TODO and what?
            return EBUSY;
        }
    }

    fs->fsck_blk_state[blk] = state;
    return 0;
}


/**
 *
 * Scan through map of disk block states, check that state is correct.
 *
**/

errno_t fsck_check_block_map( cpfs_fs_t *fs )
{
    cpfs_blkno_t blk;

    // no reason to check :)
    //if( blk[0] ==

    int iblocks = fs->sb.ninode / CPFS_INO_PER_BLK;
    cpfs_blkno_t ilast = fs->sb.itable_pos + iblocks;

    fsck_blkstate_t *bs = fs->fsck_blk_state;

    for( blk = 1; blk < fs->disk_size; blk++ )
    {
        if( blk < ilast )
        {
            if( (bs[blk] != bs_inode) && (bs[blk] != bs_unknown) )
                fslog( fs, err, "blk %lld is not inode (%d)", blk, bs[blk] );

        }
        else
        {

        }

    }

    return 0;
}

