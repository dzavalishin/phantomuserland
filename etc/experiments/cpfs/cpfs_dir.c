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
#include "cpfs_fsck.h"



// ----------------------------------------------------------------------------
//
// Implementation of directory scan to find/delete directory entry
//
// ----------------------------------------------------------------------------

// TODO fs->dir_mutex can be replaced with per-disk-blk mutex


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

        cpfs_mutex_lock( fs->dir_mutex ); // TODO need to use inode mutex here!
        cpfs_blkno_t phys_blk;
        rc = cpfs_ino_file_read( fs, dir_ino, blkpos*CPFS_BLKSIZE, data, CPFS_BLKSIZE , &phys_blk);

        if( rc )
        {
            cpfs_mutex_unlock( fs->dir_mutex ); // TODO need to use inode mutex here!
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
                        cpfs_mutex_unlock( fs->dir_mutex ); // TODO need to use inode mutex here!
                        cpfs_log_error("Can't write dir ino %d @ blk %d", dir_ino, blkpos );
                        return rc;
                    }
                }

                cpfs_mutex_unlock( fs->dir_mutex ); // TODO need to use inode mutex here!

                if( (*file_ino) >= fs->sb.ninode )
                {
                    cpfs_log_error("cpfs_namei_impl: dir ino %lld fname %s gives ino %lld", dir_ino, fname, file_ino );
                    continue; // Don't match dir entries if inode value is insane - right?
                }
                //cpfs_assert( (*file_ino) < fs->sb.ninode );

                return 0;
            }

        }
        cpfs_mutex_unlock( fs->dir_mutex ); // TODO need to use inode mutex here!

        blkpos++;

    }

    return ENOENT;
}





// ----------------------------------------------------------------------------
//
// Find inode by name in a given directory
//
// ----------------------------------------------------------------------------

errno_t
cpfs_namei( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t *file_ino )
{
    errno_t rc = cpfs_update_ino_atime( fs, dir_ino ); // SLOOW
    if( rc ) return rc;

    return cpfs_namei_impl( fs, dir_ino, fname, file_ino, 0 );
}




// ----------------------------------------------------------------------------
//
// Delete direcory entry
//
// ----------------------------------------------------------------------------

errno_t
cpfs_free_dirent( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname )
{
    errno_t rc = cpfs_update_ino_mtime( fs, dir_ino );
    if( rc ) return rc;

    cpfs_ino_t file_ino; // dummy
    return cpfs_namei_impl( fs, dir_ino, fname, &file_ino, 1 );
}





// ----------------------------------------------------------------------------
//
// Create and fill directory enry
//
// ----------------------------------------------------------------------------



// allocate a new dir entry in a dir

errno_t
cpfs_alloc_dirent( cpfs_fs_t *fs, cpfs_ino_t dir_ino, const char *fname, cpfs_ino_t file_ino ) 
{
    errno_t rc;
    int isdir;
    char data[CPFS_BLKSIZE];

    // TODO some speedup? In-mem hash?

    cpfs_assert(fname);

    if( file_ino >= fs->sb.ninode )
        cpfs_log_error("cpfs_alloc_dirent: dir ino %lld fname %s gives ino %lld", dir_ino, fname, file_ino );
    cpfs_assert( file_ino < fs->sb.ninode );

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

    cpfs_mutex_lock( fs->dir_mutex ); // TODO need to use inode mutex here!
    while( nblk-- > 0 )
    {
        
        cpfs_blkno_t phys_blk;
        rc = cpfs_ino_file_read( fs, dir_ino, blkpos*CPFS_BLKSIZE, data, CPFS_BLKSIZE, &phys_blk );
        if( rc )
        {
            cpfs_mutex_unlock( fs->dir_mutex ); // TODO need to use inode mutex here!
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
                //strlcpy( de->name, fname, sizeof(de->name) );
                strncpy( de->name, fname, sizeof(de->name)-1 );
                rc = cpfs_ino_file_write( fs, dir_ino, blkpos*CPFS_BLKSIZE, data, CPFS_BLKSIZE );
                if( rc )
                {
                    cpfs_log_error("Can't write dir ino %d @ blk %d", dir_ino, blkpos );
                }

                cpfs_mutex_unlock( fs->dir_mutex ); // TODO need to use inode mutex here!
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
    //strlcpy( de->name, fname, sizeof(de->name) );
    strncpy( de->name, fname, sizeof(de->name)-1 );

    rc = cpfs_ino_file_write( fs, dir_ino, blkpos*CPFS_BLKSIZE, data, CPFS_BLKSIZE );

    cpfs_mutex_unlock( fs->dir_mutex ); // TODO need to use inode mutex here!

    return rc;

}




// ----------------------------------------------------------------------------
//
// Find out if inode contains a directory
//
// ----------------------------------------------------------------------------


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
/*
    if( TRACE ) {
        //no optimisation
        
        
        int ino_in_blk = dir_ino % CPFS_INO_PER_BLK;
        int phys_blk = dir_ino/CPFS_INO_PER_BLK+1;
        
/ *
        cpfs_fsck_log(fsck_scan_dir_log_file, ino_in_blk,  phys_blk,  rdi);
* /
        
        fprintf(fsck_scan_dir_log_file,"blk=%d, pos=%d, data[isdir=%d, fileSize=%lld, nlink=%u, first block=%lld] blocks0[", phys_blk, ino_in_blk, *yesno, (long unsigned int)rdi->fsize, rdi->nlinks,  (long unsigned int)rdi->blocks0[0]);        
        for(int idx=0;idx<CPFS_INO_DIR_BLOCKS, rdi->blocks0[idx]!=0;idx++){
            fprintf(fsck_scan_dir_log_file,"%s%lld",  idx==0 ? "" :", ", (long unsigned int) rdi->blocks0[idx]);
        }        
        fprintf(fsck_scan_dir_log_file,"] indir[");
        for(int idx=0;idx<CPFS_MAX_INDIR, rdi->indir[idx]!=0;idx++){
            fprintf(fsck_scan_dir_log_file,"%s%lld", idx==0 ? "" :", ", (long unsigned int) rdi->indir[idx]);
        }        
        fprintf(fsck_scan_dir_log_file,"] \n");
        fflush(fsck_scan_dir_log_file);
        
    }
*/
    return 0;
}









// ----------------------------------------------------------------------------
//
// General directory scan function
//
// ----------------------------------------------------------------------------






errno_t
cpfs_scan_dir( cpfs_fs_t *fs, cpfs_ino_t dir_ino, dir_scan_func_t f, void *farg)
{
    errno_t rc;
    int isdir = 0;
    //cpfs_assert( file_ino != 0 );

    if(TRACE){ //for debug only
        int stop_line = 1;
        (void) stop_line; // remove unused var warning
    }
    
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

        cpfs_blkno_t phys_blk;
        rc = cpfs_ino_file_read( fs, dir_ino, blkpos*CPFS_BLKSIZE, data, CPFS_BLKSIZE, &phys_blk );

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
/*            if( TRACE ) {
                fprintf(fsck_scan_dir_log_file,"blk=%lld, pos=%d, data[ino=%lld, name=%s]  \n", (long long)phys_blk, i, (long long)de->inode, de->name );
                fflush(fsck_scan_dir_log_file);
            }*/
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

    if( de->inode ) printf("%03lld: '%s'\n", (long long)de->inode, de->name );

    return dir_scan_continue;
}



errno_t
cpfs_dump_dir( cpfs_fs_t *fs, cpfs_ino_t dir_ino )
{
    return cpfs_scan_dir( fs, dir_ino, dir_print, 0);
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













