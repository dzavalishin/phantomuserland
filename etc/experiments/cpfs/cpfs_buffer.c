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


// TODO mark buf with actual data somehow, skip disk read if so
// TODO only shared bufs?


// Just init - mutexes, memory, etc
errno_t
cpfs_buf_init( cpfs_fs_t *fs )
{
    fs->nbuf = CPFS_MAX_CONCUR_IO*4; // suppose one IO needs max 4 buffers - TODO unchecked
    fs->buf = calloc( fs->nbuf, sizeof(cpfs_buf_t) );

    return fs->buf ? 0 : ENOMEM;
}


// share buffers? shared flag?

errno_t
cpfs_buf_alloc( cpfs_fs_t *fs, cpfs_blkno_t blk, cpfs_buf_t **buf, char shared )
{
    cpfs_mutex_lock( fs->buf_mutex );

    int i;
    for( i = 0; i < fs->nbuf; i++ )
    {
        if( shared && fs->buf[i].used && (fs->buf[i].shared) && (fs->buf[i].blk == blk) )
            goto use;

        if( fs->buf[i].used )
            continue;

    use:
        fs->buf[i].used++;
        fs->buf[i].blk = blk;
        fs->buf[i].shared = shared;

        *buf = fs->buf+i;
        cpfs_mutex_unlock( fs->buf_mutex );
        return 0;
    }

    cpfs_mutex_unlock( fs->buf_mutex );
    return ENOMEM;
}

// TODO check - we release buf even if write failed
errno_t
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


errno_t
cpfs_buf_find( cpfs_fs_t *fs, cpfs_blkno_t blk, cpfs_buf_t **buf ) // char shared
{
    cpfs_mutex_lock( fs->buf_mutex );

    int i;
    for( i = 0; i < fs->nbuf; i++ )
    {
        if( fs->buf[i].used && (fs->buf[i].shared) && (fs->buf[i].blk == blk) )
        {
            *buf = fs->buf+i;
            cpfs_mutex_unlock( fs->buf_mutex );
            return 0;
        }
    }

    cpfs_mutex_unlock( fs->buf_mutex );
    return ENOENT;
}











