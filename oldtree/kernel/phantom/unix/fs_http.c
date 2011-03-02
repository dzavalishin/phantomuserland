/**
 *
 * Phantom OS Unix Box
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel http fs (not even written, just stub from tcpfs)
 *
 *
**/

#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "httpfs"
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


static size_t      httpfs_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      httpfs_write(   struct uufile *f, void *dest, size_t bytes);
static errno_t     httpfs_stat(    struct uufile *f, struct stat *data);
static int     	   httpfs_ioctl(   struct uufile *f, errno_t *err, int request, void *data);

static size_t      httpfs_getpath( struct uufile *f, void *dest, size_t bytes);

// returns -1 for non-files
static ssize_t     httpfs_getsize( struct uufile *f);

//static void *      httpfs_copyimpl( void *impl );


struct uufileops httpfs_fops =
{
    .read 	= httpfs_read,
    .write 	= httpfs_write,

    .getpath 	= httpfs_getpath,
    .getsize 	= httpfs_getsize,

    //.copyimpl   = httpfs_copyimpl,

    .stat       = httpfs_stat,
    .ioctl      = httpfs_ioctl,
};





// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------


static errno_t     httpfs_open(struct uufile *, int create, int write);
static errno_t     httpfs_close(struct uufile *);
static uufile_t *  httpfs_namei(uufs_t *fs, const char *filename);
static uufile_t *  httpfs_getRoot(uufs_t *fs);
static errno_t     httpfs_dismiss(uufs_t *fs);


struct uufs http_fs =
{
    .name       = "http",
    .open 	= httpfs_open,
    .close 	= httpfs_close,
    .namei 	= httpfs_namei,
    .root 	= httpfs_getRoot,
    .dismiss    = httpfs_dismiss,

    .impl       = 0,
};


static struct uufile httpfs_root =
{
    .ops 	= &httpfs_fops,
    .pos        = 0,
    .fs         = &http_fs,
    .name       = "/",
    .flags      = UU_FILE_FLAG_NODESTROY,
};



// -----------------------------------------------------------------------
// FS methods
// -----------------------------------------------------------------------


static errno_t     httpfs_open(struct uufile *f, int create, int write)
{
    (void) f;
    (void) create;
    (void) write;
    return 0;
}

static errno_t     httpfs_close(struct uufile *f)
{
    if( f->impl )
    {
        struct uusocket *us = f->impl;
        http_close(us->prot_data);
        free(f->impl);
        f->impl = 0;
    }

    return 0;
}



// Create a file struct for given path
static uufile_t *  httpfs_namei(uufs_t *fs, const char *filename)
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

    ret->ops = &httpfs_fops;

    ret->pos = 0;
    ret->fs = &http_fs;
    ret->impl = us;
    ret->flags = UU_FILE_FLAG_NET|UU_FILE_FLAG_TCP|UU_FILE_FLAG_OPEN; // TODO wrong! open in open!

    return ret;
}

// Return a file struct for fs root
static uufile_t *  httpfs_getRoot(uufs_t *fs)
{
    (void) fs;
    return &httpfs_root;
}

static errno_t     httpfs_dismiss(uufs_t *fs)
{
    (void) fs;
    // TODO impl
    return 0;
}


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static size_t      httpfs_read(    struct uufile *f, void *buf, size_t len)
{
    struct uusocket *s = f->impl;

    if( s == 0 || s->prot_data == 0 )
        return -1;

    return tcp_recvfrom( s->prot_data, buf, len, &s->addr, 0, 0 );
}

static size_t      httpfs_write(   struct uufile *f, void *buf, size_t len)
{
    struct uusocket *s = f->impl;

    if( s == 0 || s->prot_data == 0 )
        return -1;

    return tcp_sendto( s->prot_data, buf, len, &s->addr);
}

//static errno_t     httpfs_stat(    struct uufile *f, struct ??);
//static errno_t     httpfs_ioctl(   struct uufile *f, struct ??);

static size_t      httpfs_getpath( struct uufile *f, void *dest, size_t bytes)
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
static ssize_t      httpfs_getsize( struct uufile *f)
{
    (void) f;
    return -1;
}

/*
static void *      httpfs_copyimpl( void *impl )
{
    //return strdup(impl);

}*/

static errno_t     httpfs_stat(    struct uufile *f, struct stat *data)
{
    (void) f;
    (void) data;

    return ESPIPE;
}

static int     	   httpfs_ioctl(   struct uufile *f, errno_t *err, int request, void *data)
{
    (void) f;
    (void) err;
    (void) request;
    (void) data;

    *err = ENODEV;
    return -1;
}




#endif // HAVE_UNIX

