#include <phantom_libc.h>
#include <kernel/vm.h>

#include "i386/pci.h"
#include "virtio.h"
#include <virtio_pci.h>
#include <virtio_blk.h>

#include "driver_map.h"
#include "device.h"


//static short basereg;
//static int irq;
static int rodisk;

static virtio_device_t vdev;

static int seq_number = 0;

static void driver_virtio_disk_interrupt(virtio_device_t *me, int isr )
{
    printf("got virtio interrupt\n");

}

int driver_virtio_disk_write(virtio_device_t *vd, physaddr_t data, size_t len);


phantom_device_t *driver_virtio_disk_probe( pci_cfg_t *pci, int stage )
{

    if(vdev.pci)
    {
        printf("Just one drv instance yet\n");
        return 0;
    }

    vdev.interrupt = driver_virtio_disk_interrupt;

    // Say we need it. Not sure, really, that we do. :)
    vdev.guest_features = VIRTIO_BLK_F_BARRIER;

    if( virtio_probe( &vdev, pci ) )
        return 0;

    u_int8_t status = virtio_get_status( &vdev ); //inb(basereg+VIRTIO_PCI_STATUS);
    printf("Status is: 0x%x\n", status );


    printf("Features are: %b\n", vdev.host_features, "\020\0BARRIER\1SIZE_MAX\2SEG_MAX\4GEOM\5RDONLY\6BLK_SIZE" );

    rodisk = vdev.host_features & (1<<VIRTIO_BLK_F_RO);
    if(rodisk)
        printf("Disk is RDONLY\n");


    printf("Registered at IRQ %d IO 0x%X\n", vdev.irq, vdev.basereg );

    phantom_device_t * dev = (phantom_device_t *)malloc(sizeof(phantom_device_t));
    dev->name = "VirtIO Disk";
    dev->seq_number = seq_number++;

    virtio_set_status( &vdev, VIRTIO_CONFIG_S_DRIVER );

    struct virtio_blk_config cfg;
    virtio_get_config_struct( &vdev, &cfg, sizeof(cfg) );

    printf("VIRTIO disk size is %d Mb\n", cfg.capacity/2048 );

//    printf("Will send VIRTIO_CONFIG_S_DRIVER_OK\n");
//getchar();
    virtio_set_status( &vdev, VIRTIO_CONFIG_S_DRIVER|VIRTIO_CONFIG_S_DRIVER_OK );

#if 0
    printf("Will write to disk\n");
getchar();
    char test[512] = "Hello virtio disk";
    driver_virtio_disk_write( &vdev, kvtophys(test), 512 );
    printf("Write to disk requested\n");
getchar();
#endif

    return dev;
}



int driver_virtio_disk_write(virtio_device_t *vd, physaddr_t data, size_t len)
{
    struct vring_desc wr[2];
    struct vring_desc rd[1];

    struct virtio_blk_outhdr *ohdr = (void *)calloc(1, sizeof(struct virtio_blk_outhdr));
    struct virtio_blk_inhdr * ihdr = (void *)calloc(1, sizeof(struct virtio_blk_inhdr));

    ohdr->type = VIRTIO_BLK_T_OUT;
    ohdr->ioprio = 0;
    ohdr->sector = 0;

    // TODO must be 0th!
    wr[1].addr = kvtophys(ohdr);
    wr[1].len  = sizeof(*ohdr);

    rd[0].addr = kvtophys(ihdr);
    rd[0].len  = sizeof(*ihdr);

    wr[0].addr = data;
    wr[0].len  = len;

    virtio_attach_buffers( vd, 0,
                          2, wr,
                          1, rd
                          );

    virtio_kick( vd, 0);

    return 0;
}

