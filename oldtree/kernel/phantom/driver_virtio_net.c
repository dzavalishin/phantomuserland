#ifdef ARCH_ia32

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

//#include "driver_map.h"
//#include "device.h"
#include <device.h>
#include <kernel/drivers.h>
#include <kernel/ethernet_defs.h>
#include <kernel/page.h>
#include "net.h"

//#include <x86/phantom_page.h>

// FIXME some races prevent this driver from starting at stage 1



#define WIRED_ADDRESS 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 122)
#define WIRED_NETMASK 	0xffffff00
#define WIRED_BROADCAST IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0xFF)

#define WIRED_NET 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0)
#define WIRED_ROUTER 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 122)

#define DEF_ROUTE_ROUTER        IPV4_DOTADDR_TO_ADDR(10, 0, 2, 2)




// At least this many buffers driver must have in recv queue
#define MIN_RECV_BUF    8


typedef struct vionet
{
    unsigned char	mac_addr[ETH_ALEN];

    int                 thread;
    hal_sem_t           sem;                    // thread wakeup sem

    int                 active;

    int                 recv_buffers_in_driver;      // How many recv buffers driver has on its side

    hal_mutex_t         recv_mutex; // Stops more than one thread to come in read
    hal_cond_t          recv_cond;  // Recv done, read_buf filled
    void                *read_buf;
    size_t              read_max;
    size_t              read_len;

} vionet_t;




int driver_virtio_net_write(virtio_device_t *vd, const void *data, size_t len);
static void driver_virtio_net_interrupt(virtio_device_t *me, int isr );

static void provide_buffers(virtio_device_t *vd);
static void vnet_thread(void *_dev);


static int vnet_stop(phantom_device_t * dev);
static int vnet_write(phantom_device_t * dev, const void *buf, int buflen );
static int vnet_read(phantom_device_t * dev, void *buf, int buflen );
static int vnet_get_address( struct phantom_device *dev, void *buf, int len);






static virtio_device_t vdev;

static int seq_number = 0;


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
    SHOW_INFO( 11, "Status is: 0x%X", status );

    /* driver is ready */
    virtio_set_status( &vdev, VIRTIO_CONFIG_S_ACKNOWLEDGE );

    SHOW_INFO( 11, "Status is: 0x%X", virtio_get_status( &vdev ) );






    SHOW_INFO( 1, "Host features are: 0x%b", vdev.host_features, "\020\1CSUM\2GUEST_CSUM\6MAC\7GSO\x8GUEST_TSO4\x9GUEST_TSO6\xaGUEST_ECN\xbGUEST_UFO\xcHOST_TSO4\xdHOST_TSO6\xeHOST_ECN\xfHOST_UFO\x10MRG_RXBUF" );

    struct virtio_net_config cfg;
    virtio_get_config_struct( &vdev, &cfg, sizeof(cfg) );

    /* driver is ready */
    //virtio_set_status( &vdev, VIRTIO_CONFIG_S_ACKNOWLEDGE );


    vionet_t *vnet = calloc( sizeof(vionet_t), 1 );
    assert(vnet);

    vdev.pvt = vnet;

    // QEMU MAC is 52:54:00:12:34:56...
    if(vdev.host_features & (1 << VIRTIO_NET_F_MAC))
    {
        SHOW_INFO( 0, "MAC address %02x:%02x:%02x:%02x:%02x:%02x",
                   cfg.mac[0], cfg.mac[1],
                   cfg.mac[2], cfg.mac[3],
                   cfg.mac[4], cfg.mac[5] );

        memcpy( vnet->mac_addr, cfg.mac, ETH_ALEN );
    }

    assert( 0 == hal_sem_init( &vnet->sem, "VioNet" ));



    SHOW_INFO( 1, "Registered at IRQ %d, IO 0x%X", vdev.irq, vdev.basereg );

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = "VirtIO Network";
    dev->seq_number = seq_number++;
    dev->drv_private = &vdev;

    dev->dops.stop =  vnet_stop;
    dev->dops.read =  vnet_read;
    dev->dops.write = vnet_write;
    dev->dops.get_address = vnet_get_address;

    vdev.guest_features  = vdev.host_features & (1 << VIRTIO_NET_F_MAC);
    virtio_set_features( &vdev, vdev.guest_features );


    /* driver is ready * /
    virtio_set_status( &vdev,  VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER );

    SHOW_INFO( 0, "Status is: 0x%X", virtio_get_status( &vdev ) );
    hal_sleep_msec(10);
    */

    /* driver is ready */
    virtio_set_status( &vdev,  VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER | VIRTIO_CONFIG_S_DRIVER_OK);
    SHOW_INFO( 11, "Status is: 0x%X", virtio_get_status( &vdev ) );


    hal_mutex_init( &vnet->recv_mutex, "vNetRecv" );
    hal_cond_init( &vnet->recv_cond, "vNetRecv" );
    vnet->read_buf = 0;
    vnet->read_max = 0;

    vnet->thread = hal_start_kernel_thread_arg( vnet_thread, dev );


    vnet->active = 1;

    // To provide initial buffers
    hal_sem_release( &(vnet->sem) );

    //provide_buffers(&vdev);

#if 0
    SHOW_FLOW0( 5, "Write to net" );
    static char buf[1500] = "Hello world";

    driver_virtio_net_write( &vdev, buf, sizeof(buf) );
#endif

    ifnet *interface;
    if( if_register_interface( IF_TYPE_ETHERNET, &interface, dev) )
    {
        SHOW_ERROR( 0, "Failed to register interface for %s", dev->name );
    }
    else
    {
        if_simple_setup( interface, WIRED_ADDRESS, WIRED_NETMASK, WIRED_BROADCAST, WIRED_NET, WIRED_ROUTER, DEF_ROUTE_ROUTER );
    }


    return dev;
}

int driver_virtio_net_write(virtio_device_t *vd, const void *idata, size_t len)
{
    struct vring_desc wr[2];
#if 1
    physaddr_t	pa;
    assert( 0 == hal_alloc_phys_page(&pa));

    SHOW_FLOW( 9, "xmit pa = %p", pa );


    char buf[PAGE_SIZE];


    //struct virtio_net_hdr *hdr = (struct virtio_net_hdr *)buf;
    void *data = buf+sizeof(struct virtio_net_hdr);

    unsigned int maxlen = PAGE_SIZE - sizeof(struct virtio_net_hdr);

    if( len > maxlen )
    {
        SHOW_ERROR( 1, "xmit data cut len %d max %d", len, maxlen );
        len = maxlen;
    }

    memset( buf, 0, PAGE_SIZE );
    memcpy( data, idata, len );

    // put data
    memcpy_v2p( pa, buf, PAGE_SIZE );

    wr[0].addr = pa;
    wr[0].len  = sizeof(struct virtio_net_hdr) + len;
    wr[0].flags = 0;

#else

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
#endif

    virtio_kick( vd, 0);

    return len;
}

static void provide_buffers(virtio_device_t *vd)
{
    struct vring_desc rd[2];

    physaddr_t	pa;
    assert( 0 == hal_alloc_phys_page(&pa));

    SHOW_FLOW( 9, "recv pa = %p", pa );

#if 1
    rd[0].addr = pa;
    rd[0].len  = PAGE_SIZE;
    rd[0].flags = VRING_DESC_F_WRITE;

    virtio_attach_buffers_list( vd, 1, 1, rd );
#else

    rd[0].addr = pa;
    rd[0].len  = sizeof(struct virtio_net_hdr);
    rd[0].flags = 0;

    rd[1].addr = pa + sizeof(struct virtio_net_hdr);
    rd[1].len  = PAGE_SIZE - sizeof(struct virtio_net_hdr);
    rd[1].flags = VRING_DESC_F_WRITE;

    virtio_attach_buffers_list( vd, 1, 2, rd );
#endif
    virtio_kick( vd, 0);

}



static void vnet_thread(void *_dev)
{
    hal_set_thread_name("VirtIOdrv");

    phantom_device_t * 	dev = _dev;
    virtio_device_t  *	vdev = dev->drv_private;
    vionet_t *		vnet = vdev->pvt;

    SHOW_FLOW0( 1, "Thread ready, wait 4 nic active" );

    while(1)
    {
        while( !vnet->active )
            hal_sleep_msec(1000);

        SHOW_FLOW0( 1, "Thread ready, wait 4 sema" );

        hal_sem_acquire( &(vnet->sem) );

#if 1
        while( vnet->recv_buffers_in_driver < MIN_RECV_BUF )
        {
            SHOW_FLOW0( 1, "Provide recv buffer" );
            vnet->recv_buffers_in_driver++;
            provide_buffers(vdev);
        }
#endif


        struct vring_desc rd[2];
        unsigned int dlen;

        // recv q
        int nRead = virtio_detach_buffers_list( vdev, 1, 2, rd, (int *)&dlen );
        if( nRead > 0 )
        {
            // Some reception occured
            SHOW_FLOW0( 1, "Got recv buffer" );

            if( nRead != 1)
                SHOW_ERROR( 1, "Got recv chain %d", nRead );

            if( dlen > PAGE_SIZE || dlen < sizeof(struct virtio_net_hdr))
                SHOW_ERROR( 1, "Got recv dlen %d", dlen );

            physaddr_t	pa = rd[0].addr;

            if( dlen >= sizeof(struct virtio_net_hdr) && dlen <= PAGE_SIZE)
            {
                //struct virtio_net_hdr nh;
                char buf[PAGE_SIZE];

                // get data
                memcpy_p2v( buf, pa, PAGE_SIZE );

                struct virtio_net_hdr *hdr = (struct virtio_net_hdr *)buf;
                void *data = buf+sizeof(struct virtio_net_hdr);

                if( hdr->flags || hdr->gso_type)
                SHOW_ERROR( 1, "Hdr flags/gso != 0 (%x/%x)", hdr->flags, hdr->gso_type );

                // TODO really recv data
                if(vnet->read_max > 0)
                {
                    hal_mutex_lock( &vnet->recv_mutex );

                    size_t olen = dlen - sizeof(struct virtio_net_hdr);

                    if(olen > vnet->read_max)
                    {
                        SHOW_ERROR( 1, "Buffer cut olen %d max %d", olen, vnet->read_max );
                        olen = vnet->read_max;
                    }

                    memcpy( vnet->read_buf, data, olen );
                    vnet->read_len = olen;

                    hal_cond_broadcast( &vnet->recv_cond );
                    hal_mutex_unlock( &vnet->recv_mutex );
                }
            }

            hal_free_phys_page(pa);
        }


        // xmit q
        nRead = virtio_detach_buffers_list( vdev, 0, 2, rd, (int *)&dlen );
        if( nRead > 0 )
        {
            physaddr_t	pa = rd[0].addr;
            // Some reception occured
            SHOW_FLOW( 1, "Got xmit buffer %p", pa );

            if( nRead != 1)
                SHOW_ERROR( 1, "Got xmit chain %d", nRead );

            if( dlen > PAGE_SIZE || dlen < sizeof(struct virtio_net_hdr))
                SHOW_ERROR( 1, "Got xmit dlen %d", dlen );

            hal_free_phys_page(pa);
        }

    }

}


static void driver_virtio_net_interrupt(virtio_device_t *vdev, int isr )
{
    (void) isr;

    vionet_t *		vnet = vdev->pvt;

    (void) vnet;

    SHOW_FLOW0( 4, "got virtio net interrupt");

    // Just try to do everything
    hal_sem_release( &(vnet->sem) );
}










static int vnet_stop(phantom_device_t * dev)
{
    virtio_device_t  *	vdev = dev->drv_private;
    vionet_t *		vnet = vdev->pvt;

    vnet->active = 0;

    return 0;
}

static int vnet_write(phantom_device_t * dev, const void *buf, int buflen )
{
    virtio_device_t  *	vdev = dev->drv_private;
    return driver_virtio_net_write(vdev, buf, buflen);
}

static int vnet_read(phantom_device_t * dev, void *buf, int buflen )
{
    virtio_device_t  *	vdev = dev->drv_private;
    vionet_t *		vnet = vdev->pvt;
    assert(vnet);

    int ret = -1;

    hal_mutex_lock( &vnet->recv_mutex );

    vnet->read_buf = buf;
    vnet->read_max = buflen;

    hal_cond_wait( &vnet->recv_cond, &vnet->recv_mutex );
    ret = vnet->read_len;

    hal_mutex_unlock( &vnet->recv_mutex );

    return ret;
}

static int vnet_get_address( struct phantom_device *dev, void *buf, int len)
{
    virtio_device_t  *	vdev = dev->drv_private;
    vionet_t *		vnet = vdev->pvt;

    int err = NO_ERROR;

    if(!vnet)        return ERR_IO_ERROR;

    if( (unsigned)len >= sizeof(vnet->mac_addr)) {
        memcpy(buf, vnet->mac_addr, sizeof(vnet->mac_addr));
    } else {
        err = ERR_VFS_INSUFFICIENT_BUF;
    }

    return err;

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


#endif // ARCH_ia32
