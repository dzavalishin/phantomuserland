#if HAVE_PCI && HAVE_NET

/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * VirtIo Net driver. Seems to be working.
 *
 *
**/

#define DEBUG_MSG_PREFIX "VirtIo.Net"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 11
#define debug_level_info 1

#include <phantom_libc.h>

#include <kernel/vm.h>
#include <kernel/drivers.h>
#include <kernel/ethernet_defs.h>
#include <kernel/page.h>

#include <kernel/virtio.h>
#include <virtio_pci.h>
#include <virtio_net.h>

#include <threads.h>

#include <device.h>
#include <kernel/net.h>


#define DEBUG_NO_INTR 0

// FIXME some races prevent this driver from starting at stage 1



#define WIRED_ADDRESS 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 122)
#define WIRED_NETMASK 	0xffffff00
#define WIRED_BROADCAST IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0xFF)

#define WIRED_NET 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0)
#define WIRED_ROUTER 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 122)

#define DEF_ROUTE_ROUTER        IPV4_DOTADDR_TO_ADDR(10, 0, 2, 2)




// At least this many buffers driver must have in recv queue
#define MIN_RECV_BUF    8
#define MAX_SEND_BUF    16


typedef struct vionet
{
    unsigned char	mac_addr[ETH_ALEN];

    int                 thread;
    hal_sem_t           sem;                    // thread wakeup sem

    int                 active;

    int                 recv_buffers_in_driver;      // How many recv buffers driver has on its side
    int                 send_buffers_in_driver;

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

    vdev.guest_features = 0;
    vdev.guest_features |= (1<<VIRTIO_NET_F_MAC); // Does not work on QEMU

    if( virtio_probe( &vdev, pci ) )
        return 0;

    SHOW_INFO( 11, "Status is: 0x%X", virtio_get_status( &vdev ) );

    /* driver is ready */
    //virtio_set_status( &vdev, VIRTIO_CONFIG_S_ACKNOWLEDGE );
    virtio_set_status( &vdev, VIRTIO_CONFIG_S_DRIVER );

    SHOW_INFO( 11, "Status is: 0x%X", virtio_get_status( &vdev ) );






    SHOW_INFO( 1, "Host features are: 0x%b", vdev.host_features, "\020\1CSUM\2GUEST_CSUM\6MAC\7GSO\x8GUEST_TSO4\x9GUEST_TSO6\xaGUEST_ECN\xbGUEST_UFO\xcHOST_TSO4\xdHOST_TSO6\xeHOST_ECN\xfHOST_UFO\x10MRG_RXBUF" );

    struct virtio_net_config cfg;
    virtio_get_config_struct( &vdev, &cfg, sizeof(cfg) );

    /* driver is ready */
    //virtio_set_status( &vdev, VIRTIO_CONFIG_S_ACKNOWLEDGE );
    virtio_set_status( &vdev, VIRTIO_CONFIG_S_DRIVER|VIRTIO_CONFIG_S_DRIVER_OK );


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

/*
    // driver is ready
    virtio_set_status( &vdev,  VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER );
    SHOW_INFO( 0, "Status is: 0x%X", virtio_get_status( &vdev ) );
    hal_sleep_msec(10);
*/

    /* driver is ready */
//    virtio_set_status( &vdev,  VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER | VIRTIO_CONFIG_S_DRIVER_OK);
    //virtio_set_status( &vdev,  VIRTIO_CONFIG_S_DRIVER | VIRTIO_CONFIG_S_DRIVER_OK );
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


static void cleanup_xmit_buffers(virtio_device_t *vdev)
{
    vionet_t *		vnet = vdev->pvt;
    struct vring_desc 	rd[2];
    unsigned int 	dlen;
    int 		nRead;

    // xmit q
    while( (nRead = virtio_detach_buffers_list( vdev, VIRTIO_NET_Q_XMIT, 2, rd, (int *)&dlen )) > 0 )
    {
        physaddr_t	pa = rd[0].addr;
        // Send finished, got buffer back
        SHOW_FLOW( 1, "Got back xmit buffer %p", pa );

        if( nRead != 2 )
            SHOW_ERROR( 1, "Got back xmit chain of wrong length %d", nRead );

        if( dlen > PAGE_SIZE || dlen < sizeof(struct virtio_net_hdr) )
            SHOW_ERROR( 1, "Got back xmit dlen %d", dlen );

        if( rd[0].len != sizeof(struct virtio_net_hdr) )
            SHOW_ERROR( 1, "hdr size not %d", sizeof(struct virtio_net_hdr) );

        hal_free_phys_page(pa);
        vnet->send_buffers_in_driver--;
    }
}



int driver_virtio_net_write(virtio_device_t *vd, const void *idata, size_t len)
{
    vionet_t *		vnet = vd->pvt;
    struct vring_desc wr[2];

    cleanup_xmit_buffers(vd);

    // TODO dumb! just sleep!
    if( vnet->send_buffers_in_driver > MAX_SEND_BUF )
        SHOW_ERROR( 0, "Send stall with %d bufs in driver", vnet->send_buffers_in_driver );

    while( vnet->send_buffers_in_driver > MAX_SEND_BUF )
    {
        hal_sleep_msec(100);
        cleanup_xmit_buffers(vd);
    }

#if 1
    physaddr_t	pa;
    assert( 0 == hal_alloc_phys_page(&pa) );

    SHOW_FLOW( 9, "xmit pa = %p len = %d", pa, len );


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

    //hexdump( idata, len, 0, 0 );
    //hexdump( buf, sizeof(struct virtio_net_hdr) + len, 0, 0 );

    // put data
    memcpy_v2p( pa, buf, PAGE_SIZE );

    wr[0].addr = pa;
    //wr[0].len  = sizeof(struct virtio_net_hdr) + len;
    wr[0].len  = sizeof(struct virtio_net_hdr);
    wr[0].flags = 0;

    wr[1].addr = pa+sizeof(struct virtio_net_hdr);
    wr[1].len  = len;
    wr[1].flags = 0;

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

#endif

    vnet->send_buffers_in_driver++;

    virtio_attach_buffers_list( vd, VIRTIO_NET_Q_XMIT, 2, wr );
    virtio_kick( vd, VIRTIO_NET_Q_XMIT );

    return len;
}

static void provide_buffers(virtio_device_t *vd)
{
    struct vring_desc rd[2];

    physaddr_t	pa;
    assert( 0 == hal_alloc_phys_page(&pa) );

    memzero_page_v2p( pa ); // TODO pre-zero pages! alloc_zero_phys_page!

    SHOW_FLOW( 9, "recv pa = %p", pa );

#if 0
    rd[0].addr = pa;
    rd[0].len  = PAGE_SIZE;
    rd[0].flags = VRING_DESC_F_WRITE;

    virtio_attach_buffers_list( vd, VIRTIO_NET_Q_RECV, 1, rd );
#else

    rd[0].addr = pa;
    rd[0].len  = sizeof(struct virtio_net_hdr);
    rd[0].flags = 0;
    rd[0].flags = VRING_DESC_F_WRITE;

    rd[1].addr = pa + sizeof(struct virtio_net_hdr);
    rd[1].len  = PAGE_SIZE - sizeof(struct virtio_net_hdr);
    rd[1].flags = VRING_DESC_F_WRITE;

    virtio_attach_buffers_list( vd, VIRTIO_NET_Q_RECV, 2, rd );
#endif
    virtio_kick( vd, VIRTIO_NET_Q_RECV );

}



static void vnet_thread(void *_dev)
{
    t_current_set_name("VirtIOdrv");

    phantom_device_t * 	dev = _dev;
    virtio_device_t  *	vdev = dev->drv_private;
    vionet_t *		vnet = vdev->pvt;

    SHOW_FLOW0( 1, "Thread ready, wait 4 nic active" );

    while(1)
    {
        while( !vnet->active )
            hal_sleep_msec(1000);

        // Provide recv buffers before going to sleep
#if 1
        while( vnet->recv_buffers_in_driver < MIN_RECV_BUF )
        {
            SHOW_FLOW0( 1, "Provide recv buffer" );
            vnet->recv_buffers_in_driver++;
            provide_buffers(vdev);
        }
#endif

        SHOW_FLOW0( 1, "Thread ready, wait 4 sema" );

#if DEBUG_NO_INTR
        hal_sleep_msec(1000);
#else
        hal_sem_acquire( &(vnet->sem) );
#endif

        SHOW_FLOW0( 1, "Thread sema activated" );



        struct vring_desc rd[2];
        unsigned int dlen;

        // recv q
        int nRead = virtio_detach_buffers_list( vdev, VIRTIO_NET_Q_RECV, 2, rd, (int *)&dlen );
        if( nRead > 0 )
        {
            // Some reception occured
            SHOW_FLOW0( 1, "Got recv buffer" );

            if( nRead != 2)
                SHOW_ERROR( 1, "Got recv chain of wrong len %d", nRead );

            if( dlen > PAGE_SIZE || dlen < sizeof(struct virtio_net_hdr))
                SHOW_ERROR( 1, "Got recv dlen %d", dlen );

            vnet->recv_buffers_in_driver--;

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

        cleanup_xmit_buffers(vdev);

    }

}


static void driver_virtio_net_interrupt(virtio_device_t *vdev, int isr )
{
    (void) isr;
    vionet_t *		vnet = vdev->pvt;

#if DEBUG_NO_INTR
    //SHOW_FLOW0( 4, "got virtio net interrupt");
    lprintf("!!! got virtio net interrupt !!!");
#endif

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






#endif // HAVE_NET && HAVE_PCI
