#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "boot"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <i386/pio.h>
#include <hal.h>
#include <phantom_assert.h>

#include "net/ns8390.h"
#include "device.h"
#include "ethernet_defs.h"

#include "newos.h"
#include "net.h"

#include <driver_isa_ne2000.h>
#include <driver_isa_ne2000_priv.h>

#include <errno.h>


#define WIRED_ADDRESS 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 126)
#define WIRED_NETMASK 	0xffffff00
#define WIRED_BROADCAST IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0xFF)

#define WIRED_NET 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0)
#define WIRED_ROUTER 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 126)

#define DEF_ROUTE_ROUTER        IPV4_DOTADDR_TO_ADDR(10, 0, 2, 2)




#define eth_nic_base (dev->iobase)
#define eth_asic_base (eth_nic_base + NE_ASIC_OFFSET)

#define ASIC_PIO NE_DATA


struct ne
{
    unsigned char 	eth_vendor;
    unsigned char 	eth_flags;

    unsigned char 	eth_memsize;
    unsigned char 	eth_rx_start;
    unsigned char 	eth_tx_start;

    physaddr_t 		eth_bmem;
    physaddr_t 		eth_rmem;

    unsigned char 	eth_drain_receiver;

    unsigned char	node_addr[ETH_ALEN];
};


static void ne_reset(phantom_device_t * dev);
static int ne_disable(phantom_device_t * dev);
static errno_t ne_probe1(int ioaddr);
static errno_t ne_probe(phantom_device_t * dev);
static void enable_multicast(phantom_device_t * dev);
static void eth_pio_write(
                          phantom_device_t * dev,
                          const unsigned char *src,
                          unsigned int dst,
                          unsigned int cnt);
static void eth_pio_read(
                         phantom_device_t * dev,
                         unsigned int src,
                         unsigned char *dst,
                         unsigned int cnt);

static int ne_poll(phantom_device_t * dev, void *buf, int buflen);

static int ne_write(phantom_device_t * dev, const void *buf, int buflen );
static int ne_read(phantom_device_t * dev, void *buf, int buflen );
static int ne_get_address( struct phantom_device *dev, void *buf, int len);




static int seq_number = 0;

phantom_device_t * driver_isa_ne2000_probe( int port, int irq, int stage )
{
    if( seq_number || ne_probe1(port) )        return 0;


    phantom_device_t * dev = calloc(1, sizeof(phantom_device_t));
    assert(dev != 0);

    dev->name = "NE2000";
    dev->seq_number = seq_number++;

    dev->iobase = port;
    dev->irq = irq;
    dev->iomem = 0; // TODO map mem

    dev->dops.stop = ne_disable;
    dev->dops.read = ne_read;
    dev->dops.write = ne_write;
    dev->dops.get_address = ne_get_address;

    dev->drv_private = calloc( 1, sizeof(struct ne));
    assert( dev->drv_private != 0 );

    if( !ne_probe( dev ) )
    {
        free(dev->drv_private);
        free(dev);
        return 0;
    }

    ifnet *interface;
    if( if_register_interface( IF_TYPE_ETHERNET, &interface, dev) )
    {
        SHOW_ERROR( 0, "Failed to register interface for %s", dev->name );
    }
    else
    {
        ifaddr *address;

        // set the ip address for this net interface
        address = malloc(sizeof(ifaddr));
        address->addr.len = 4;
        address->addr.type = ADDR_TYPE_IP;
        NETADDR_TO_IPV4(address->addr) = WIRED_ADDRESS;

        address->netmask.len = 4;
        address->netmask.type = ADDR_TYPE_IP;
        NETADDR_TO_IPV4(address->netmask) = WIRED_NETMASK; 

        address->broadcast.len = 4;
        address->broadcast.type = ADDR_TYPE_IP;
        NETADDR_TO_IPV4(address->broadcast) = WIRED_BROADCAST;

        if_bind_address(interface, address);

        // set up an initial routing table

        int rc;
        if( (rc = ipv4_route_add(
                                 WIRED_NET,
                                 WIRED_NETMASK,
                                 WIRED_ROUTER,
                                 interface->id) ) )
        {
            SHOW_ERROR( 1, "Adding route - failed, rc = %d", rc);
        }
        else
        {
            SHOW_INFO0( 1, "Adding route - ok");
        }


        SHOW_INFO0( 1, "Adding default route...");
        if( (rc = ipv4_route_add_default(
                                         WIRED_ROUTER,
                                         interface->id,
                                         DEF_ROUTE_ROUTER
                                        ) ) )
        {
            SHOW_ERROR( 1, "Adding route - failed, rc = %d", rc);
        }
        else
        {
            SHOW_INFO0( 1, "Adding route - ok");
        }

    }


    return dev;
}

/**************************************************************************
 NE_DISABLE - Turn off adapter
 **************************************************************************/
static int ne_disable(phantom_device_t * dev)
{
    ne_reset(dev);
    return 0;
}

/**************************************************************************
 enable_multicast - Enable Multicast
 **************************************************************************/
static void enable_multicast(phantom_device_t * dev)
{
    unsigned char mcfilter[8];
    int i;

    memset(mcfilter, 0xFF, 8);
    outb( eth_nic_base + D8390_P0_RCR, 4);
    outb( eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_RD2 + D8390_COMMAND_PS1 );
    for (i = 0; i < 8; i++) {
        outb( eth_nic_base + 8 + i, mcfilter[i] );
        if (inb(eth_nic_base + 8 + i) != mcfilter[i])
            SHOW_ERROR( 0, "Error SMC 83C690 Multicast filter read/write mishap %d", i);
    }
    outb( eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_RD2 + D8390_COMMAND_PS0 );
    outb( eth_nic_base + D8390_P0_RCR, 4 | 0x08 );
}


/**************************************************************************
 NE_PROBE - Initialize an adapter ???
 **************************************************************************/
static int ne_probe(phantom_device_t * dev)
{
    int i;
    unsigned char c;
    unsigned char romdata[16];
    unsigned char testbuf[32];

    struct ne *pvt = dev->drv_private;

    pvt->eth_vendor = VENDOR_NONE;
    pvt->eth_drain_receiver = 0;

    /******************************************************************
     Search for NE1000/2000 if no WD/SMC or 3com cards
     ******************************************************************/
    if (pvt->eth_vendor == VENDOR_NONE)
    {

        static unsigned char test[] = "NE*000 memory";

        pvt->eth_bmem = 0; /* No shared memory */

        pvt->eth_flags = FLAG_PIO;
        pvt->eth_memsize = MEM_16384;
        pvt->eth_tx_start = 32;
        pvt->eth_rx_start = 32 + D8390_TXBUF_SIZE;
        c = inb(eth_asic_base + NE_RESET);
        outb( eth_asic_base + NE_RESET, c );
        (void) inb(0x84);
        outb( eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_STP | D8390_COMMAND_RD2 );
        outb( eth_nic_base + D8390_P0_RCR, D8390_RCR_MON );
        outb( eth_nic_base + D8390_P0_DCR, D8390_DCR_FT1 | D8390_DCR_LS );
        outb( eth_nic_base + D8390_P0_PSTART, MEM_8192 );
        outb( eth_nic_base + D8390_P0_PSTOP, MEM_16384 );
        eth_pio_write( dev, (unsigned char *) test, 8192, sizeof(test));
        eth_pio_read( dev, 8192, testbuf, sizeof(test));
        if (!memcmp(test, testbuf, sizeof(test)))
            goto out;
        pvt->eth_flags |= FLAG_16BIT;
        pvt->eth_memsize = MEM_32768;
        pvt->eth_tx_start = 64;
        pvt->eth_rx_start = 64 + D8390_TXBUF_SIZE;
        outb( eth_nic_base + D8390_P0_DCR, D8390_DCR_WTS | D8390_DCR_FT1 | D8390_DCR_LS);
        outb( eth_nic_base + D8390_P0_PSTART, MEM_16384);
        outb( eth_nic_base + D8390_P0_PSTOP, MEM_32768);
        eth_pio_write( dev, (unsigned char *) test, 16384, sizeof(test));
        eth_pio_read( dev, 16384, testbuf, sizeof(test));
        if (!memcmp(testbuf, test, sizeof(test)))
            goto out;

    out:
        if (eth_nic_base == 0)
            return (0);

        if (eth_nic_base > ISA_MAX_ADDR) /* PCI probably */
            pvt->eth_flags |= FLAG_16BIT;

        pvt->eth_vendor = VENDOR_NOVELL;

        eth_pio_read( dev, 0, romdata, sizeof(romdata));

        for (i = 0; i < ETH_ALEN; i++) {
            pvt->node_addr[i] = romdata[i + ((pvt->eth_flags & FLAG_16BIT) ? i : 0)];
        }

        //nic->ioaddr = eth_nic_base;
        SHOW_INFO( 0, "NE%c000 base 0x%X, MAC Addr %02x:%02x:%02x:%02x:%02x:%02x",
                   (pvt->eth_flags & FLAG_16BIT) ? '2' : '1', eth_nic_base,
                   //eth_ntoa( pvt->node_addr)

                   pvt->node_addr[0],
                   pvt->node_addr[1],
                   pvt->node_addr[2],
                   pvt->node_addr[3],
                   pvt->node_addr[4],
                   pvt->node_addr[5]

                 );
    }

    if (pvt->eth_vendor == VENDOR_NONE)
        return (0);

    if (pvt->eth_vendor != VENDOR_3COM)
        pvt->eth_rmem = pvt->eth_bmem;

    ne_reset(dev);
    //nic->nic_op = &ne_operations;
    return 1;
}



/**************************************************************************
 ETH_PIO_WRITE - Write a frame via Programmed I/O
 **************************************************************************/
static void eth_pio_write( phantom_device_t * dev,
                           const unsigned char *src,
                           unsigned int dst,
                           unsigned int cnt)
{
    struct ne *pvt = dev->drv_private;

    outb(eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_RD2 | D8390_COMMAND_STA);
    outb(eth_nic_base + D8390_P0_ISR, D8390_ISR_RDC);

    outb( eth_nic_base + D8390_P0_RBCR0, cnt );
    outb( eth_nic_base + D8390_P0_RBCR1, cnt >> 8);

    outb( eth_nic_base + D8390_P0_RSAR0, dst);
    outb( eth_nic_base + D8390_P0_RSAR1, dst >> 8);

    outb( eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_RD1 | D8390_COMMAND_STA);
    if (pvt->eth_flags & FLAG_16BIT)
        cnt = (cnt + 1) >> 1;

    while (cnt--) {
        if (pvt->eth_flags & FLAG_16BIT) {
            outw( eth_asic_base + ASIC_PIO, *((unsigned short *) src));
            src += 2;
        } else
            outb( eth_asic_base + ASIC_PIO, *(src++) );
    }
}


/**************************************************************************
 NE_PROBE1 - Look for an adapter on the ISA bus
 **************************************************************************/
static errno_t ne_probe1(int ioaddr) {
    //From the eCos driver
    unsigned int regd;
    unsigned int state;


    state = inb(ioaddr);
    outb(ioaddr, D8390_COMMAND_RD2 | D8390_COMMAND_PS1 | D8390_COMMAND_STP);
    regd = inb(ioaddr + D8390_P0_TCR);

    if (inb(ioaddr + D8390_P0_TCR)) {
        outb(ioaddr, state);
        outb(ioaddr + 0x0d, regd);
        return 0;
    }

    return ENXIO;
}



#if 0
/**************************************************************************
 NE_TRANSMIT - Transmit a frame
 **************************************************************************/
static void ne_transmit(//struct nic *nic,
                                          phantom_device_t * dev,
                                          const char *d, /* Destination */
                                          unsigned int t, /* Type */
                                          unsigned int s, /* size */
                                          const char *p) { /* Packet */

    struct ne *pvt = dev->drv_private;

    /* Programmed I/O */
    unsigned short type;
    type = (t >> 8) | (t << 8);
    eth_pio_write( dev, (unsigned char *) d, pvt->eth_tx_start << 8, ETH_ALEN);
    eth_pio_write( dev, pvt->node_addr, (pvt->eth_tx_start << 8) + ETH_ALEN, ETH_ALEN);
    /* bcc generates worse code without (const+const) below */
    eth_pio_write( dev, (unsigned char *) &type, (pvt->eth_tx_start << 8) + (ETH_ALEN
                                                                             + ETH_ALEN), 2);
    eth_pio_write( dev, (unsigned char *) p, (pvt->eth_tx_start << 8) + ETH_HLEN, s);
    s += ETH_HLEN;
    if (s < ETH_ZLEN)
        s = ETH_ZLEN;

    outb( eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_PS0 | D8390_COMMAND_RD2 | D8390_COMMAND_STA );
    outb( eth_nic_base + D8390_P0_TPSR, pvt->eth_tx_start );
    outb( eth_nic_base + D8390_P0_TBCR0, s );
    outb( eth_nic_base + D8390_P0_TBCR1, s >> 8 );

    outb( eth_nic_base + D8390_P0_COMMAND,
          D8390_COMMAND_PS0 | D8390_COMMAND_TXP |
          D8390_COMMAND_RD2 | D8390_COMMAND_STA );
}
#endif

static int ne_write(phantom_device_t * dev, const void *buf, int in_buflen )
{
    struct ne *pvt = dev->drv_private;

    int buflen = in_buflen;
    assert( buflen < 2048 ); // TODO correct limit
    if(buflen < 0)
        return ERR_INVALID_ARGS;

    // TODO sleep until have place in buf

    eth_pio_write( dev, buf, pvt->eth_tx_start << 8, buflen);

    if(buflen < ETH_ZLEN) buflen = ETH_ZLEN;

    outb( eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_PS0 | D8390_COMMAND_RD2 | D8390_COMMAND_STA );
    outb( eth_nic_base + D8390_P0_TPSR, pvt->eth_tx_start );
    outb( eth_nic_base + D8390_P0_TBCR0, buflen );
    outb( eth_nic_base + D8390_P0_TBCR1, buflen >> 8 );

    outb( eth_nic_base + D8390_P0_COMMAND,
          D8390_COMMAND_PS0 | D8390_COMMAND_TXP |
          D8390_COMMAND_RD2 | D8390_COMMAND_STA );

    return in_buflen;
}



/**************************************************************************
 ETH_PIO_READ - Read a frame via Programmed I/O
 **************************************************************************/
static void eth_pio_read(
                         phantom_device_t * dev,
                         unsigned int src,
                         unsigned char *dst,
                         unsigned int cnt)
{
    struct ne *pvt = dev->drv_private;

    outb( eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_RD2 | D8390_COMMAND_STA );
    outb( eth_nic_base + D8390_P0_RBCR0, cnt );
    outb( eth_nic_base + D8390_P0_RBCR1, cnt >> 8 );
    outb( eth_nic_base + D8390_P0_RSAR0, src );
    outb( eth_nic_base + D8390_P0_RSAR1, src >> 8 );
    outb( eth_nic_base + D8390_P0_COMMAND, D8390_COMMAND_RD0 | D8390_COMMAND_STA );

    if (pvt->eth_flags & FLAG_16BIT)
        cnt = (cnt + 1) >> 1;

    while (cnt--) {
        if (pvt->eth_flags & FLAG_16BIT) {
            *((unsigned short *) dst) = inw(eth_asic_base + ASIC_PIO);
            dst += 2;
        } else
            *(dst++) = inb(eth_asic_base + ASIC_PIO);
    }
}



/**************************************************************************
 NE_RESET - Reset adapter
 **************************************************************************/
static void ne_reset(phantom_device_t * dev)
{
    int i;

    struct ne *pvt = dev->drv_private;

    pvt->eth_drain_receiver = 0;
    outb( eth_nic_base+D8390_P0_COMMAND,
          D8390_COMMAND_PS0 |
          D8390_COMMAND_RD2 |
          D8390_COMMAND_STP );
    if (pvt->eth_flags & FLAG_16BIT)
        outb( eth_nic_base+D8390_P0_DCR, 0x49 );
    else
        outb( eth_nic_base+D8390_P0_DCR, 0x48 );

    outb( eth_nic_base+D8390_P0_RBCR0, 0);
    outb( eth_nic_base+D8390_P0_RBCR1, 0);
    outb( eth_nic_base+D8390_P0_RCR, 0x20 ); /* monitor mode */
    outb( eth_nic_base+D8390_P0_TCR, 2);
    outb( eth_nic_base+D8390_P0_TPSR, pvt->eth_tx_start );
    outb( eth_nic_base+D8390_P0_PSTART, pvt->eth_rx_start );

    outb( eth_nic_base+D8390_P0_PSTOP, pvt->eth_memsize );
    outb( eth_nic_base+D8390_P0_BOUND, pvt->eth_memsize - 1 );
    outb( eth_nic_base+D8390_P0_ISR, 0xFF);
    outb( eth_nic_base+D8390_P0_IMR, 0);
    outb(eth_nic_base+D8390_P0_COMMAND,
         D8390_COMMAND_PS1 |
         D8390_COMMAND_RD2 |
         D8390_COMMAND_STP );

    for (i=0; i<ETH_ALEN; i++)
        outb( eth_nic_base+D8390_P1_PAR0+i, pvt->node_addr[i] );

    for (i=0; i<ETH_ALEN; i++)
        outb( eth_nic_base+D8390_P1_MAR0+i, 0xFF );

    outb( eth_nic_base+D8390_P1_CURR , pvt->eth_rx_start );
    outb( eth_nic_base+D8390_P0_COMMAND,
          D8390_COMMAND_PS0 |
          D8390_COMMAND_RD2 |
          D8390_COMMAND_STA );

    outb( eth_nic_base+D8390_P0_ISR, 0xFF );
    outb( eth_nic_base+D8390_P0_TCR, 0 ); /* transmitter on */
    outb( eth_nic_base+D8390_P0_RCR, 4 ); /* allow rx broadcast frames */

    enable_multicast(dev);
}





/**************************************************************************
 NE_POLL - Wait for a frame
 **************************************************************************/
static int ne_poll(phantom_device_t * dev, void *buf, int buflen)
{
    int ret = 0;
    unsigned char rstat, curr, next;
    unsigned short len, frag;
    unsigned short pktoff;
    //unsigned char *p;
    struct ringbuffer pkthdr;

    struct ne *pvt = dev->drv_private;

    rstat = inb(eth_nic_base+D8390_P0_RSR);
    if (!(rstat & D8390_RSTAT_PRX)) return 0;
    next = inb(eth_nic_base+D8390_P0_BOUND)+1;
    if (next >= pvt->eth_memsize) next = pvt->eth_rx_start;
    outb( eth_nic_base+D8390_P0_COMMAND, D8390_COMMAND_PS1 );
    curr = inb(eth_nic_base+D8390_P1_CURR);
    outb( eth_nic_base+D8390_P0_COMMAND, D8390_COMMAND_PS0 );

    if (curr >= pvt->eth_memsize) curr = pvt->eth_rx_start;
    if (curr == next) return 0;

    //if ( ! retrieve ) return 1;

    pktoff = next << 8;
    if( pvt->eth_flags & FLAG_PIO )
        eth_pio_read( dev, pktoff, (unsigned char *)&pkthdr, 4);
    else
    {
        //memcpy(&pkthdr, bus_to_virt(eth_rmem + pktoff), 4);
        memcpy_p2v( &pkthdr, pvt->eth_rmem + pktoff, 4 );
    }

    pktoff += sizeof(pkthdr);

    /* incoming length includes FCS so must sub 4 */
    len = pkthdr.len - 4;

    if ((pkthdr.status & D8390_RSTAT_PRX) == 0 || len < ETH_ZLEN
        || len > ETH_FRAME_LEN) {
        SHOW_ERROR0( 0, "Bogus packet, ignoring" );
        return 0;
    }

    unsigned char *p = buf;
    //p = nic->packet;
    //nic->packetlen = len; /* available to caller */

    frag = (pvt->eth_memsize << 8) - pktoff;

    if( len > buflen )
    {
        SHOW_ERROR0(0, "Packet too long");
        len = buflen;
    }
    ret = 0;

    if(len > frag)
    { /* We have a wrap-around */
        /* read first part */
        if( pvt->eth_flags & FLAG_PIO )
            eth_pio_read( dev, pktoff, p, frag);
        else
        {
            //memcpy(p, bus_to_virt(eth_rmem + pktoff), frag);
            memcpy_p2v( p, pvt->eth_rmem + pktoff, frag );
        }
        pktoff = pvt->eth_rx_start << 8;
        p += frag;
        ret += frag;
        len -= frag;
    }

    /* read second part */
    if( pvt->eth_flags & FLAG_PIO)
        eth_pio_read( dev, pktoff, p, len);
    else
    {
        //memcpy(p, bus_to_virt(eth_rmem + pktoff), len);
        memcpy_p2v( p, pvt->eth_rmem + pktoff, len );
        ret += len;
    }

    next = pkthdr.next; /* frame number of next packet */

    if(next == pvt->eth_rx_start)
        next = pvt->eth_memsize;

    outb( eth_nic_base+D8390_P0_BOUND, next-1 );

    return ret;
}






/**************************************************************************
 Wrappers
 **************************************************************************/


static int ne_read( struct phantom_device *dev, void *buf, int len)
{
    if(len < ETHERNET_MAX_SIZE)
        return ERR_VFS_INSUFFICIENT_BUF;

    int ret = 0;
    do {
        ret = ne_poll(dev, buf, len);
        if( ret <= 0 )
        {
            hal_sleep_msec(200); // BUG! POLLING!
        }
    } while( ret <= 0 );

    return ret;
}



static int ne_get_address( struct phantom_device *dev, void *buf, int len)
{
    struct ne *pvt = dev->drv_private;
    int err = NO_ERROR;

    if(!pvt)        return ERR_IO_ERROR;

    if(len >= sizeof(pvt->node_addr)) {
        memcpy(buf, pvt->node_addr, sizeof(pvt->node_addr));
    } else {
        err = ERR_VFS_INSUFFICIENT_BUF;
    }

    return err;
}








#if 0

/**************************************************************************
 ETHERBOOT -  BOOTP/TFTP Bootstrap Program

 Author: Martin Renters
 Date: May/94

 This code is based heavily on David Greenman's if_ed.c driver

 Copyright (C) 1993-1994, David Greenman, Martin Renters.
 This software may be used, modified, copied, distributed, and sold, in
 both source and binary form provided that the above copyright and these
 terms are retained. Under no circumstances are the authors responsible for
 the proper functioning of this software, nor do the authors assume any
 responsibility for damages incurred with its use.

 Multicast support added by Timothy Legge (timlegge@users.sourceforge.net) 09/28/2003
 Relocation support added by Ken Yap (ken_yap@users.sourceforge.net) 28/12/02
 Card Detect support adapted from the eCos driver (Christian Plessl <cplessl@ee.ethz.ch>)
 Extracted from ns8390.c and adapted by Pantelis Koukousoulas <pktoss@gmail.com>
 **************************************************************************/


static isa_probe_addr_t ne_probe_addrs[] = { 0x300, 0x280, 0x320, 0x340, 0x380, 0x220, };




#endif

#endif // HAVE_UNIX

