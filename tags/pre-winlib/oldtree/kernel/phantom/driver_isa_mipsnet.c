/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Quick and dirty MipsNet driver.
 *
**/


#if HAVE_NET && defined(ARCH_mips)

#define DEBUG_MSG_PREFIX "mipsnet"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_assert.h>

#include <hal.h>
#include <threads.h>
#include <errno.h>
#include <device.h>

#include <kernel/drivers.h>
#include <kernel/ethernet_defs.h>
#include <kernel/net.h>

#include <mips/arch/board-mips-mipssim-defs.h>


#define WIRED_ADDRESS 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 126)
#define WIRED_NETMASK 	0xffffff00
#define WIRED_BROADCAST IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0xFF)

#define WIRED_NET 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0)
#define WIRED_ROUTER 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 126)

#define DEF_ROUTE_ROUTER        IPV4_DOTADDR_TO_ADDR(10, 0, 2, 2)



#define BUSRD(___addr) (isa_bus.read32(___addr))
#define BUSWR(___addr, ___data) (isa_bus.write32(___addr, ___data))



struct mipsnet
{
    unsigned char	node_addr[ETH_ALEN];

    //int                 thread;

    //int                 active;
    int                 interrupt_count;
    int                 test_interrupt;

    hal_sem_t           recv_interrupt_sem;
    hal_sem_t           send_interrupt_sem;
};


static void mipsnet_reset(phantom_device_t * dev);

static errno_t mipsnet_probe(unsigned int ioaddr);

static void mipsnet_pio_write(
                          phantom_device_t * dev,
                          const unsigned char *src,
                          unsigned int cnt);
static int mipsnet_pio_read(
                         phantom_device_t * dev,
                         unsigned char *dst,
                         unsigned int cnt);

static int mipsnet_write(phantom_device_t * dev, const void *buf, int buflen );
static int mipsnet_read(phantom_device_t * dev, void *buf, int buflen );
static int mipsnet_get_address( struct phantom_device *dev, void *buf, int len);


static void mipsnet_interrupt( void *_dev );
//static void mipsnet_thread(void *_dev );

static int seq_number = 0;

phantom_device_t * driver_isa_mipsnet_probe( int port, int irq, int stage )
{
    (void) stage;
    SHOW_FLOW( 0, "Looking for MipsNet at 0x%x", port );

    if( mipsnet_probe(port) )
    {
        SHOW_ERROR( 1, "failed for port 0x%X", port );
        return 0;
    }


    phantom_device_t * dev = calloc(1, sizeof(phantom_device_t));
    assert(dev != 0);

    dev->name = "NE2000";
    dev->seq_number = seq_number++;

    dev->iobase = port;
    dev->irq = irq;
    dev->iomem = 0; // TODO map mem

    //dev->dops.stop = mipsnet_disable;
    dev->dops.read = mipsnet_read;
    dev->dops.write = mipsnet_write;
    dev->dops.get_address = mipsnet_get_address;

    dev->drv_private = calloc( 1, sizeof(struct mipsnet));
    assert( dev->drv_private != 0 );

    struct mipsnet *pvt = dev->drv_private;

    //hal_sem_init( &(pvt->reset_sem), "Ne2kReset" );

    hal_sem_init( &(pvt->recv_interrupt_sem), "mipsRecv" );
    hal_sem_init( &(pvt->send_interrupt_sem), "mipsSend" );

    mipsnet_reset(dev);

    if( hal_irq_alloc( irq, &mipsnet_interrupt, dev, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", irq );
        //goto free2;
    //free2:
        free(dev->drv_private);
        free(dev);
        return 0;
    }

    //pvt->thread = hal_start_kernel_thread_arg( mipsnet_thread, dev );

    //mipsnet_ei(dev);

    //pvt->active = 1;


    ifnet *interface;
    if( if_register_interface( IF_TYPE_ETHERNET, &interface, dev) )
    {
        SHOW_ERROR( 0, "Failed to register interface for %s", dev->name );
    }
    else
        if_simple_setup(interface, WIRED_ADDRESS, WIRED_NETMASK, WIRED_BROADCAST, WIRED_NET, WIRED_ROUTER, DEF_ROUTE_ROUTER );


    return dev;
}


static void mipsnet_interrupt( void *_dev )
{
    phantom_device_t * dev = _dev;
    struct mipsnet *pvt = dev->drv_private;

    (void) pvt;

    u_int32_t status = BUSRD( dev->iobase + MIPSNET_INT_CTL );

    if( status & MIPSNET_INTCTL_TESTBIT )
    {
        pvt->test_interrupt++;
        BUSWR( dev->iobase + MIPSNET_INT_CTL, 0 ); // ack test interrupt
    }

    // ack rx/tx interrupts
    BUSWR( dev->iobase + MIPSNET_INT_CTL, status & (MIPSNET_INTCTL_RXDONE|MIPSNET_INTCTL_TXDONE) );

    SHOW_FLOW( 6, "Interrupt status %b", status, "\020\1TXDONE\2RXDONE\32TEST" );

    if( status & MIPSNET_INTCTL_RXDONE )
        hal_sem_release( &(pvt->recv_interrupt_sem) );

    if( status & MIPSNET_INTCTL_TXDONE )
        hal_sem_release( &(pvt->send_interrupt_sem) );

    //if( status & (D8390_ISR_RXE|D8390_ISR_TXE) )        hal_sem_release( &(pvt->reset_sem) );


}


/*
static void mipsnet_thread(void *_dev)
{
    hal_set_thread_name("MipsNetDrv");

    phantom_device_t * dev = _dev;
    struct mipsnet *pvt = dev->drv_private;

    SHOW_FLOW0( 1, "Thread ready, wait 4 nic active" );

    while(1)
    {
        while( !pvt->active )
            hal_sleep_msec(1000);

        SHOW_FLOW0( 1, "Thread ready, wait 4 sema" );

        hal_sem_acquire( &(pvt->reset_sem) );


        // TODO reset card on some errors

        //SHOW_FLOW( 1, "Reset card, status = 0x%x", read_csr(nic, PCNET_CSR_STATUS));
        SHOW_ERROR0( 0, "Reset card");
        mipsnet_reset(dev);
        mipsnet_ei(dev);
    }

}
*/


/**************************************************************************
 Turn off adapter
 ************************************************************************** /
static int mipsnet_disable(phantom_device_t * dev)
{
    mipsnet_reset(dev);
    return 0;
}*/



/**************************************************************************
 NE_PROBE0 - Look for an adapter on the ISA bus
 **************************************************************************/
static errno_t mipsnet_probe(unsigned int ioaddr)
{
    unsigned int id0 = BUSRD( ioaddr+MIPSNET_DEV_ID );
    unsigned int id1 = BUSRD( ioaddr+MIPSNET_DEV_ID+4 );

    if( (id0 != MIPSNET_DEV_ID_VALUE_LO) || (id1 != MIPSNET_DEV_ID_VALUE_HI) )
    {
        SHOW_ERROR( 0, "dev not found @ %p, id = %x, %x", ioaddr, id0, id1 );
        return ENXIO;
    }

    return 0;
}




/**************************************************************************
 Write a frame via Programmed I/O
 **************************************************************************/
static void mipsnet_pio_write( phantom_device_t * dev,
                           const unsigned char * src,
                           unsigned int cnt)
{
    //struct mipsnet *pvt = dev->drv_private;

    BUSWR( dev->iobase + MIPSNET_TX_DATA_COUNT, cnt );

    while (cnt--)
        BUSWR( dev->iobase + MIPSNET_TX_DATA_BUFFER, *src++ );
}



/**************************************************************************
 Read a frame via Programmed I/O
 **************************************************************************/
static int mipsnet_pio_read(
                         phantom_device_t * dev,
                         unsigned char *dst,
                         unsigned int cnt)
{
    //struct mipsnet *pvt = dev->drv_private;

    unsigned int devcnt = BUSRD( dev->iobase + MIPSNET_RX_DATA_COUNT );
    assert(devcnt > 0);

    int ret = 0;

    while( devcnt-- )
    {
        u_int8_t data = BUSRD( dev->iobase + MIPSNET_RX_DATA_BUFFER );
        if( cnt-- )
        {
            *dst++ = data;
            ret++;
        }
    }

    // Space in buf? Zero.
    while(cnt--)
    {
        *dst++ = 0;
        ret++;
    }

    return ret;
}











/**************************************************************************
 NE_RESET - Reset adapter
 **************************************************************************/
static void mipsnet_reset(phantom_device_t * dev)
{
    struct mipsnet *pvt = dev->drv_private;

    // Release send sema once so that we'll xmit first packet without a problem
    hal_sem_release( &(pvt->send_interrupt_sem) );

    BUSWR( dev->iobase + MIPSNET_RX_DATA_COUNT, 0 );
    BUSWR( dev->iobase + MIPSNET_TX_DATA_COUNT, 0 );

    BUSWR( dev->iobase + MIPSNET_INT_CTL, MIPSNET_INTCTL_TXDONE|MIPSNET_INTCTL_RXDONE|MIPSNET_INTCTL_TESTBIT );

}





/**************************************************************************
 Interface
 **************************************************************************/


static int mipsnet_read( struct phantom_device *dev, void *buf, int len)
{
    SHOW_FLOW( 4, "read %d bytes", len );

    if(len < ETHERNET_MAX_SIZE)
        return ERR_VFS_INSUFFICIENT_BUF;

    struct mipsnet *pvt = dev->drv_private;

    int ret = 0;
    do {
        SHOW_FLOW( 7, "poll %d bytes", len );

        hal_sem_acquire( &(pvt->recv_interrupt_sem) );

        ret = mipsnet_pio_read( dev, buf, len );

    } while( ret <= 0 );

    //hexdump( buf, ret, "recv pkt", 0 );

    SHOW_FLOW( 7, "read %d bytes", ret );
    return ret;
}

static int mipsnet_write(phantom_device_t * dev, const void *buf, int in_buflen )
{
    struct mipsnet *pvt = dev->drv_private;

    SHOW_FLOW( 3, "write %d bytes", in_buflen );

    //hexdump( buf, in_buflen, "pkt", 0 );

    int buflen = in_buflen;
    //assert( buflen < 1514 );

    if( buflen > 1514 )
        buflen = 1514;

    if(buflen < 0)
        return ERR_INVALID_ARGS;

    hal_sem_acquire( &(pvt->send_interrupt_sem) );

    mipsnet_pio_write( dev, buf, buflen);

    return buflen;
}


static int mipsnet_get_address( struct phantom_device *dev, void *buf, int len)
{
    struct mipsnet *pvt = dev->drv_private;
    int err = NO_ERROR;

    if(!pvt)        return ERR_IO_ERROR;

    if( (unsigned)len >= sizeof(pvt->node_addr)) {
        memcpy(buf, pvt->node_addr, sizeof(pvt->node_addr));
    } else {
        err = ERR_VFS_INSUFFICIENT_BUF;
    }

    return err;
}









#endif // HAVE_NET & MIPS

