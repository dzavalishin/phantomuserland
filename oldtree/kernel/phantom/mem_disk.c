/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * In memory disk pseudo-driver.
 * Used to access mem-mapped flashes and ramdisks.
 *
**/

#define DEV_NAME "memdisk"
#define DEBUG_MSG_PREFIX "memdisk"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/drivers.h>
#include <kernel/page.h>
#include <kernel/atomic.h>
#include <kernel/libkern.h>

#include <errno.h>
#include <assert.h>
#include <hal.h>
#include <time.h>
#include <threads.h>
#include <disk.h>
#include <disk_q.h>

#include <pager_io_req.h>

#include <dev/mem_disk.h>

typedef struct
{
    int                 can_write;
} memdisk_t;


static void memdisk_connect( phantom_device_t *dev, int nSect );


//static int memdisk_start(phantom_device_t *dev);
//static int memdisk_stop(phantom_device_t *dev);

static int memdisk_write(phantom_device_t *dev, const void *buf, int len);
static int memdisk_read(phantom_device_t *dev, void *buf, int len);

//static int memdisk_ioctl(struct phantom_device *dev, int type, void *buf, int len);



static int seq_number = 0;
errno_t driver_memdisk_init( physaddr_t disk_data, size_t disk_data_size, int flags )
{

    SHOW_FLOW( 1, "Start " DEV_NAME " @ %p size %d", disk_data, disk_data_size );

    phantom_device_t * dev = calloc(sizeof(phantom_device_t),1);


    dev->iomemsize = disk_data_size;

    if( flags & MEMDISK_FLAG_VIRT )
    {
        dev->iomem = disk_data;
    }
    else
    {
        const int n_pages = BYTES_TO_PAGES(dev->iomemsize);
        void *va;

        if( hal_alloc_vaddress( &va, n_pages ) )
            panic("Can't alloc vaddress for %d mem pages", n_pages);

        hal_pages_control_etc( disk_data, va, n_pages, page_map, page_rw, 0 );

        dev->iomem = (addr_t)va; // loose phys addr?!
    }


    dev->name = DEV_NAME;
    dev->seq_number = seq_number++;

    //dev->dops.start = memdisk_start;
    //dev->dops.stop  = memdisk_stop;
    dev->dops.read  = memdisk_read;
    dev->dops.write = memdisk_write;
    //dev->dops.ioctl = memdisk_ioctl;

/*
    memdisk_t *es = calloc(1,sizeof(memdisk_t));
    assert(es);
    dev->drv_private = es;

    es->nunit = seq_number-1;

    if( memdisk_init(dev) )
        goto free1;
*/

    memdisk_connect( dev, disk_data_size/512 );

    return 0;

//free1:
    //free(es);

//free:
    free(dev);
    return ENXIO;
}













static int memdisk_read(phantom_device_t *dev, void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;

    //memdisk_t *es = dev->drv_private;

    return -1;
}

static int memdisk_write(phantom_device_t *dev, const void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;

    //memdisk_t *es = dev->drv_private;

    return -1;
}





//---------------------------------------------------------------------------
// Disk io interface
//---------------------------------------------------------------------------

static errno_t memdisk_AsyncIo( struct phantom_disk_partition *part, pager_io_request *rq )
{
    phantom_device_t *dev = part->specific;
    //memdisk_t *a = dev->drv_private;

    //rq->flag_ioerror = 0;
    rq->rc = 0;

    size_t size = rq->nSect * part->block_size;
    off_t shift = rq->blockNo * part->block_size;

    if( size+shift > dev->iomemsize )
    {
        //rq->flag_ioerror = 1;
        rq->rc = EIO;
        pager_io_request_done( rq );
        return EIO;
    }

    if(rq->flag_pageout)
    {
        //rq->flag_ioerror = 1;
        rq->rc = EIO;
    }
    else
    {
        // read
        memcpy_v2p( rq->phys_page, (void *)dev->iomem+shift, size );
    }

    pager_io_request_done( rq );
    return rq->rc;
}


phantom_disk_partition_t *phantom_create_memdisk_partition_struct( phantom_device_t *dev, long size, int unit )
{
    phantom_disk_partition_t * ret = phantom_create_partition_struct( 0, 0, size );

    ret->asyncIo = memdisk_AsyncIo;
    ret->flags |= PART_FLAG_IS_WHOLE_DISK;

    ret->specific = dev;
    strlcpy( ret->name, "MEMD0", sizeof(ret->name) );

    ret->name[4] += unit;

    return ret;
}


static void memdisk_connect( phantom_device_t *dev, int nSect )
{
    phantom_disk_partition_t *part = phantom_create_memdisk_partition_struct( dev, nSect, 0 );
    if(part == 0)
    {
        SHOW_ERROR0( 0, "Failed to create whole disk partition" );
        return;
    }
#if 1
    errno_t err = phantom_register_disk_drive(part);
    if(err)
    {
        SHOW_ERROR( 0, "Disk %p err %d", dev, err );
        return;
    }
#endif
}










