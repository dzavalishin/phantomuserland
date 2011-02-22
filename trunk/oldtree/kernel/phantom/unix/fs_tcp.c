/**
 *
 * Phantom OS Unix Box
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel tcp fs
 *
 *
**/

#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "tcpfs"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include "net.h"
#include "tcp.h"

#include <unix/uufile.h>
#include <unix/uunet.h>
//#include <unix/uuprocess.h>
#include <malloc.h>
#include <string.h>



// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------


static size_t      tcpfs_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      tcpfs_write(   struct uufile *f, const void *src, size_t bytes);
static errno_t     tcpfs_stat(    struct uufile *f, struct stat *data);
static int     	   tcpfs_ioctl(   struct uufile *f, errno_t *err, int request, void *data);

static size_t      tcpfs_getpath( struct uufile *f, void *dest, size_t bytes);

// returns -1 for non-files
static ssize_t     tcpfs_getsize( struct uufile *f);

//static void *      tcpfs_copyimpl( void *impl );


struct uufileops tcpfs_fops =
{
    .read 	= tcpfs_read,
    .write 	= tcpfs_write,

    .getpath 	= tcpfs_getpath,
    .getsize 	= tcpfs_getsize,

    //.copyimpl   = tcpfs_copyimpl,

    .stat       = tcpfs_stat,
    .ioctl      = tcpfs_ioctl,
};





// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------


//static uufile_t *  tcpfs_open(const char *name, int create, int write);
static errno_t     tcpfs_open(struct uufile *, int create, int write);
static errno_t     tcpfs_close(struct uufile *);

// Create a file struct for given path
static uufile_t *  tcpfs_namei(uufs_t *fs, const char *filename);

// Return a file struct for fs root
static uufile_t *  tcpfs_getRoot(uufs_t *fs);
static errno_t     tcpfs_dismiss(uufs_t *fs);


struct uufs tcp_fs =
{
    .name       = "tcp",
    .open 	= tcpfs_open,
    .close 	= tcpfs_close,
    .namei 	= tcpfs_namei,
    .root 	= tcpfs_getRoot,
    .dismiss    = tcpfs_dismiss,

    .impl       = 0,
};


static struct uufile tcpfs_root =
{
    .ops 	= &tcpfs_fops,
    .pos        = 0,
    .fs         = &tcp_fs,
    .name       = "/",
};



// -----------------------------------------------------------------------
// FS methods
// -----------------------------------------------------------------------


static errno_t     tcpfs_open(struct uufile *f, int create, int write)
{
    (void) f;
    (void) create;
    (void) write;
    return 0;
}

static errno_t     tcpfs_close(struct uufile *f)
{
    if( f->impl )
    {
        struct uusocket *us = f->impl;
        tcp_close(us->prot_data);
        free(f->impl);
        f->impl = 0;
    }


    return 0;
}



// Create a file struct for given path
static uufile_t *  tcpfs_namei(uufs_t *fs, const char *filename)
{
    int ip0, ip1, ip2, ip3, port;

    (void) fs;

    if( 5 != sscanf( filename, "%d.%d.%d.%d:%d", &ip0, &ip1, &ip2, &ip3, &port ) )
    {
        return 0;
    }

    sockaddr addr;
    addr.port = port;

    addr.addr.len = 4;
    addr.addr.type = ADDR_TYPE_IP;
    NETADDR_TO_IPV4(addr.addr) = IPV4_DOTADDR_TO_ADDR(ip0, ip2, ip2, ip3);

    struct uusocket *us = calloc(1, sizeof(struct uusocket));
    if(us == 0)  return 0;

    us->addr = addr;


    if( tcp_open(&(us->prot_data)) )
    {
        SHOW_ERROR0(0, "can't prepare endpoint");
    fail:
        free(us);
        return 0;
    }

    if( tcp_connect( us->prot_data, &us->addr) )
    {
        SHOW_ERROR(0, "can't connect to %s", filename);
        goto fail;
    }


    uufile_t *ret = create_uufile();

    ret->ops = &tcpfs_fops;

    ret->pos = 0;
    ret->fs = &tcp_fs;
    ret->impl = us;
    ret->flags = UU_FILE_FLAG_NET|UU_FILE_FLAG_TCP;

    return ret;
}

// Return a file struct for fs root
static uufile_t *  tcpfs_getRoot(uufs_t *fs)
{
    (void) fs;
    return &tcpfs_root;
}

static errno_t     tcpfs_dismiss(uufs_t *fs)
{
    (void) fs;
    // TODO impl
    return 0;
}


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static size_t      tcpfs_read(    struct uufile *f, void *buf, size_t len)
{
    struct uusocket *s = f->impl;

    if( s == 0 || s->prot_data == 0 )
        return -1;

    return tcp_recvfrom( s->prot_data, buf, len, &s->addr, 0, 0 );
}

static size_t      tcpfs_write(   struct uufile *f, const void *buf, size_t len)
{
    struct uusocket *s = f->impl;

    if( s == 0 || s->prot_data == 0 )
        return -1;

    return tcp_sendto( s->prot_data, buf, len, &s->addr);
}

//static errno_t     tcpfs_stat(    struct uufile *f, struct ??);
//static errno_t     tcpfs_ioctl(   struct uufile *f, struct ??);

static size_t      tcpfs_getpath( struct uufile *f, void *dest, size_t bytes)
{
    if( bytes == 0 ) return 0;

    struct uusocket *s = f->impl;

    int i4a = NETADDR_TO_IPV4(s->addr.addr);
    char *i4b = (char *)&i4a;

    snprintf( dest, bytes, "%d.%d.%d.%d:%d",
              i4b[0], i4b[1], i4b[2], i4b[3],
              s->addr.port
            );
    return strlen(dest);
}

// returns -1 for non-files
static ssize_t      tcpfs_getsize( struct uufile *f)
{
    (void) f;
    return -1;
}

/*
static void *      tcpfs_copyimpl( void *impl )
{
    //return strdup(impl);

}*/

static errno_t     tcpfs_stat(    struct uufile *f, struct stat *data)
{
    (void) f;
    (void) data;

    return ESPIPE;
}

static int     	   tcpfs_ioctl(   struct uufile *f, errno_t *err, int request, void *data)
{
    (void) f;
    (void) err;
    (void) request;
    (void) data;

    *err = ENODEV;
    return -1;
}




#endif // HAVE_UNIX

