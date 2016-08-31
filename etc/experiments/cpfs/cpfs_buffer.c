/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Disk buffers.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"




static void 	cpfs_clear_some_buf( cpfs_fs_t *fs );



// Just init - mutexes, memory, etc
errno_t
cpfs_buf_init( cpfs_fs_t *fs )
{
    fs->nbuf = CPFS_MAX_CONCUR_IO*4; // suppose one IO needs max 4 buffers - TODO unchecked
    fs->buf = calloc( fs->nbuf, sizeof(cpfs_buf_t) );

    return fs->buf ? 0 : ENOMEM;
}

// Find first free buf, read in data

static errno_t
cpfs_buf_alloc( cpfs_fs_t *fs, cpfs_blkno_t blk, cpfs_buf_t **buf )
{
    errno_t rc;
    //char shared = 1; // just shared now
    cpfs_mutex_lock( fs->buf_mutex );

    int i;
    for( i = 0; i < fs->nbuf; i++ )
    {
        //if( shared && fs->buf[i].used && (fs->buf[i].shared) && (fs->buf[i].blk == blk) )            goto use;

        if( fs->buf[i].used )
            continue;

    //use:
        rc = cpfs_disk_read( fs->disk_id, blk, fs->buf[i].data );
        if( rc )
        {
            cpfs_mutex_unlock( fs->buf_mutex );
            return EIO;
        }

        fs->buf[i].used++;
        fs->buf[i].lock++;
        fs->buf[i].blk = blk;
        //fs->buf[i].shared = shared;

        *buf = fs->buf+i;
        cpfs_mutex_unlock( fs->buf_mutex );
        return 0;
    }

    cpfs_mutex_unlock( fs->buf_mutex );
    return ENOMEM;
}

/*
static errno_t
cpfs_buf_free( cpfs_fs_t *fs, cpfs_blkno_t blk, cpfs_buf_t *buf )
{
    errno_t rc;

    cpfs_mutex_lock( fs->buf_mutex );
    cpfs_assert( buf->blk == blk );

    if( buf->used <= 0 )
    {
        cpfs_mutex_unlock( fs->buf_mutex );
        return ENOENT;
    }

    if( buf->write )
        rc = cpfs_disk_write( fs->disk_id, blk, buf->data );

    buf->used--;

    cpfs_mutex_unlock( fs->buf_mutex );

    return rc;
}
*/

static errno_t
cpfs_buf_find( cpfs_fs_t *fs, cpfs_blkno_t blk, cpfs_buf_t **buf ) // char shared
{
    cpfs_mutex_lock( fs->buf_mutex );

    int i;
    for( i = 0; i < fs->nbuf; i++ )
    {
        //if( fs->buf[i].used && (fs->buf[i].shared) && (fs->buf[i].blk == blk) )
        if( fs->buf[i].used && (fs->buf[i].blk == blk) )
        {
            *buf = fs->buf+i;
            (*buf)->lock++;
            cpfs_mutex_unlock( fs->buf_mutex );
            return 0;
        }
    }

    cpfs_mutex_unlock( fs->buf_mutex );
    return ENOENT;
}





// -----------------------------------------------------------------------
//
//  Public interface
//
// -----------------------------------------------------------------------

//
// Get buffer. Find existing or allocate new. Read in disk data if needed.
//
//


errno_t
cpfs_buf_lock( cpfs_fs_t *fs, cpfs_blkno_t blk, cpfs_buf_t **buf )
{
    errno_t rc;

    //cpfs_log_error( "lock blk %lld", (long long)blk );

    rc = cpfs_buf_find( fs, blk, buf );
    if(!rc) return rc; // Success

    while(1)
    {
        rc = cpfs_buf_alloc( fs, blk, buf );
        if(!rc) return rc; // Success

        // No free buffer, we need one more IO to push some buf out
        // RTFIX - unpredictable operation time, while(1)
        cpfs_clear_some_buf( fs );
    }
}




errno_t
cpfs_buf_unlock( cpfs_fs_t *fs, cpfs_blkno_t blk, char write )
{
    int i;

    //cpfs_log_error( "unlock blk %lld", (long long)blk );

    cpfs_mutex_lock( fs->buf_mutex );

    for( i = 0; i < fs->nbuf; i++ )
    {
        if( fs->buf[i].used && (fs->buf[i].blk == blk) )
        {
            if( write ) fs->buf[i].write = 1;

            if( fs->buf[i].lock <= 0 )
                cpfs_panic( "unlock unlocked buf blk %lld", (long long)blk );

            fs->buf[i].lock--;
            cpfs_mutex_unlock( fs->buf_mutex );
            return 0;
        }
    }

    cpfs_log_error( "unlock: buf for blk %lld is not found", (long long)blk );

    cpfs_mutex_unlock( fs->buf_mutex );

    return 0;
}













static void
cpfs_clear_some_buf( cpfs_fs_t *fs )
{
    int i;
    errno_t rc;

    cpfs_mutex_lock( fs->buf_mutex );

    // TODO look for LRU or just do round robin lookup
    for( i = 0; i < fs->nbuf; i++ )
    {
        if( fs->buf[i].used && !(fs->buf[i].lock) )
        {
            //*buf = fs->buf+i;
            //(*buf)->lock++;

            if( fs->buf[i].write )
            {
                rc = cpfs_disk_write( fs->disk_id, fs->buf[i].blk, fs->buf[i].data );
                if( rc )
                {
                    // we're quite fried, we even can't return error to caller
                    // add inode num to each buffer, mark inode as having IO error, return error on next IO? close?
                    cpfs_log_error( "cache write IO error, blk %lld", (long long)fs->buf[i].blk );
                    cpfs_panic("io err in cache write blk %lld", (long long)fs->buf[i].blk);
                    continue;
                }
                fs->buf[i].write = 0;
            }

            fs->buf[i].used = 0;

            cpfs_mutex_unlock( fs->buf_mutex );
            return;
        }
    }

    cpfs_mutex_unlock( fs->buf_mutex );

}



void
cpfs_clear_all_buf( cpfs_fs_t *fs )
{
    int i;
    errno_t rc;

    cpfs_mutex_lock( fs->buf_mutex );

    for( i = 0; i < fs->nbuf; i++ )
    {
        if( fs->buf[i].used && fs->buf[i].lock )
            cpfs_panic("locked blk %lld in cpfs_clear_all_buf", (long long)fs->buf[i].blk);

        if( fs->buf[i].used && !(fs->buf[i].lock) )
        {
            //*buf = fs->buf+i;
            //(*buf)->lock++;

            if( fs->buf[i].write )
            {
                rc = cpfs_disk_write( fs->disk_id, fs->buf[i].blk, fs->buf[i].data );
                if( rc )
                {
                    // we're quite fried, we even can't return error to caller
                    // add inode num to each buffer, mark inode as having IO error, return error on next IO? close?
                    cpfs_log_error( "cache write IO error, blk %lld", (long long)fs->buf[i].blk );
                    //cpfs_panic("io err in cache write blk %lld", (long long)fs->buf[i].blk); // panic does not help here
                    continue;
                }

                fs->buf[i].write = 0;
                fs->buf[i].used = 0;
            }

            //cpfs_mutex_unlock( fs->buf_mutex );
            //return;
        }
    }

    cpfs_mutex_unlock( fs->buf_mutex );

}





