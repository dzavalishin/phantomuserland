#include <phantom_libc.h>

#include "i386/pci.h"
#include "virtio.h"
#include <virtio_pci.h>
#include <virtio_rng.h>

#include "driver_map.h"
#include "device.h"


//static short basereg;
//static int irq;

static virtio_device_t vdev;

static int seq_number = 0;

static hal_mutex_t access_mutex;
static hal_cond_t wait_4_data;


static void driver_virtio_random_interrupt(virtio_device_t *me, int isr )
{
    (void) me;
    (void) isr;

    printf("got virtio random interrupt\n");

    hal_mutex_lock( &access_mutex );
    hal_mutex_unlock( &access_mutex );
}

phantom_device_t *driver_virtio_random_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    if(vdev.pci)
    {
        printf("Just one virtio random drv instance is possible!\n");
        return 0;
    }

    assert(!hal_mutex_init(&access_mutex, "VirtRND"));
    assert(!hal_cond_init(&wait_4_data, "VirtRND"));

    vdev.interrupt = driver_virtio_random_interrupt;


    if( virtio_probe( &vdev, pci ) )
        return 0;

    //u_int8_t status = virtio_get_status( &vdev ); //inb(basereg+VIRTIO_PCI_STATUS);




    printf("Registered at IRQ %d IO 0x%X\n", vdev.irq, vdev.basereg );

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = "VirtIO Random";
    dev->seq_number = seq_number++;

    return dev;
}
