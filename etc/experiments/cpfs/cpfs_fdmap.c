/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * File descriptors map.
 *
 * Used to allocate and access file descriptors passed out to users.
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"



static cpfs_fid_t       *fdmap;                 // Map of file descriptors - global for all FS instances!
static int              nfdmap;
static int		fdmap_alloc;            // Last postion of allocator search
static cpfs_mutex_t     fdmap_mutex;



// Just init - mutexes, memory, etc
errno_t
cpfs_fdmap_init( void )
{
    nfdmap = CPFS_MAX_CONCUR_IO;
    fdmap = calloc( nfdmap, sizeof(cpfs_fid_t) );

    cpfs_mutex_init( &fdmap_mutex );

    return fdmap ? 0 : ENOMEM;
}


errno_t
cpfs_fdmap_alloc( cpfs_fs_t *fs, cpfs_ino_t ino, int *fd, cpfs_fid_t **fid )
{
    cpfs_assert( fd != 0 );
    cpfs_assert( fid != 0 );

    cpfs_mutex_lock( fdmap_mutex );

    int pos = fdmap_alloc;

    while(1)
    {
        pos++;
        if( pos >= nfdmap ) pos = 0;

        if( pos == fdmap_alloc)
            break;

        if( !fdmap[pos].used )
        {
            fdmap[pos].inode = ino;
            fdmap[pos].fs = fs;
            fdmap[pos].used++;
            fdmap[pos].lock = 0;

            *fd = pos;
            *fid = fdmap+pos;

            cpfs_mutex_unlock( fdmap_mutex );
            return 0;
        }

    }

    cpfs_mutex_unlock( fdmap_mutex );
    return EMFILE;
}

errno_t
cpfs_fdmap_free( int fd )
{
    if( (fd < 0) || (fd >= nfdmap ) )
        return EINVAL;

    cpfs_mutex_lock( fdmap_mutex );

    if( !fdmap[fd].used )
    {
        cpfs_mutex_unlock( fdmap_mutex );
        return EINVAL;
    }

    if( fdmap[fd].lock )
    {
        cpfs_mutex_unlock( fdmap_mutex );
        return EWOULDBLOCK;
    }

    fdmap[fd].used--;

    cpfs_mutex_unlock( fdmap_mutex );
    return 0;
}


errno_t
cpfs_fdmap_lock( int fd, cpfs_fid_t **fid )
{
    if( (fd < 0) || (fd >= nfdmap ) )
        return EINVAL;

    cpfs_mutex_lock( fdmap_mutex );

    if( !fdmap[fd].used )
    {
        cpfs_mutex_unlock( fdmap_mutex );
        return EINVAL;
    }

    // TODO document this limit
    if( !fdmap[fd].lock > 126 ) // TODO max char macros
    {
        cpfs_panic( "too many locks for fd %d", fd );
        //cpfs_mutex_unlock( fdmap_mutex );
        //return EINVAL;
    }

    fdmap[fd].lock++;
    *fid = fdmap+fd;

    cpfs_mutex_unlock( fdmap_mutex );
    return 0;
}


errno_t
cpfs_fdmap_unlock( int fd )
{
    if( (fd < 0) || (fd >= nfdmap ) )
        return EINVAL;

    cpfs_mutex_lock( fdmap_mutex );

    if( !fdmap[fd].used )
    {
        cpfs_panic( "unlock unused fd %d", fd );
        //cpfs_mutex_unlock( fdmap_mutex );
        //return EINVAL;
    }

    if( !fdmap[fd].lock <= 0 )
    {
        cpfs_panic( "unlock unlocked fd %d", fd );
        //cpfs_mutex_unlock( fdmap_mutex );
        //return EINVAL;
    }

    fdmap[fd].lock--;

    cpfs_mutex_unlock( fdmap_mutex );
    return 0;
}




int
cpfs_fdmap_is_inode_used( cpfs_fs_t *fs, cpfs_ino_t ino )
{
    cpfs_mutex_lock( fdmap_mutex );

    int i;
    for( i = 0; i < nfdmap; i++ )
    {
        if( fdmap[i].inode == ino )
            return 1;
    }

    cpfs_mutex_unlock( fdmap_mutex );
    return 0;
}




