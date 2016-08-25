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
//#include <unistd.h>
#include <sys/types.h>

#include <stdarg.h>

#include "cpfs.h"
#include "cpfs_local.h"


typedef enum { msg, warn, err } severity_t;


static void fslog( severity_t severity, const char *fmt, ... );

static errno_t fsck_sb( cpfs_fs_t *fs, int fix );
static void fsck_scan_dirs( cpfs_fs_t *fs );


static int nWarn = 0;
static int nErr = 0;




errno_t
cpfs_fsck( cpfs_fs_t *fs, int fix )
{
    errno_t rc;

    rc = fsck_sb( fs, fix );
    if( rc ) goto error;

    fsck_scan_dirs( fs );

    printf("FSCK done, %d warnings, %d errors\n", nWarn, nErr);

    return (nErr > 0) ? EINVAL : 0;

error:
    cpfs_log_error( "FSCK error: %d", rc );
    return rc;
}

// TODO fsck is not finished

static errno_t
fsck_sb( cpfs_fs_t *fs, int fix )
{
    errno_t rc;

    printf("FSCK - ckeck superblock\n");

    struct cpfs_sb      *sb = cpfs_lock_blk( fs, 0 );
    if( sb == 0 ) return EIO;



    if( sb->sb_magic_0 != CPFS_SB_MAGIC )
    {
        fslog( err, "SB magic 0 wrong, %x", sb->sb_magic_0 );
    }


    if( sb->ninode < 1024 )
    {
        fslog( err, "SB ninode too small %d", sb->ninode );
    }

    if( sb->itable_pos != 1 )
    {
        fslog( err, "SB itable_pos != 1, %lld", sb->itable_pos );
    }

    if( sb->itable_end < sb->itable_pos )
    {
        fslog( err, "SB itable_end < itable_pos, %lld", sb->itable_end );
    }

    int ino_table_blkno = CPFS_INO_PER_BLK * sb->ninode;

    if( sb->itable_end >= sb->itable_pos+ino_table_blkno )
    {
        fslog( err, "SB sb->itable_end >= sb->itable_pos+ino_table_blkno, %lld", sb->itable_end );
    }

    if( sb->disk_size != fs->disk_size )
    {
        fslog( err, "SB disk_size != fs->disk_size, %lld", sb->disk_size );
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



    // TODO scan all dirs, find all used inodes, check that actual inodes are used/unused correspondingly


    return 0;

}






void fslog( severity_t severity, const char *fmt, ... )
{
    switch( severity )
    {
    case msg:
        printf( "FSCK Info: ");
        break;

    case warn:
        printf( "FSCK Warn: ");
        nWarn++;
        break;

    case err:
        printf( "FSCK Err:  ");
        nErr++;
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
static errno_t fsck_scan_dir( cpfs_fs_t *fs, cpfs_ino_t dir, int depth );

static dir_scan_ret_t de_subscan( cpfs_fs_t *fs, struct cpfs_dir_entry *de, void *farg )
{
    errno_t rc;
    int depth = (int)farg;

    if( de->inode == 0 )    return dir_scan_continue;

    //if( 0 == strcmp( (const char*)farg, de->name ) )        return dir_scan_error;

    int i;
    for( i = depth; i; i-- ) printf("\t");
    printf("%s", de->name);

    int isdir = 0;
    rc = cpfs_is_dir( fs, de->inode, &isdir );

    if( isdir )
    {
        printf("/");
        rc = fsck_scan_dir( fs, de->inode, depth+1 );
        if( rc ) return dir_scan_error;
    }
    else
    {
    }
    printf("\n");


    return dir_scan_continue;
}



errno_t
fsck_scan_dir( cpfs_fs_t *fs, cpfs_ino_t dir, int depth )
{
    printf("fsck scan dirs\n");
    errno_t rc = cpfs_scan_dir( fs, dir, de_subscan, (void *)depth );
    return rc;
}

static void fsck_scan_dirs( cpfs_fs_t *fs )
{
    fsck_scan_dir( fs, 0, 0 );
}




