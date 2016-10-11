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

// to detect we're compiled for POK/JetOS
#include <stdio.h>


static cpfs_fid_t       *fdmap = 0;             // Map of file descriptors - global for all FS instances!
static int              nfdmap;
static int		fdmap_alloc;            // Last position of allocator search
static cpfs_mutex_t     fdmap_mutex;

void            cpfs_fdmap_stop( void );


// ----------------------------------------------------------------------------
//
// init/stop
//
// ----------------------------------------------------------------------------


// Just init - mutexes, memory, etc
errno_t
cpfs_fdmap_init( void )
{
    if( fdmap ) return 0;

    nfdmap = CPFS_MAX_CONCUR_IO;
    fdmap = calloc( nfdmap, sizeof(cpfs_fid_t) );
    if( fdmap == 0 ) return ENOMEM;

    cpfs_mutex_init( &fdmap_mutex );

#ifndef __POK_LIBC_STDIO_H__
    if( fdmap != 0 )
        atexit( cpfs_fdmap_stop );
#endif

    return fdmap ? 0 : ENOMEM;
}


void
cpfs_fdmap_stop( void )
{

    if( fdmap ) free( fdmap );
    fdmap = 0;

    cpfs_mutex_stop( fdmap_mutex );

}


// ----------------------------------------------------------------------------
//
// Allocate file descriptor - move to OS code?
//
// ----------------------------------------------------------------------------



errno_t
cpfs_fdmap_alloc( cpfs_fs_t *fs, cpfs_ino_t ino, int *fd )
{
    cpfs_assert( fd != 0 );
    cpfs_assert( fs != 0 );

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
            //*fid = fdmap+pos;

            //printf( "allock for ino %lld\n", ino );

            cpfs_mutex_unlock( fdmap_mutex );
            return 0;
        }

    }

    cpfs_mutex_unlock( fdmap_mutex );
    return EMFILE;
}


// ----------------------------------------------------------------------------
//
// Free file descriptor - TODO move to OS code?
//
// ----------------------------------------------------------------------------




errno_t
cpfs_fdmap_free( int fd )
{
    if( (fd < 0) || (fd >= nfdmap ) )
    {
        printf("can't free fd %d: out of range\n", fd);
        return EINVAL;
    }

    cpfs_mutex_lock( fdmap_mutex );

    if( !fdmap[fd].used )
    {
        printf("can't free fd %d: unused\n", fd);
        cpfs_mutex_unlock( fdmap_mutex );
        return EINVAL;
    }

    if( fdmap[fd].lock )
    {
        printf("can't free fd %d: locked\n", fd);
        cpfs_mutex_unlock( fdmap_mutex );
        return EWOULDBLOCK;
    }

    fdmap[fd].used--;

    //printf( "free for ino %lld\n", fdmap[fd].inode );

    cpfs_mutex_unlock( fdmap_mutex );
    return 0;
}




// ----------------------------------------------------------------------------
//
// ? TODO describe me, use me
//
// ----------------------------------------------------------------------------




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

    if( fdmap[fd].lock > 126 ) // TODO max char macros // TODO don't panic
    {
        cpfs_panic( "too many locks for fd %d", fd );
        //cpfs_mutex_unlock( fdmap_mutex );
        //return EINVAL;
    }

    fdmap[fd].lock++;
    *fid = fdmap+fd;

    //printf( "lock for ino %lld\n", fdmap[fd].inode );

    cpfs_mutex_unlock( fdmap_mutex );
    return 0;
}


// ----------------------------------------------------------------------------
//
// ? TODO describe me
//
// ----------------------------------------------------------------------------




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

    if( fdmap[fd].lock <= 0 )
    {
        cpfs_panic( "unlock unlocked fd %d", fd );
        //cpfs_mutex_unlock( fdmap_mutex );
        //return EINVAL;
    }

    //printf( "unlock for ino %lld\n", fdmap[fd].inode );

    fdmap[fd].lock--;

    cpfs_mutex_unlock( fdmap_mutex );
    return 0;
}


// ----------------------------------------------------------------------------
//
// Find out if inode is used by some open file
//
// ----------------------------------------------------------------------------




int
cpfs_fdmap_is_inode_used( cpfs_fs_t *fs, cpfs_ino_t ino )
{
    cpfs_mutex_lock( fdmap_mutex );

    int i;
    for( i = 0; i < nfdmap; i++ )
    {
        if( (fdmap[i].inode == ino) && (fdmap[i].fs == fs) && (fdmap[i].used) )
        {
            cpfs_mutex_unlock( fdmap_mutex );
            return 1;
        }
    }

    cpfs_mutex_unlock( fdmap_mutex );
    return 0;
}


// ----------------------------------------------------------------------------
//
// Find inode and FS instance by file descriptor
//
// ----------------------------------------------------------------------------




errno_t
cpfs_fdmap_get( int fd, cpfs_ino_t *ino, cpfs_fs_t **fs  )
{
    cpfs_assert( ino != 0 );

    if( (fd < 0) || (fd >= nfdmap ) )
        return EINVAL;

    cpfs_mutex_lock( fdmap_mutex );

    if( !fdmap[fd].used )
    {
        cpfs_mutex_unlock( fdmap_mutex );
        return EINVAL;
    }

    *ino = fdmap[fd].inode;
    *fs  = fdmap[fd].fs;

    cpfs_mutex_unlock( fdmap_mutex );
    return 0;
}



