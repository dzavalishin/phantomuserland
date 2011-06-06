#include <kernel/config.h>
#if HAVE_NET && defined(ARCH_ia32)

#include <phantom_libc.h>
#include <i386/pio.h>
//#include "driver_map.h"
#include <device.h>
#include <kernel/drivers.h>

#include "driver_pci_intel82559.h"
#include "driver_pci_intel82559_priv.h"

#include <kernel/ethernet_defs.h>

//#include "newos.h"
#include "net.h"


#if 0
#  define WW() getchar()
#else
#  define WW()
#endif

#define DEV_NAME "Intel82559: "

#define KERN_ERR "Error: "
#define KERN_INFO "Info: "
#define printk printf

static int DEBUG = 3;


#define WIRED_ADDRESS 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 125)
#define WIRED_NETMASK 	0xffffff00
#define WIRED_BROADCAST IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0xFF)

#define WIRED_NET 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0)
#define WIRED_ROUTER 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 125)

#define DEF_ROUTE_ROUTER        IPV4_DOTADDR_TO_ADDR(10, 0, 2, 2)








/* Used to pass the media type, etc.
   Both 'options[]' and 'full_duplex[]' should exist for driver
   interoperability, however setting full_duplex[] is deprecated.
   The media type is usually passed in 'options[]'.
    Use option values 0x10/0x20 for 10Mbps, 0x100,0x200 for 100Mbps.
    Use option values 0x10 and 0x100 for forcing half duplex fixed speed.
    Use option values 0x20 and 0x200 for forcing full duplex operation.
*/
#define MAX_UNITS 8		/* More are supported, limit only on options */
//static int options[MAX_UNITS] = {-1, -1, -1, -1, -1, -1, -1, -1};
static int full_duplex[MAX_UNITS] = {-1, -1, -1, -1, -1, -1, -1, -1};




























static int seq_number = 0;

phantom_device_t * driver_intel_82559_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    intel82559 *nic = NULL;

    printf( DEV_NAME " probe\n");
WW();
    nic = intel82559_new();

    if (nic == NULL)
    {
        if(DEBUG) printf(DEV_NAME "new returned 0\n");
        return 0;
    }


    nic->irq = pci->interrupt;

    int i;
    for (i = 0; i < 6; i++)
    {
        if (pci->base[i] > 0xffff)
        {
            nic->phys_base = (pci->base[i]);
            nic->phys_size = pci->size[i];
            //if(DEBUG) printf( " base 0x%lx, size 0x%lx\n", nic->phys_base, nic->phys_size);
        } else if( pci->base[i] > 0) {
            nic->io_port = pci->base[i];
            //if(DEBUG) printf( "io_port 0x%x\n", nic->io_port);
        }
    }

    printf("Look for " DEV_NAME "at io %X int %d\n", nic->io_port, nic->irq );

    intel82559_stop(nic);
    hal_sleep_msec(10);

    if (intel82559_init(nic, seq_number) < 0)
    {
        printf( DEV_NAME "pcnet_init failed\n");

        //intel82559_delete(nic);
        return 0;
    }

    intel82559_start(nic);


    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = DEV_NAME "network card";
    dev->seq_number = seq_number++;
    dev->drv_private = nic;

    dev->dops.read = intel82559_read;
    dev->dops.write = intel82559_write;
    dev->dops.get_address = intel82559_get_address;

    ifnet *interface;
    if( if_register_interface( IF_TYPE_ETHERNET, &interface, dev) )
    {
        printf(DEV_NAME "Failed to register interface for %s", dev->name );
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

        printf(DEV_NAME "Adding route...");
        int rc;
        if( (rc = ipv4_route_add(
                                 WIRED_NET,
                                 WIRED_NETMASK,
                                 WIRED_ROUTER,
                                 interface->id) ) )
        {
            printf("failed, rc = %d\n", rc);
        }
        else
        {
            printf("ok\n");
        }


        printf(DEV_NAME "Adding default route...");
        if( (rc = ipv4_route_add_default(
                                         WIRED_ROUTER,
                                         interface->id,
                                         DEF_ROUTE_ROUTER
                                        ) ) )
        {
            printf("failed, rc = %d\n", rc);
        }
        else
        {
            printf("ok\n");
        }

    }


WW();
    return dev;

}






static intel82559 *intel82559_new(void)
{
    intel82559 *ret = calloc(1,sizeof(intel82559));
    if(ret == 0)
        panic("oom in intel82559");

    return ret;
}


int intel82559_init(intel82559 *nic, int card_idx)
{
    u_int16_t eeprom[0x100];


    int ioaddr = nic->io_port;
    

    char *dev_name = "Intel 82559";

    //int acpi_idle_state = acpi_set_pwr_state(pdev, ACPI_D0);

    /* Read the station address EEPROM before doing the reset.
     Nominally his should even be done before accepting the device, but
     then we wouldn't have a device name with which to report the error.
     The size test is for 6 bit vs. 8 bit address serial EEPROMs.
     */
    {
        u_int16_t sum = 0;
        int j;
        int read_cmd, ee_size;

        if ((do_eeprom_cmd(ioaddr, EE_READ_CMD << 24, 27) & 0xffe0000)
            == 0xffe0000) {
            ee_size = 0x100;
            read_cmd = EE_READ_CMD << 24;
        } else {
            ee_size = 0x40;
            read_cmd = EE_READ_CMD << 22;
        }

        int i;
        for (j = 0, i = 0; i < ee_size; i++) {
            u_int16_t value = do_eeprom_cmd(ioaddr, read_cmd | (i << 16), 27);
            eeprom[i] = value;
            sum += value;
            if (i < 3) {
                //dev->dev_addr[j++] = value;
                //dev->dev_addr[j++] = value >> 8;
                //printf("Mac %X:%X:", value & 0xFF, (value >> 8) & 0xFF );
                nic->mac_addr[j++] = value;
                nic->mac_addr[j++] = value >> 8;
            }
        }
        printf("\n");
        if (sum != 0xBABA)
            printf("Warning: %s: Invalid EEPROM checksum %#4.4x, "
                   "check settings before activating this device!\n",
                   dev_name, sum);
        /* Don't fail as the EEPro may actually be
         usable, especially if the MAC address is set later. */
    }

    /* Reset the chip: stop Tx and Rx processes and clear counters.
     This takes less than 10usec and will easily finish before the next
     action. */
    outl(ioaddr + SCBPort, PortReset);

    printf("Found %s: %s%s at %#3lx, ", dev_name,
           eeprom[3] & 0x0100 ? "OEM " : "",
           "?",//pci_id_tbl[chip_idx].name,
           ioaddr);

    {
    int i;
    for (i = 0; i < 5; i++)
        printk("%2.2X:", nic->mac_addr[i]);
    printk("%2.2X, IRQ %d.\n", nic->mac_addr[i], nic->irq);
    }


    outl(ioaddr + SCBPort, PortReset);

    /* Return the chip to its original power state. */
    //acpi_set_pwr_state(pdev, acpi_idle_state);

    /* We do a request_region() only to register /proc/ioports info. */
    //request_region(ioaddr, pci_id_tbl[chip_idx].io_size, dev->name);


    //nic->priv_addr = priv_mem;
    //nic->pci_dev = pdev;
    //nic->chip_id = chip_idx;

    //nic->drv_flags = pci_id_tbl[chip_idx].drv_flags;
    nic->drv_flags = ResetMII; // For i82559ER

    //nic->acpi_pwr = acpi_idle_state;
    nic->msg_level = 0xFF; // (1 << debug) - 1;
    //nic->rx_copybreak = rx_copybreak;
    nic->max_interrupt_work = 20; // events to process per interrupt  //max_interrupt_work;
    nic->multicast_filter_limit = 64; //multicast_filter_limit;

    nic->full_duplex = 1; // option >= 0 && (option & 0x220) ? 1 : 0;
    if (card_idx >= 0) {
        if (full_duplex[card_idx] >= 0)
            nic->full_duplex = full_duplex[card_idx];
    }
    nic->default_port = 0; // option >= 0 ? (option & 0x0f) : 0;
    if (nic->full_duplex)
        nic->medialock = 1;

    nic->phy[0] = eeprom[6];
    nic->phy[1] = eeprom[7];
    nic->rx_bug = (eeprom[3] & 0x03) == 3 ? 0 : 1;

    if (nic->rx_bug)
        printk(KERN_INFO "  Receiver lock-up workaround activated.\n");



    return -1;
}



static int intel82559_read(struct phantom_device *dev, void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;

    return -1;
}

static int intel82559_write(struct phantom_device *dev, const void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;

    return -1;
}

static int intel82559_get_address(struct phantom_device *dev, void *buf, int len)
{
    intel82559 *nic = (intel82559 *)dev->drv_private;
    int err = NO_ERROR;

    if(!nic)        return ERR_IO_ERROR;

    if(len >= (int)sizeof(nic->mac_addr)) {
        memcpy(buf, nic->mac_addr, sizeof(nic->mac_addr));
    } else {
        err = ERR_VFS_INSUFFICIENT_BUF;
    }

    return err;
}


static int intel82559_start(intel82559 *nic)
{
    (void) nic;
    return -1;
}

static int intel82559_stop(intel82559 *nic)
{
    (void) nic;
    return -1;
}































#define eeprom_delay(ee_addr)	inw(ee_addr)

static int do_eeprom_cmd(long ioaddr, int cmd, int cmd_len)
{
	unsigned retval = 0;
	long ee_addr = ioaddr + SCBeeprom;

	outw(ee_addr, EE_ENB | EE_SHIFT_CLK);

	/* Shift the command bits out. */
	do {
		short dataval = (cmd & (1 << cmd_len)) ? EE_WRITE_1 : EE_WRITE_0;
		outw(ee_addr, dataval);
		eeprom_delay(ee_addr);
		outw(ee_addr, dataval | EE_SHIFT_CLK);
		eeprom_delay(ee_addr);
		retval = (retval << 1) | ((inw(ee_addr) & EE_DATA_READ) ? 1 : 0);
	} while (--cmd_len >= 0);
	outw(ee_addr, EE_ENB);

	/* Terminate the EEPROM access. */
	outw(ee_addr, EE_ENB & ~EE_CS );
	return retval;
}

#if 0
static int mdio_read(struct net_device *dev, int phy_id, int location)
{
	long ioaddr = dev->base_addr;
	int val, boguscnt = 64*10;		/* <64 usec. to complete, typ 27 ticks */

	outl(0x08000000 | (location<<16) | (phy_id<<21), ioaddr + SCBCtrlMDI);
	do {
		val = inl(ioaddr + SCBCtrlMDI);
		if (--boguscnt < 0) {
			printk(KERN_ERR "%s: mdio_read() timed out with val = %8.8x.\n",
				   dev->name, val);
			break;
		}
	} while (! (val & 0x10000000));
	return val & 0xffff;
}

static int mdio_write(long ioaddr, int phy_id, int location, int value)
{
	int val, boguscnt = 64*10;		/* <64 usec. to complete, typ 27 ticks */
	outl(0x04000000 | (location<<16) | (phy_id<<21) | value,
		 ioaddr + SCBCtrlMDI);
	do {
		val = inl(ioaddr + SCBCtrlMDI);
		if (--boguscnt < 0) {
			printk(KERN_ERR" mdio_write() timed out with val = %8.8x.\n", val);
			break;
		}
	} while (! (val & 0x10000000));
	return val & 0xffff;
}
#endif

#endif // HAVE_NET
