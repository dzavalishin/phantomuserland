/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Directories related code.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"



//#warning code is wrong, can't use cpfs_disk_read/write, must use cpfs_ino_file_read

static errno_t
cpfs_namei_impl( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t *file_ino, int remove )
{
    errno_t rc;
    int isdir = 0;
    // TODO some speedup? In-mem hash?

    cpfs_assert( file_ino != 0 );
    cpfs_assert( fname != 0 );


    rc = cpfs_is_dir( fs, dir_ino, &isdir );
    if( rc ) return rc;

    if( !isdir ) return ENOTDIR;

    // read in directory inode to find out dir file len

    cpfs_size_t fsize;
    rc = cpfs_fsize( fs, dir_ino, &fsize );
    if( rc )
        return rc;

    int nentry = fsize/CPFS_DIR_REC_SIZE;

    cpfs_assert( (fsize%CPFS_DIR_REC_SIZE) == 0 );

    int nblk = nentry / CPFS_DIR_PER_BLK;
    int blkpos = 0;

    // Scan sequentially through the dir looking for name

    while( nblk-- > 0 )
    {
        char data[CPFS_BLKSIZE];

        //rc = cpfs_disk_read( 0, blkpos, data );
        rc = cpfs_ino_file_read( fs, dir_ino, blkpos*CPFS_BLKSIZE, data, CPFS_BLKSIZE );

        if( rc )
        {
            cpfs_log_error("Can't read dir ino %d @ blk %d", dir_ino, blkpos );
            return rc;
        }

        int i;
        for( i = 0; i < CPFS_DIR_PER_BLK; i++ )
        {
            if( nentry-- <= 0 )
                return ENOENT;

            // NB! sizeof(struct cpfs_dir_entry) != CPFS_DIR_REC_SIZE
            struct cpfs_dir_entry *de = ((void *)data) + (i*CPFS_DIR_REC_SIZE);


            if( de->inode && (0 == strcmp( fname, de->name )) )
            {
                *file_ino = de->inode;

                if( remove )
                {
                    de->inode = 0;

                    //rc = cpfs_disk_write( 0, blkpos, data );
                    rc = cpfs_ino_file_write( fs, dir_ino, blkpos*CPFS_BLKSIZE, data, CPFS_BLKSIZE );
                    if( rc )
                    {
                        cpfs_log_error("Can't write dir ino %d @ blk %d", dir_ino, blkpos );
                        return rc;
                    }
                }

                return 0;
            }

        }

        blkpos++;

    }

    return ENOENT;
}


errno_t
cpfs_namei( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t *file_ino )
{
    errno_t rc = cpfs_update_ino_atime( fs, dir_ino ); // SLOOW
    if( rc ) return rc;

    return cpfs_namei_impl( fs, dir_ino, fname, file_ino, 0 );
}

errno_t
cpfs_free_dirent( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname )
{
    errno_t rc = cpfs_update_ino_mtime( fs, dir_ino );
    if( rc ) return rc;

    cpfs_ino_t file_ino; // dummy
    return cpfs_namei_impl( fs, dir_ino, fname, &file_ino, 1 );
}






// allocate a new dir entry in a dir

errno_t
cpfs_alloc_dirent( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t file_ino ) 
{
    errno_t rc;
    int isdir;
    char data[CPFS_BLKSIZE];

    // TODO some speedup? In-mem hash?

    cpfs_assert(fname);

    rc = cpfs_is_dir( fs, dir_ino, &isdir );
    if( rc ) return rc;

    if( !isdir ) return ENOTDIR;

    rc = cpfs_dir_has_entry( fs, dir_ino, fname );
    if( rc ) return rc;

    // read in directory inode to find out dir file len

    cpfs_size_t fsize;
    if( cpfs_fsize( fs, dir_ino, &fsize ) )
        return EIO;

    int nentry = fsize/CPFS_DIR_REC_SIZE;

    cpfs_assert( (fsize%CPFS_DIR_REC_SIZE) == 0 );

    int nblk = nentry / CPFS_DIR_PER_BLK;
    int blkpos = 0;

    // a bit early, mtime will be updated in case of io error too
    rc = cpfs_update_ino_mtime( fs, dir_ino );
    if( rc ) return rc;

    // Scan sequentially through the dir looking for name

    while( nblk-- > 0 )
    {
        //rc = cpfs_disk_read( 0, blkpos, data );
        rc = cpfs_ino_file_read( fs, dir_ino, blkpos*CPFS_BLKSIZE, data, CPFS_BLKSIZE );
        if( rc )
        {
            cpfs_log_error("Can't read dir ino %d @ blk %d", dir_ino, blkpos );
            return EIO;
        }

        int i;
        for( i = 0; i < CPFS_DIR_PER_BLK; i++ )
        {
            if( nentry-- <= 0 )
                break;

            // NB! sizeof(struct cpfs_dir_entry) != CPFS_DIR_REC_SIZE
            struct cpfs_dir_entry *de = ((void *)data) + (i*CPFS_DIR_REC_SIZE);


            if( 0 == de->inode )
            {
                de->inode = file_ino;
                strlcpy( de->name, fname, sizeof(de->name) );
                rc = cpfs_ino_file_write( fs, dir_ino, blkpos*CPFS_BLKSIZE, data, CPFS_BLKSIZE );
                if( rc )
                {
                    cpfs_log_error("Can't write dir ino %d @ blk %d", dir_ino, blkpos );
                }

                //cpfs_debug_fdump( "alloc_dirent.data", data, CPFS_BLKSIZE );
                return rc;
            }

        }

        blkpos++;
    }

    // No free entry found, attempt to append one - we append one block, following calls will find rest of blk to be free

    memset( data, 0, sizeof( data ) );
    struct cpfs_dir_entry *de = ((void *)data);

    de->inode = file_ino;
    strlcpy( de->name, fname, sizeof(de->name) );
    //return cpfs_disk_write( 0, nblk, data );
    return cpfs_ino_file_write( fs, dir_ino, blkpos*CPFS_BLKSIZE, data, CPFS_BLKSIZE );

}

// TODO return dir size in entries too
/* checks that given inode contains directory */

errno_t
cpfs_is_dir( cpfs_fs_t *fs, cpfs_ino_t dir_ino, int *yesno )
{
    struct cpfs_inode *rdi = cpfs_lock_ino( fs, dir_ino );
    if( rdi == 0 )
    {
        return EIO;
    }

    *yesno = (rdi->ftype == CPFS_FTYPE_DIR);

    cpfs_unlock_ino( fs, dir_ino );

    return 0;
}


/*
// debug

errno_t
cpfs_dump_dir( cpfs_fs_t *fs, cpfs_ino_t dir_ino )
{
    errno_t rc;
    int isdir = 0;

    cpfs_assert( file_ino != 0 );

    rc = cpfs_is_dir( fs, dir_ino, &isdir );
    if( rc ) return rc;

    if( !isdir ) return ENOTDIR;

    printf("dump dir ino %d\n", dir_ino );

    cpfs_size_t fsize;
    rc = cpfs_fsize( fs, dir_ino, &fsize );
    if( rc )
        return rc;

    int nentry = fsize/CPFS_DIR_REC_SIZE;

    cpfs_assert( (inode.fsize%CPFS_DIR_REC_SIZE) == 0 );

    int nblk = nentry / CPFS_DIR_PER_BLK;
    int blkpos = 0;

    // Scan sequentially through the dir looking for name

    while( nblk-- > 0 )
    {
        char data[CPFS_BLKSIZE];

        //rc = cpfs_disk_read( 0, blkpos, data );
        rc = cpfs_ino_file_read( fs, dir_ino, blkpos*CPFS_BLKSIZE, data, CPFS_BLKSIZE );

        if( rc )
        {
            cpfs_log_error("Can't read dir ino %d @ blk %d", dir_ino, blkpos );
            return rc;
        }

        //cpfs_debug_fdump( "dump_dirent.data", data, CPFS_BLKSIZE );

        int i;
        for( i = 0; i < CPFS_DIR_PER_BLK; i++ )
        {
            if( nentry-- <= 0 )
                return 0;

            // NB! sizeof(struct cpfs_dir_entry) != CPFS_DIR_REC_SIZE
            struct cpfs_dir_entry *de = ((void *)data) + (i*CPFS_DIR_REC_SIZE);

            //if( de->inode ) printf("%03d: '%s'\n", (int)de->inode, de->name );
            if( de->inode ) printf("%03lld: '%s'\n", de->inode, de->name );

        }

        blkpos++;

    }

    return 0;
}

*/









/**
 *
 * General directory scan function
 *
**/





errno_t
cpfs_scan_dir( cpfs_fs_t *fs, cpfs_ino_t dir_ino, dir_scan_func_t f, void *farg )
{
    errno_t rc;
    int isdir = 0;

    //cpfs_assert( file_ino != 0 );

    rc = cpfs_is_dir( fs, dir_ino, &isdir );
    if( rc ) return rc;

    if( !isdir ) return ENOTDIR;

    //printf("scan dir ino %d\n", dir_ino );

    cpfs_size_t fsize;
    rc = cpfs_fsize( fs, dir_ino, &fsize );
    if( rc )
        return rc;

    int nentry = fsize/CPFS_DIR_REC_SIZE;

    cpfs_assert( (fsize%CPFS_DIR_REC_SIZE) == 0 );

    int nblk = nentry / CPFS_DIR_PER_BLK;
    int blkpos = 0;

    // Scan sequentially through the dir looking for name

    while( nblk-- > 0 )
    {
        char data[CPFS_BLKSIZE];

        rc = cpfs_ino_file_read( fs, dir_ino, blkpos*CPFS_BLKSIZE, data, CPFS_BLKSIZE );

        if( rc )
        {
            cpfs_log_error("Can't read dir ino %d @ blk %d", dir_ino, blkpos );
            return rc;
        }

        int i;
        for( i = 0; i < CPFS_DIR_PER_BLK; i++ )
        {
            if( nentry-- <= 0 )
                return 0;

            // NB! sizeof(struct cpfs_dir_entry) != CPFS_DIR_REC_SIZE
            struct cpfs_dir_entry *de = ((void *)data) + (i*CPFS_DIR_REC_SIZE);

            //if( de->inode ) printf("%03lld: '%s'\n", de->inode, de->name );

            dir_scan_ret_t fret = f( fs, de, farg );

            if( fret == dir_scan_success ) return 0;
            if( fret == dir_scan_error ) return EMFILE;

        }

        blkpos++;

    }

    return 0;
}






/**
 *
 * Use of general directory scan function: dump dir
 *
**/



static dir_scan_ret_t dir_print( cpfs_fs_t *fs, struct cpfs_dir_entry *de, void *farg )
{
    (void) fs;
    (void) farg;

    if( de->inode ) printf("%03lld: '%s'\n", de->inode, de->name );

    return dir_scan_continue;
}



errno_t
cpfs_dump_dir( cpfs_fs_t *fs, cpfs_ino_t dir_ino )
{
    return cpfs_scan_dir( fs, dir_ino, dir_print, 0 );
}




/**
 *
 * Use of general directory scan function: is dir empty
 *
**/



static dir_scan_ret_t de_isempty( cpfs_fs_t *fs, struct cpfs_dir_entry *de, void *farg )
{
    (void) fs;
    (void) farg;

    if( de->inode ) return dir_scan_error;
    return dir_scan_continue;
}



errno_t
cpfs_is_empty_dir( cpfs_fs_t *fs, cpfs_ino_t dir_ino )
{
    errno_t rc = cpfs_scan_dir( fs, dir_ino, de_isempty, 0 );
    if( rc == EMFILE )
        return ENOTEMPTY;

    return 0;
}




/**
 *
 * Use of general directory scan function: dir has entry
 *
**/



static dir_scan_ret_t de_hasentry( cpfs_fs_t *fs, struct cpfs_dir_entry *de, void *farg )
{
    (void) fs;
    (void) farg;

    if( de->inode == 0 )    return dir_scan_continue;

    if( 0 == strcmp( (const char*)farg, de->name ) )
        return dir_scan_error;

    return dir_scan_continue;
}



errno_t
cpfs_dir_has_entry( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *name )
{
    errno_t rc = cpfs_scan_dir( fs, dir_ino, de_hasentry, (void *)name );
    if( rc == EMFILE )
        return EEXIST;

    return 0;
}













