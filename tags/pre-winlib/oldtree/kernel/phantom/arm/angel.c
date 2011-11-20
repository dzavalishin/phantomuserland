/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM angel semihosting support.
 *
**/

#define DEBUG_MSG_PREFIX "angel"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <hal.h>
#include <errno.h>
#include <phantom_libc.h>
#include <sys/fcntl.h>

#include <device.h>
#include <pager_io_req.h>
#include <disk.h>

#define SYS_OPEN        0x01
#define SYS_CLOSE       0x02
#define SYS_WRITEC      0x03
#define SYS_WRITE0      0x04
#define SYS_WRITE       0x05
#define SYS_READ        0x06
#define SYS_READC       0x07
#define SYS_ISTTY       0x09
#define SYS_SEEK        0x0a
#define SYS_FLEN        0x0c
#define SYS_TMPNAM      0x0d
#define SYS_REMOVE      0x0e
#define SYS_RENAME      0x0f
#define SYS_CLOCK       0x10
#define SYS_TIME        0x11
#define SYS_SYSTEM      0x12
#define SYS_ERRNO       0x13
#define SYS_GET_CMDLINE 0x15
#define SYS_HEAPINFO    0x16
#define SYS_EXIT        0x18


// In entry.S
int call_arm_angel( int nsys, void *args );


static void noangel(void)
{
    panic("my angel seems to be dead");
}

int have_angel(void)
{
    return( -1 != call_arm_angel( SYS_TIME, 0 ) );
}

static void assert_angel(void)
{
    static int ok = 0;
    if( ok ) return;

    if( !have_angel() )
        noangel();
    ok = 1;
}



void arm_angel_halt(void)
{
    call_arm_angel( SYS_EXIT, 0 );
    noangel();
}



time_t arm_angel_time(void)
{
    assert_angel();
    return call_arm_angel( SYS_TIME, 0 );
}

long arm_angel_clock(void)
{
    assert_angel();
    return call_arm_angel( SYS_CLOCK, 0 );
}




int arm_angel_open(const char *name, int flags )
{
    assert(name);
    assert_angel();

    // Cretinos...
    int angel_mode = 1; // RDONLY+BINARY

    if( flags & O_WRONLY )
        angel_mode = 3;// RDWR+BINARY

    int args[3];

    args[0] = (addr_t)name;
    args[1] = angel_mode;
    args[2] = 0666; // ? not sure

    return call_arm_angel( SYS_OPEN, args );
}


int arm_angel_close(int fd)
{
    assert_angel();

    int args[1];

    args[0] = fd;

    return call_arm_angel( SYS_CLOSE, args );
}


int arm_angel_flen(int fd)
{
    assert_angel();

    int args[1];

    args[0] = fd;

    return call_arm_angel( SYS_FLEN, args );
}



int arm_angel_putc_console(char c)
{
    assert_angel();

    char args[1];

    args[0] = c;

    return call_arm_angel( SYS_WRITEC, args );
}



int arm_angel_write(int fd, void *data, int len )
{
    assert_angel();

    int args[3];

    args[0] = fd;
    args[1] = (addr_t)data;
    args[2] = len;

    return call_arm_angel( SYS_WRITE, args );
}


int arm_angel_read(int fd, void *data, int len )
{
    assert_angel();

    int args[3];

    args[0] = fd;
    args[1] = (addr_t)data;
    args[2] = len;

    return call_arm_angel( SYS_READ, args );
}


int arm_angel_seek(int fd, int pos )
{
    assert_angel();

    int args[2];

    args[0] = fd;
    args[1] = pos;

    return call_arm_angel( SYS_SEEK, args );
}


//! Returns len of cmdline, or negative on error
int arm_angel_get_cmd_line(char *dest, int len )
{
    assert(len);
    assert_angel();

    int args[2];

    args[0] = (addr_t)dest;
    args[1] = len;

    *dest = 0; // If it doesnt'work, return empty string
    int rc = call_arm_angel( SYS_GET_CMDLINE, args );

    return (rc < 0) ? rc : args[1];
}














static phantom_disk_partition_t *phantom_create_angel_partition_struct( long size, phantom_device_t *dev );



static int seq_number = 0;


phantom_device_t *driver_angel_disk_probe( int stage )
{
    (void) stage;

    const char *fn = "angel.img";

    SHOW_FLOW( 1, "Look for angel disk '%s'", fn );

    if(seq_number)
    {
        SHOW_ERROR0( 0, "Just one drv instance yet");
        return 0;
    }

    if( !have_angel() )
    {
        SHOW_ERROR0( 1, "No angel, sorry" );
        return 0;
    }

    int fd = arm_angel_open( fn, O_RDWR );
    if( fd < 0 )
    {
        SHOW_ERROR0( 1, "File open error" );
        return 0;
    }

    int len = arm_angel_flen(fd);
    if( len < 0 )
    {
        SHOW_ERROR0( 1, "File length negative" );
        arm_angel_close(fd);
        return 0;
    }

    SHOW_INFO( 0, "Arm Angel disk in file '%s'", fn );

    phantom_device_t * dev = (phantom_device_t *)calloc(1, sizeof(phantom_device_t));
    assert(dev);

    dev->name = "AngelDisk";
    dev->seq_number = seq_number++;
    dev->drv_private = (void*) fd;


    phantom_disk_partition_t *p = phantom_create_angel_partition_struct( len, dev );
    (void) p;

#if 0
    errno_t ret = phantom_register_disk_drive(p);
    if( ret )
        SHOW_ERROR( 0, "Can't register Angel drive: %d", ret );
#endif

    return dev;
}


#define SECT_SIZE 512

static errno_t a_read( int fd, physaddr_t pa )
{
    char data[SECT_SIZE];

    int rc = arm_angel_read( fd, data, SECT_SIZE );

    if( rc < 0 )
        return rc;

    memcpy_v2p( pa, data, SECT_SIZE );

    return 0;
}


static errno_t a_write( int fd, physaddr_t pa )
{
    char data[SECT_SIZE];

    memcpy_p2v( data, pa, SECT_SIZE );

    int rc = arm_angel_write( fd, data, SECT_SIZE );

    if( rc < 0 )
        return rc;

    return 0;
}


static errno_t driver_angel_disk_rq(phantom_device_t *dev, pager_io_request *rq)
{
    int sect = rq->blockNo;
    int n = rq->nSect;
    physaddr_t pa = rq->phys_page;

    int fd = (int) (dev->drv_private);

    while(n--)
    {
        if( arm_angel_seek(fd, sect*SECT_SIZE ) < 0 )
            return -1;

        if( rq->flag_pageout )
        {
            if( a_write( fd, pa ) )
                return -1;
        }
        else
        {
            if( a_read ( fd, pa ) )
                return -1;
        }

        sect++;
        pa += SECT_SIZE;
    }

    return 0;
}




static errno_t angelAsyncIo( struct phantom_disk_partition *p, pager_io_request *rq )
{
    assert(p->specific != 0);

    // Temp! Rewrite!
    //assert(p->base == 0 );

    phantom_device_t *dev = (phantom_device_t *)p->specific;

    int ret = driver_angel_disk_rq( dev, rq );

    rq->rc = ret ? EIO : 0;
    rq->flag_ioerror = ret;

    // callback - is it ok to do it here?
    pager_io_request_done( rq );

    return rq->rc;
}


static phantom_disk_partition_t *phantom_create_angel_partition_struct( long size, phantom_device_t *dev )
{
    phantom_disk_partition_t * ret = phantom_create_partition_struct( 0, 0, size);

    ret->asyncIo = angelAsyncIo;
    ret->flags |= PART_FLAG_IS_WHOLE_DISK;

    ret->specific = dev;

    strlcpy( ret->name, "angel", PARTITION_NAME_LEN );

    return ret;
}










