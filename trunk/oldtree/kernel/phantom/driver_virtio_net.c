/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * VirtIo Net driver. Doesnt work yet.
 *
 *
**/

#include <kernel/config.h>

#if HAVE_NET


#define DEBUG_MSG_PREFIX "VirtIo.Net"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>
#include <kernel/vm.h>

#include "i386/pci.h"
#include "virtio.h"
#include <virtio_pci.h>
#include <virtio_net.h>

#include "driver_map.h"
#include "device.h"
#include <x86/phantom_page.h>

#define DEBUG 0

#if DEBUG
#  define WW() getchar()
#else
#  define WW()
#endif


int driver_virtio_net_write(virtio_device_t *vd, void *data, size_t len);
static void provide_buffers(virtio_device_t *vd);




static virtio_device_t vdev;

static int seq_number = 0;

static void driver_virtio_net_interrupt(virtio_device_t *me, int isr )
{
    (void) me;
    (void) isr;

    SHOW_FLOW0( 4, "got virtio net interrupt");

}

phantom_device_t *driver_virtio_net_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    if(vdev.pci)
    {
        SHOW_ERROR0( 0, "Just one drv instance yet");
        return 0;
    }

    vdev.interrupt = driver_virtio_net_interrupt;
    vdev.name = "Net";

    //vdev.guest_features = VIRTIO_NET_F_MAC; // Does not work on QEMU
    vdev.guest_features = 0;

    if( virtio_probe( &vdev, pci ) )
        return 0;

    u_int8_t status = virtio_get_status( &vdev ); //inb(basereg+VIRTIO_PCI_STATUS);
    SHOW_INFO( 0, "Status is: 0x%X\n", status );

    /* driver is ready */
    virtio_set_status( &vdev, VIRTIO_CONFIG_S_ACKNOWLEDGE );

    SHOW_INFO( 0, "Status is: 0x%X\n", virtio_get_status( &vdev ) );






    SHOW_INFO( 1, "Host features are: 0x%b\n", vdev.host_features, "\020\1CSUM\2GUEST_CSUM\6MAC\7GSO\x8GUEST_TSO4\x9GUEST_TSO6\xaGUEST_ECN\xbGUEST_UFO\xcHOST_TSO4\xdHOST_TSO6\xeHOST_ECN\xfHOST_UFO\x10MRG_RXBUF" );

    struct virtio_net_config cfg;
    virtio_get_config_struct( &vdev, &cfg, sizeof(cfg) );

    /* driver is ready */
    //virtio_set_status( &vdev, VIRTIO_CONFIG_S_ACKNOWLEDGE );


    // QEMU MAC is 52:54:00:12:34:56...
    if(vdev.host_features & (1 << VIRTIO_NET_F_MAC))
    {
        SHOW_INFO( 0, "MAC address %02x:%02x:%02x:%02x:%02x:%02x",
                   cfg.mac[0], cfg.mac[1],
                   cfg.mac[2], cfg.mac[3],
                   cfg.mac[4], cfg.mac[5] );
    }


    SHOW_INFO( 1, "Registered at IRQ %d, IO 0x%X\n", vdev.irq, vdev.basereg );

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = "VirtIO Network";
    dev->seq_number = seq_number++;
    dev->drv_private = &vdev;


    vdev.guest_features  = vdev.host_features & (1 << VIRTIO_NET_F_MAC);
    virtio_set_features( &vdev, vdev.guest_features );


    /* driver is ready */
    virtio_set_status( &vdev,  VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER );

    SHOW_INFO( 0, "Status is: 0x%X\n", virtio_get_status( &vdev ) );
    hal_sleep_msec(10);

    /* driver is ready */
    virtio_set_status( &vdev,  VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER | VIRTIO_CONFIG_S_DRIVER_OK);
    SHOW_INFO( 0, "Status is: 0x%X\n", virtio_get_status( &vdev ) );

    provide_buffers(&vdev);

#if 0
    SHOW_FLOW0( 5, "Write to net" );
    static char buf[1500] = "Hello world";

    driver_virtio_net_write( &vdev, buf, sizeof(buf) );
#endif

    return dev;
}

int driver_virtio_net_write(virtio_device_t *vd, void *data, size_t len)
{
    struct vring_desc wr[2];

    // NB - crashes in this case!
    //struct virtio_net_hdr *tx_virtio_hdr = (void *)calloc(1, sizeof(struct virtio_net_hdr));

    struct virtio_net_hdr *tx_virtio_hdr;
    physaddr_t pa;

    hal_pv_alloc( &pa, (void **)&tx_virtio_hdr, sizeof(struct virtio_net_hdr) );

    tx_virtio_hdr->flags = 0;
    tx_virtio_hdr->csum_offset = 0;
    tx_virtio_hdr->csum_start = 0;
    tx_virtio_hdr->gso_type = VIRTIO_NET_HDR_GSO_NONE;
    tx_virtio_hdr->gso_size = 0;
    tx_virtio_hdr->hdr_len = 0;

    //wr[0].addr = kvtophys(tx_virtio_hdr);
    wr[0].addr = pa;
    wr[0].len  = sizeof(*tx_virtio_hdr);
    wr[0].flags = 0;

    wr[1].addr = kvtophys(data);
    wr[1].len  = len;
    wr[1].flags = 0;

    virtio_attach_buffers_list( vd, 0, 2, wr );
    virtio_kick( vd, 0);

    return 0;
}

static void provide_buffers(virtio_device_t *vd)
{
	struct vring_desc rd[2];

	physaddr_t	pa;
	assert( 0 == hal_alloc_phys_page(&pa));

	SHOW_FLOW( 9, "pa = %p", pa );

    rd[0].addr = pa;
    rd[0].len  = sizeof(struct virtio_net_hdr);
    rd[0].flags = 0;

    rd[1].addr = pa + sizeof(struct virtio_net_hdr);
    rd[1].len  = PAGE_SIZE - sizeof(struct virtio_net_hdr);
    rd[1].flags = VRING_DESC_F_WRITE;

    virtio_attach_buffers_list( vd, 1, 2, rd );

	virtio_kick( vd, 0);

}




#if 0



/* virtio-net.c - etherboot driver for virtio network interface
 *
 * (c) Copyright 2008 Bull S.A.S.
 *
 *  Author: Laurent Vivier <Laurent.Vivier@bull.net>
 *
 * some parts from Linux Virtio PCI driver
 *
 *  Copyright IBM Corp. 2007
 *  Authors: Anthony Liguori  <aliguori@us.ibm.com>
 *
 *  some parts from Linux Virtio Ring
 *
 *  Copyright Rusty Russell IBM Corporation 2007
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 *
 */

#include "etherboot.h"
#include "nic.h"
#include "gpxe/virtio-ring.h"
#include "gpxe/virtio-pci.h"
#include "virtio-net.h"

#define BUG() do { \
   printf("BUG: failure at %s:%d/%s()!\n", \
          __FILE__, __LINE__, __FUNCTION__); \
   while(1); \
} while (0)
#define BUG_ON(condition) do { if (condition) BUG(); } while (0)

/* Ethernet header */

struct eth_hdr {
   unsigned char dst_addr[ETH_ALEN];
   unsigned char src_addr[ETH_ALEN];
   unsigned short type;
};

struct eth_frame {
   struct eth_hdr hdr;
   unsigned char data[ETH_FRAME_LEN];
};

/* TX: virtio header and eth buffer */

static struct virtio_net_hdr tx_virtio_hdr;
static struct eth_frame tx_eth_frame;

/* RX: virtio headers and buffers */

#define RX_BUF_NB  6
static struct virtio_net_hdr rx_hdr[RX_BUF_NB];
static unsigned char rx_buffer[RX_BUF_NB][ETH_FRAME_LEN];

/* virtio queues and vrings */

enum {
   RX_INDEX = 0,
   TX_INDEX,
   QUEUE_NB
};

static struct vring_virtqueue virtqueue[QUEUE_NB];

/*
 * virtnet_disable
 *
 * Turn off ethernet interface
 *
 */

static void virtnet_disable(struct nic *nic)
{
   int i;

   for (i = 0; i < QUEUE_NB; i++) {
           vring_disable_cb(&virtqueue[i]);
           vp_del_vq(nic->ioaddr, i);
   }
   vp_reset(nic->ioaddr);
}

/*
 * virtnet_poll
 *
 * Wait for a frame
 *
 * return true if there is a packet ready to read
 *
 * nic->packet should contain data on return
 * nic->packetlen should contain length of data
 *
 */
static int virtnet_poll(struct nic *nic, int retrieve)
{
   unsigned int len;
   u16 token;
   struct virtio_net_hdr *hdr;
   struct vring_list list[2];

   if (!vring_more_used(&virtqueue[RX_INDEX]))
           return 0;

   if (!retrieve)
           return 1;

   token = vring_get_buf(&virtqueue[RX_INDEX], &len);

   BUG_ON(len > sizeof(struct virtio_net_hdr) + ETH_FRAME_LEN);

   hdr = &rx_hdr[token];   /* FIXME: check flags */
   len -= sizeof(struct virtio_net_hdr);

   nic->packetlen = len;
   memcpy(nic->packet, (char *)rx_buffer[token], nic->packetlen);

   /* add buffer to desc */

   list[0].addr = (char*)&rx_hdr[token];
   list[0].length = sizeof(struct virtio_net_hdr);
   list[1].addr = (char*)&rx_buffer[token];
   list[1].length = ETH_FRAME_LEN;

   vring_add_buf(&virtqueue[RX_INDEX], list, 0, 2, token, 0);
   vring_kick(nic->ioaddr, &virtqueue[RX_INDEX], 1);

   return 1;
}

/*
 *
 * virtnet_transmit
 *
 * Transmit a frame
 *
 */

static void virtnet_transmit(struct nic *nic, const char *destaddr,
        unsigned int type, unsigned int len, const char *data)
{
   struct vring_list list[2];

   /*
    * from http://www.etherboot.org/wiki/dev/devmanual :
    *     "You do not need more than one transmit buffer."
    */

   /* FIXME: initialize header according to vp_get_features() */

   tx_virtio_hdr.flags = 0;
   tx_virtio_hdr.csum_offset = 0;
   tx_virtio_hdr.csum_start = 0;
   tx_virtio_hdr.gso_type = VIRTIO_NET_HDR_GSO_NONE;
   tx_virtio_hdr.gso_size = 0;
   tx_virtio_hdr.hdr_len = 0;

   /* add ethernet frame into vring */

   BUG_ON(len > sizeof(tx_eth_frame.data));

   memcpy(tx_eth_frame.hdr.dst_addr, destaddr, ETH_ALEN);
   memcpy(tx_eth_frame.hdr.src_addr, nic->node_addr, ETH_ALEN);
   tx_eth_frame.hdr.type = htons(type);
   memcpy(tx_eth_frame.data, data, len);

   list[0].addr = (char*)&tx_virtio_hdr;
   list[0].length = sizeof(struct virtio_net_hdr);
   list[1].addr = (char*)&tx_eth_frame;
   list[1].length = ETH_FRAME_LEN;

   vring_add_buf(&virtqueue[TX_INDEX], list, 2, 0, 0, 0);

   vring_kick(nic->ioaddr, &virtqueue[TX_INDEX], 1);

   /*
    * http://www.etherboot.org/wiki/dev/devmanual
    *
    *   "You should ensure the packet is fully transmitted
    *    before returning from this routine"
    */

   while (!vring_more_used(&virtqueue[TX_INDEX])) {
           mb();
           udelay(10);
   }

   /* free desc */

   (void)vring_get_buf(&virtqueue[TX_INDEX], NULL);
}

static void virtnet_irq(struct nic *nic __unused, irq_action_t action)
{
   switch ( action ) {
   case DISABLE :
           vring_disable_cb(&virtqueue[RX_INDEX]);
           vring_disable_cb(&virtqueue[TX_INDEX]);
           break;
   case ENABLE :
           vring_enable_cb(&virtqueue[RX_INDEX]);
           vring_enable_cb(&virtqueue[TX_INDEX]);
           break;
   case FORCE :
           break;
   }
}

static void provide_buffers(struct nic *nic)
{
   int i;
   struct vring_list list[2];

   for (i = 0; i < RX_BUF_NB; i++) {
           list[0].addr = (char*)&rx_hdr[i];
           list[0].length = sizeof(struct virtio_net_hdr);
           list[1].addr = (char*)&rx_buffer[i];
           list[1].length = ETH_FRAME_LEN;
           vring_add_buf(&virtqueue[RX_INDEX], list, 0, 2, i, i);
   }

   /* nofify */

   vring_kick(nic->ioaddr, &virtqueue[RX_INDEX], i);
}

static struct nic_operations virtnet_operations = {
        .connect = dummy_connect,
        .poll = virtnet_poll,
        .transmit = virtnet_transmit,
        .irq = virtnet_irq,
};

/*
 * virtnet_probe
 *
 * Look for a virtio network adapter
 *
 */

static int virtnet_probe(struct nic *nic, struct pci_device *pci)
{
   u32 features;
   int i;

   /* Mask the bit that says "this is an io addr" */

   nic->ioaddr = pci->ioaddr & ~3;

   /* Copy IRQ from PCI information */

   nic->irqno = pci->irq;

   printf("I/O address 0x%08x, IRQ #%d\n", nic->ioaddr, nic->irqno);

   adjust_pci_device(pci);

   vp_reset(nic->ioaddr);

   features = vp_get_features(nic->ioaddr);
   if (features & (1 << VIRTIO_NET_F_MAC)) {
           vp_get(nic->ioaddr, offsetof(struct virtio_net_config, mac),
                  nic->node_addr, ETH_ALEN);
           printf("MAC address ");
           for (i = 0; i < ETH_ALEN; i++) {
                   printf("%02x%c", nic->node_addr[i],
                          (i == ETH_ALEN - 1) ? '\n' : ':');
           }
   }

   /* initialize emit/receive queue */

   for (i = 0; i < QUEUE_NB; i++) {
           virtqueue[i].free_head = 0;
           virtqueue[i].last_used_idx = 0;
           memset((char*)&virtqueue[i].queue, 0, sizeof(virtqueue[i].queue));
           if (vp_find_vq(nic->ioaddr, i, &virtqueue[i]) == -1)
                   printf("Cannot register queue #%d\n", i);
   }

   /* provide some receive buffers */

    provide_buffers(nic);

   /* define NIC interface */

    nic->nic_op = &virtnet_operations;

   /* driver is ready */

   vp_set_features(nic->ioaddr, features & (1 << VIRTIO_NET_F_MAC));
   vp_set_status(nic->ioaddr, VIRTIO_CONFIG_S_DRIVER | VIRTIO_CONFIG_S_DRIVER_OK);

   return 1;
}

static struct pci_device_id virtnet_nics[] = {
PCI_ROM(0x1af4, 0x1000, "virtio-net",              "Virtio Network Interface", 0),
};

PCI_DRIVER ( virtnet_driver, virtnet_nics, PCI_NO_CLASS );

DRIVER ( "VIRTIO-NET", nic_driver, pci_driver, virtnet_driver,
         virtnet_probe, virtnet_disable );


#endif

#endif // HAVE_NET

