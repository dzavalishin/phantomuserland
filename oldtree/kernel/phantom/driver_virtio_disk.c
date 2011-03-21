#ifdef ARCH_ia32
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * VirtIO disk driver. Not finished, kills world.
 *
**/

#define DEBUG_MSG_PREFIX "VioDisk"
#include "debug_ext.h"
#define debug_level_info 10
#define debug_level_flow 10
#define debug_level_error 10

#include <phantom_libc.h>
#include <kernel/vm.h>

#include "i386/pci.h"
#include "virtio.h"
#include <virtio_pci.h>
#include <virtio_blk.h>

#include <pager_io_req.h>
#include <disk.h>

#include <sys/cdefs.h>


//#include "driver_map.h"
//#include "device.h"
#include <device.h>
#include <kernel/drivers.h>


//static short basereg;
//static int irq;
static int rodisk;

static virtio_device_t vdev;

static int seq_number = 0;



#define MAXREQ 64
#define REQ_MAGIC 0xAAEEF0EE


static  struct vioBlockReq * getReq(void);
static  void putReq(struct vioBlockReq *req);





static void driver_virtio_disk_interrupt(virtio_device_t *vd, int isr );
int driver_virtio_disk_rq(virtio_device_t *vd, pager_io_request *rq);

static void driver_virtio_disk_write(virtio_device_t *vd, physaddr_t data, size_t len, pager_io_request *rq, int sect);
static void driver_virtio_disk_read(virtio_device_t *vd, physaddr_t data, size_t len, pager_io_request *rq, int sect);

static phantom_disk_partition_t *phantom_create_virtio_partition_struct( long size, virtio_device_t *vd );




phantom_device_t *driver_virtio_disk_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    if(vdev.pci)
    {
        printf("Just one drv instance yet\n");
        return 0;
    }

    vdev.interrupt = driver_virtio_disk_interrupt;
    vdev.name = "VirtIODisk0";

    // Say we need it. Not sure, really, that we do. :)
    vdev.guest_features = VIRTIO_BLK_F_BARRIER;

    if( virtio_probe( &vdev, pci ) )
        return 0;

    //u_int8_t status = virtio_get_status( &vdev );
    //printf("Status is: 0x%x\n", status );


    SHOW_FLOW( 1, "Features are: %b", vdev.host_features, "\020\1BARRIER\2SIZE_MAX\3SEG_MAX\5GEOM\6RDONLY\7BLK_SIZE" );

    rodisk = vdev.host_features & (1<<VIRTIO_BLK_F_RO);
    if(rodisk)
        SHOW_FLOW0( 1, "Disk is RDONLY");


    SHOW_FLOW( 1, "Registered at IRQ %d IO 0x%X", vdev.irq, vdev.basereg );

    phantom_device_t * dev = (phantom_device_t *)malloc(sizeof(phantom_device_t));
    dev->name = "VirtIO Disk";
    dev->seq_number = seq_number++;
    dev->drv_private = &vdev;




    virtio_set_status( &vdev, VIRTIO_CONFIG_S_DRIVER );

    struct virtio_blk_config cfg;
    virtio_get_config_struct( &vdev, &cfg, sizeof(cfg) );

    SHOW_FLOW( 1, "VIRTIO disk size is %d Mb", cfg.capacity/2048 );

    virtio_set_status( &vdev, VIRTIO_CONFIG_S_DRIVER|VIRTIO_CONFIG_S_DRIVER_OK );

#if 0
    printf("Will write to disk\n");
//getchar();
    static char test[512] = "Hello virtio disk";

    physaddr_t pa;
    void *va;
    hal_pv_alloc( &pa, &va, sizeof(test) );

    strlcpy( va, test, sizeof(test) );

    driver_virtio_disk_write( &vdev, pa, sizeof(test), 0, 0 );
    printf("Write to disk requested\n");
//getchar();
#endif

    phantom_disk_partition_t *p = phantom_create_virtio_partition_struct( cfg.capacity, &vdev );
    (void) p;

#if 0
    errno_t ret = phantom_register_disk_drive(p);
    if( ret )
        SHOW_ERROR( 0, "Can't register VirtIO drive: %d", ret );
#endif

    return dev;
}



struct vioBlockReq
{
    struct virtio_blk_outhdr 	ohdr;
    struct virtio_blk_inhdr  	ihdr;
    pager_io_request            *rq;

    int                         used; // !0 == this record is passed to driver and didn't get back yet
    unsigned int                magic;

} __packed;

// virtio-blk header not in correct element

//static char *va2;

void driver_virtio_disk_write(virtio_device_t *vd, physaddr_t data, size_t len, pager_io_request *rq, int sect)
{

    struct vring_desc cmd[3];

    struct vioBlockReq          *vreq; // = (void *)calloc(1, sizeof(struct vioBlockReq));
    physaddr_t pa;

    //hal_pv_alloc( &pa, (void **)&vreq, sizeof(struct vioBlockReq) );
    vreq = getReq();
    pa = kvtophys(vreq);


    struct virtio_blk_outhdr *ohdr = &(vreq->ohdr);
    //struct virtio_blk_inhdr  *ihdr = &(vreq->ihdr);
    vreq->rq = rq;
    vreq->ihdr.status = 0;

    ohdr->type = VIRTIO_BLK_T_OUT;
    ohdr->ioprio = 0;
    ohdr->sector = sect;

    cmd[0].addr = pa + __offsetof(struct vioBlockReq, ohdr);
    cmd[0].len  = sizeof(struct virtio_blk_outhdr);
    cmd[0].flags= 0;

    cmd[1].addr = data;
    cmd[1].len  = len;
    cmd[1].flags= 0;

#if 1
    cmd[2].addr = pa + __offsetof(struct vioBlockReq, ihdr);
    cmd[2].len  = sizeof(struct virtio_blk_inhdr);
    cmd[2].flags= VRING_DESC_F_WRITE;
#else
    physaddr_t pa2;
    hal_pv_alloc( &pa2, (void **)&va2, sizeof(struct virtio_blk_inhdr) );

    cmd[2].addr = pa2;
    cmd[2].len  = sizeof(struct virtio_blk_inhdr);
    cmd[2].flags= VRING_DESC_F_WRITE;

    *va2 = 0;
    SHOW_FLOW( 5, "disk result va2 = %d", *va2);
#endif

    virtio_attach_buffers_list( vd, 0, 3, cmd );
    virtio_kick( vd, 0);

}


void driver_virtio_disk_read(virtio_device_t *vd, physaddr_t data, size_t len, pager_io_request *rq, int sect)
{
#if 1
    struct vring_desc cmd[3];

    struct vioBlockReq          *vreq; // = (void *)calloc(1, sizeof(struct vioBlockReq));
    physaddr_t pa;

    vreq = getReq();
    pa = kvtophys(vreq);

    struct virtio_blk_outhdr *ohdr = &(vreq->ohdr);

    vreq->rq = rq;
    vreq->ihdr.status = 0;

    ohdr->type = VIRTIO_BLK_T_IN;
    ohdr->ioprio = 0;
    ohdr->sector = sect;

    cmd[0].addr = pa + __offsetof(struct vioBlockReq, ohdr);
    cmd[0].len  = sizeof(struct virtio_blk_outhdr);
    cmd[0].flags= 0;

    cmd[1].addr = data;
    cmd[1].len  = len;
    cmd[1].flags= VRING_DESC_F_WRITE;

    cmd[2].addr = pa + __offsetof(struct vioBlockReq, ihdr);
    cmd[2].len  = sizeof(struct virtio_blk_inhdr);
    cmd[2].flags= VRING_DESC_F_WRITE;
#else
    struct vring_desc cmd[3];

    struct vioBlockReq          *vreq = (void *)calloc(1, sizeof(struct vioBlockReq));

    struct virtio_blk_outhdr *ohdr = &(vreq->ohdr);
    struct virtio_blk_inhdr  *ihdr = &(vreq->ihdr);
    vreq->rq = 0;

    ohdr->type = VIRTIO_BLK_T_IN;
    ohdr->ioprio = 0;
    ohdr->sector = 0;

    cmd[0].addr = kvtophys(ohdr);
    cmd[0].len  = sizeof(*ohdr);
    cmd[0].flags= 0;

    cmd[1].addr = data;
    cmd[1].len  = len;
    cmd[1].flags= VRING_DESC_F_WRITE;

    cmd[2].addr = kvtophys(ihdr);
    cmd[2].len  = sizeof(*ihdr);
    cmd[2].flags= VRING_DESC_F_WRITE;

#endif

    virtio_attach_buffers_list( vd, 0, 3, cmd );
    virtio_kick( vd, 0);
}









/*
int driver_virtio_disk_rq(virtio_device_t *vd, pager_io_request *rq)
{
    struct vring_desc cmd[3];

    struct vioBlockReq          *vreq = (void *)calloc(1, sizeof(struct vioBlockReq));

    struct virtio_blk_outhdr *ohdr = &(vreq->ohdr);
    struct virtio_blk_inhdr  *ihdr = &(vreq->ihdr);
    vreq->rq = rq;

    ohdr->type = rq->flag_pagein ? VIRTIO_BLK_T_IN : VIRTIO_BLK_T_OUT;
    ohdr->ioprio = rq->flag_pagein ? 1 : 0; // Some small preferance to read
    ohdr->sector = rq->blockNo;

    cmd[0].addr = kvtophys(ohdr);
    cmd[0].len  = sizeof(*ohdr);
    cmd[0].flags= 0;

    cmd[1].addr = rq->phys_page;
    cmd[1].len  = rq->nSect*512; // TODO const sector size
    cmd[1].flags= rq->flag_pagein ? VRING_DESC_F_WRITE : 0;

    cmd[2].addr = kvtophys(ihdr);
    cmd[2].len  = sizeof(*ihdr);
    cmd[2].flags= VRING_DESC_F_WRITE;

    virtio_attach_buffers_list( vd, 0, 3, cmd );
    virtio_kick( vd, 0);

    return 0;
}
*/







static void driver_virtio_disk_interrupt(virtio_device_t *vd, int isr )
{
    (void) isr;

    //SHOW_FLOW0( 5, "got virtio DISK interrupt" );

    struct vring_desc cmd[10];
    int dlen;

    int nRead = virtio_detach_buffers_list( vd, 0, 10, cmd, &dlen );
    if( nRead <= 0 )
        return;

#if 0
    SHOW_FLOW( 5, "have %d used descriptors, will get 'em (dlen = %d)", nRead, dlen );

    int i;
    for( i = 0; i < nRead; i++ )
        SHOW_FLOW( 6, "desc %2d len %4d   next %6d flags %b", i, cmd[i].len, cmd[i].next, cmd[i].flags, "\020\1NEXT\2WRITE" );
#endif

    if(nRead != 3)
        SHOW_ERROR( 0, "nRead = %d", nRead );

    if(cmd[0].len != sizeof(struct virtio_blk_outhdr))
    {
        SHOW_ERROR( 0, "cmd[0].len = %d, expected %d", cmd[0].len, sizeof(struct virtio_blk_outhdr) );
        return;
    }

    // FIXME 64 bit error
    // p = v
    struct vioBlockReq *req = (void*)(int)cmd[0].addr;

    assert( req->magic == REQ_MAGIC );

    pager_io_request            *rq = req->rq;

    if(rq)
    {
        rq->flag_ioerror = req->ihdr.status ? (~0) : 0;

        switch(req->ihdr.status)
        {
        case 0:            rq->rc = 0; break;

        case VIRTIO_BLK_S_UNSUPP:
            rq->rc = EINVAL;
            break;

        case VIRTIO_BLK_S_IOERR:
        default:           rq->rc = EIO; break;
        }

        pager_io_request_done( rq );
    }

    putReq(req);

    //SHOW_FLOW( 5, "disk result va2 = %d", *va2);
}












static  struct vioBlockReq      rlist[MAXREQ];
static  int                     rpos = 0;
static  int                     rfree = MAXREQ;

static  hal_mutex_t             rmutex;
//static  hal_cond_t              rcond;

static  struct vioBlockReq * getReq(void)
{
    struct vioBlockReq * ret;

    hal_mutex_lock( &rmutex );

    while(1)
    {
        while(rfree <= 0)
        {
            //hal_cond_wait( &rcond, &rmutex );
            // FIXME This is VERY dumb, but should happen rarely :)
            hal_mutex_unlock( &rmutex );
            hal_sleep_msec(100);
            hal_mutex_lock( &rmutex );
        }

        rpos++;
        if( rpos >= MAXREQ ) rpos = 0;

        if( rlist[rpos].used )
            continue;

        rfree--;
        rlist[rpos].used = 1;
        rlist[rpos].magic = REQ_MAGIC;
        ret = rlist+rpos;
        break;
    }

    hal_mutex_unlock( &rmutex );

    return ret;
}

static  void putReq(struct vioBlockReq *req)
{
    assert( req->magic == REQ_MAGIC );
    req->used = 0;
    rfree++;
    //hal_cond_broadcast( &rcond );
}























errno_t driver_virtio_disk_rq(virtio_device_t *vd, pager_io_request *rq)
{
    int sect = rq->blockNo;
    int n = rq->nSect;
    physaddr_t pa = rq->phys_page;


    while(n--)
    {
        if( rq->flag_pageout )
            driver_virtio_disk_write(vd, pa, 512, rq, sect);
        else
            driver_virtio_disk_read (vd, pa, 512, rq, sect);

        sect++;
        pa += 512;
    }

    return 0;
}




errno_t virtioAsyncIo( struct phantom_disk_partition *p, pager_io_request *rq )
{
    assert(p->specific != 0);

    // Temp! Rewrite!
    //assert(p->base == 0 );

    virtio_device_t *vd = (virtio_device_t *)p->specific;

    return driver_virtio_disk_rq( vd, rq );
}


static phantom_disk_partition_t *phantom_create_virtio_partition_struct( long size, virtio_device_t *vd )
{
    phantom_disk_partition_t * ret = phantom_create_partition_struct( 0, 0, size);

    ret->asyncIo = virtioAsyncIo;
    ret->flags |= PART_FLAG_IS_WHOLE_DISK;

    ret->specific = vd;

    //strlcpy( ret->name, "virtio", PARTITION_NAME_LEN );
    strlcpy( ret->name, vd->name, PARTITION_NAME_LEN );

    return ret;
}





#endif // ARCH_ia32

