/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * VirtIO disk driver. Not finished, kills world.
 *
**/

#define DEBUG_MSG_PREFIX "VioDisk"
#include "debug_ext.h"
#define debug_level_info 10
#define debug_level_flow 0
#define debug_level_error 10

#include <phantom_libc.h>
#include <kernel/vm.h>

#include "i386/pci.h"
#include "virtio.h"
#include <virtio_pci.h>
#include <virtio_blk.h>
#include <pager_io_req.h>

#include <sys/cdefs.h>


#include "driver_map.h"
#include "device.h"


//static short basereg;
//static int irq;
static int rodisk;

static virtio_device_t vdev;

static int seq_number = 0;


static void driver_virtio_disk_interrupt(virtio_device_t *vd, int isr );
int driver_virtio_disk_rq(virtio_device_t *vd, pager_io_request *rq);

int driver_virtio_disk_write(virtio_device_t *vd, physaddr_t data, size_t len);


phantom_device_t *driver_virtio_disk_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    if(vdev.pci)
    {
        printf("Just one drv instance yet\n");
        return 0;
    }

    vdev.interrupt = driver_virtio_disk_interrupt;
    vdev.name = "Disk";

    // Say we need it. Not sure, really, that we do. :)
    vdev.guest_features = VIRTIO_BLK_F_BARRIER;

    if( virtio_probe( &vdev, pci ) )
        return 0;

    //u_int8_t status = virtio_get_status( &vdev );
    //printf("Status is: 0x%x\n", status );


    printf("Features are: %b\n", vdev.host_features, "\020\1BARRIER\2SIZE_MAX\3SEG_MAX\5GEOM\6RDONLY\7BLK_SIZE" );

    rodisk = vdev.host_features & (1<<VIRTIO_BLK_F_RO);
    if(rodisk)
        printf("Disk is RDONLY\n");


    printf("Registered at IRQ %d IO 0x%X\n", vdev.irq, vdev.basereg );

    phantom_device_t * dev = (phantom_device_t *)malloc(sizeof(phantom_device_t));
    dev->name = "VirtIO Disk";
    dev->seq_number = seq_number++;
    dev->drv_private = &vdev;

    virtio_set_status( &vdev, VIRTIO_CONFIG_S_DRIVER );

    struct virtio_blk_config cfg;
    virtio_get_config_struct( &vdev, &cfg, sizeof(cfg) );

    printf("VIRTIO disk size is %d Mb\n", cfg.capacity/2048 );

    virtio_set_status( &vdev, VIRTIO_CONFIG_S_DRIVER|VIRTIO_CONFIG_S_DRIVER_OK );

#if 0
    printf("Will write to disk\n");
//getchar();
    static char test[512] = "Hello virtio disk";

    physaddr_t pa;
    void *va;
    hal_pv_alloc( &pa, &va, sizeof(test) );

    strlcpy( va, test, sizeof(test) );

    driver_virtio_disk_write( &vdev, pa, sizeof(test) );
    printf("Write to disk requested\n");
//getchar();
#endif

    return dev;
}



struct vioBlockReq
{
    struct virtio_blk_outhdr 	ohdr;
    struct virtio_blk_inhdr  	ihdr;
    pager_io_request            *rq;
} __packed;

// virtio-blk header not in correct element

int driver_virtio_disk_write(virtio_device_t *vd, physaddr_t data, size_t len)
{

    struct vring_desc cmd[3];

    struct vioBlockReq          *vreq; // = (void *)calloc(1, sizeof(struct vioBlockReq));

    physaddr_t pa;
    hal_pv_alloc( &pa, (void **)&vreq, sizeof(struct vioBlockReq) );


    struct virtio_blk_outhdr *ohdr = &(vreq->ohdr);
    //struct virtio_blk_inhdr  *ihdr = &(vreq->ihdr);
    vreq->rq = 0;

    ohdr->type = VIRTIO_BLK_T_OUT;
    ohdr->ioprio = 0;
    ohdr->sector = 0;

    cmd[2].addr = pa + __offsetof(struct vioBlockReq, ohdr);
    cmd[2].len  = sizeof(struct virtio_blk_outhdr);
    cmd[2].flags= 0;

    cmd[1].addr = data;
    cmd[1].len  = len;
    cmd[1].flags= 0;

#if 0
    cmd[2].addr = pa + __offsetof(struct vioBlockReq, ihdr);
    cmd[2].len  = sizeof(struct virtio_blk_inhdr);
    cmd[2].flags= VRING_DESC_F_WRITE;
#else
    physaddr_t pa2;
    void *va2;
    hal_pv_alloc( &pa2, &va2, sizeof(struct virtio_blk_inhdr) );

    cmd[0].addr = pa2;
    cmd[0].len  = sizeof(struct virtio_blk_inhdr);
    cmd[0].flags= VRING_DESC_F_WRITE;
#endif

    virtio_attach_buffers_list( vd, 0, 3, cmd );
    virtio_kick( vd, 0);

    return 0;
}

int driver_virtio_disk_read(virtio_device_t *vd, physaddr_t data, size_t len)
{
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

    virtio_attach_buffers_list( vd, 0, 3, cmd );
    virtio_kick( vd, 0);

    return 0;
}









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








static void driver_virtio_disk_interrupt(virtio_device_t *vd, int isr )
{
    (void) isr;

    SHOW_FLOW0( 5, "got virtio DISK interrupt" );

    struct vring_desc cmd[3];
    int dlen;

    int nRead = virtio_detach_buffers_list( vd, 0, 3, cmd, &dlen );
    if( nRead > 0 )
    {
        SHOW_FLOW( 5, "have %d used descriptors, will get 'em", nRead );

        int i;
        for( i = 0; i < nRead; i++ )
            SHOW_FLOW( 6, "desc %d len %d\n", i, cmd[i].len );

    }
}


