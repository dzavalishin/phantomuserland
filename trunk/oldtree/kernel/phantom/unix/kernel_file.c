/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel unix-like io calls
 *
 *
**/


#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "kunix"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <sys/fcntl.h>
#include <sys/stat.h>
#include <kernel/unix.h>
#include <unix/uufile.h>
#include <kunix.h>
#include <malloc.h>
#include <stdio.h>


// Kernel's global 'process' strunc
static uuprocess_t k_u;

void kernel_unix_init(void)
{
    uuprocess_t *u = &k_u;

    u->pid = -1;
    u->ppid = -1;
    u->pgrp_pid = -1;
    u->sess_pid = -1;

    u->uid = 0;
    u->euid = 0;
    u->gid = 0;
    u->egid = 0;

    strlcpy( u->cmd, "kernel", MAX_UU_CMD );

    u->umask = 0777;

    u->mem_start = 0;
    u->mem_end    = 0;

}

errno_t k_open( int *fd, const char *name, int flags, int mode )
{
    int err = 0;

    int rfd = usys_open( &err, &k_u, name, flags, mode );

    if( err )
        return err;

    *fd = rfd;

    return 0;
}



errno_t k_read(int *nread, int fd, void *addr, int count )
{
    int err = 0;
    int ret = usys_read( &err, &k_u, fd, addr, count );

    if( err )
        return err;

    *nread = ret;

    return 0;
}


errno_t k_write(int *nread, int fd, const void *addr, int count )
{
    int err = 0;
    int ret = usys_write( &err, &k_u, fd, addr, count );

    if( err )
        return err;

    *nread = ret;

    return 0;
}


errno_t k_close( int fd )
{
    int err = 0;
    usys_close( &err, &k_u, fd );
    return err;
}



errno_t k_seek(int *pos, int fd, int offset, int whence )
{
    int err = 0;
    int ret = usys_lseek( &err, &k_u, fd, offset, whence );

    if( err )
        return err;

    *pos = ret;
    return 0;
}



errno_t k_stat( const char *path, struct stat *data, int statlink )
{
    int err = 0;
    usys_stat( &err, &k_u, path, data, statlink );
    return err;
}







// -----------------------------------------------------------------------
// Toolkit
// -----------------------------------------------------------------------


errno_t k_load_file( void **odata, int *osize, const char *fname )
{
    errno_t e;
    int fd;
    struct stat info;
    int nread;

    e = k_stat( fname, &info, 1 );
    if( e )
    {
        SHOW_ERROR( 1, "can't stat %s", fname );
        return e;
    }

    int size = info.st_size;

    if( size <= 0 )
    {
        SHOW_ERROR( 0, "%s size is %d", fname, size );
        return ENOMEM;
    }

    void *data = calloc( 1, size );

    e = k_open( &fd, fname, O_RDONLY, 0 );
    if( e )
    {
        SHOW_ERROR( 1, "can't open %s", fname );
        return e;
    }

    errno_t re;

    int toread = size;
    void *dp = data;

    while( toread )
    {
        re = k_read( &nread, fd, dp, size );
        if( re || nread <= 0)
            break;

        toread -= nread;
        dp += nread;
    }

    e = k_close( fd );
    if( e )
        SHOW_ERROR( 0, "Can't close %s", fname );

    if( re )
        free( data );
    else
    {
        *odata = data;
        *osize = size;
    }

    return re;
}



#endif




