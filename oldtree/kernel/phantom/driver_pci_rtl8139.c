/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Realtek rtl 8139 net driver.
 *
**/

#include <kernel/config.h>
#if HAVE_NET && defined(ARCH_ia32)

#define DEBUG_MSG_PREFIX "RTL8139"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


/*
** original code copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <hal.h>
#include <phantom_libc.h>
#include <kernel/vm.h>
#include <i386/pio.h>
//#include "driver_map.h"
#include <device.h>
#include <kernel/drivers.h>

#include <kernel/ethernet_defs.h>

#include "newos.h"
#include "net.h"

#include <dev/pci/rtl8139_dev.h>
#include <dev/pci/rtl8139_priv.h>
#include <dev/pci/if_rlreg.h>

static int DEBUG = 0;

// temp!
#define INT_RESCHEDULE 1

//#define acquire_spinlock(sl) hal_spin_lock(sl)
//#define release_spinlock(sl) hal_spin_unlock(sl)


#define WW()
//#define WW() getchar()



static void rtl8139_stop(rtl8139 *rtl);
int rtl8139_init(rtl8139 *rtl);

static void rtl8139_int(void* data);
static void rtl8139_softint(void* data);





#define DEV_NAME "RTL8139 "

#define WIRED_ADDRESS 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 124)
#define WIRED_NETMASK 	0xffffff00
#define WIRED_BROADCAST IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0xFF)

#define WIRED_NET 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0)
#define WIRED_ROUTER    IPV4_DOTADDR_TO_ADDR(10, 0, 2, 124)

#define DEF_ROUTE_ROUTER        IPV4_DOTADDR_TO_ADDR(10, 0, 2, 2)



static int rtl8139_read( struct phantom_device *dev, void *buf, int len)
{
    rtl8139 *nic = (rtl8139 *)dev->drv_private;

    if(len < ETHERNET_MAX_SIZE)
        return ERR_VFS_INSUFFICIENT_BUF;
    return rtl8139_rx(nic, buf, len);
}

static int rtl8139_write(struct phantom_device *dev, const void *buf, int len)
{
    rtl8139 *nic = (rtl8139 *)dev->drv_private;

    if(len < 0)
        return ERR_INVALID_ARGS;

    rtl8139_xmit(nic, buf, len);
    return len;
}


static int rtl8139_get_address( struct phantom_device *dev, void *buf, int len)
{
    rtl8139 *nic = (rtl8139 *)dev->drv_private;
    int err = NO_ERROR;

    if(!nic)        return ERR_IO_ERROR;

    if(len >= (int)sizeof(nic->mac_addr)) {
        memcpy(buf, nic->mac_addr, sizeof(nic->mac_addr));
    } else {
        err = ERR_VFS_INSUFFICIENT_BUF;
    }

    return err;
}












static int seq_number = 0;


/*
static rtl8139 *rtl8139_new()
{
    rtl8139 *rtl;

    rtl = malloc(sizeof(rtl8139));
    if(rtl == NULL) {
        return 0;
    }

    memset(rtl, 0, sizeof(rtl8139));

    return rtl;
}
*/



phantom_device_t * driver_rtl_8139_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;
    rtl8139 * nic = NULL;

    SHOW_FLOW0( 1, "probe" );

    //nic = rtl8139_new();
    nic = calloc(1, sizeof(rtl8139));
    if (nic == NULL)
    {
        SHOW_ERROR0( 0, "out of mem");
        return 0;
    }

    nic->irq = pci->interrupt;

    int i;
    for (i = 0; i < 6; i++)
    {
        if (pci->base[i] > 0xffff)
        {
            nic->phys_base = pci->base[i];
            nic->phys_size = pci->size[i];
            SHOW_INFO( 0,  "base 0x%lx, size 0x%lx", nic->phys_base, nic->phys_size );
        } else if( pci->base[i] > 0) {
            nic->io_port = pci->base[i];
            SHOW_INFO( 0,  "io_port 0x%x", nic->io_port );
        }
    }

    SHOW_FLOW0( 1, "stop" );
    rtl8139_stop(nic);
    hal_sleep_msec(10);

    SHOW_FLOW0( 1, "init");
    if (rtl8139_init(nic) < 0)
    {
        SHOW_ERROR0( 0, "init failed");
        return 0;
    }

    //rtl8139_start(nic);

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = DEV_NAME " network card";
    dev->seq_number = seq_number++;
    dev->drv_private = nic;

    dev->dops.read = rtl8139_read;
    dev->dops.write = rtl8139_write;
    dev->dops.get_address = rtl8139_get_address;

    ifnet *interface;
    if( if_register_interface( IF_TYPE_ETHERNET, &interface, dev) )
    {
        SHOW_ERROR( 0,  "Failed to register interface for %s", dev->name );
    }
    else
    {
        if_simple_setup( interface, WIRED_ADDRESS, WIRED_NETMASK, WIRED_BROADCAST, WIRED_NET, WIRED_ROUTER, DEF_ROUTE_ROUTER );
    }

    return dev;

}













#define RTL_WRITE_8(rtl, reg, dat) \
    outb( (rtl)->io_port + (reg), dat)
#define RTL_WRITE_16(rtl, reg, dat) \
    outw( (rtl)->io_port + (reg), dat)
#define RTL_WRITE_32(rtl, reg, dat) \
    outl( (rtl)->io_port + (reg), dat)

#define RTL_READ_8(rtl, reg) \
    inb((rtl)->io_port + (reg))
#define RTL_READ_16(rtl, reg) \
    inw((rtl)->io_port + (reg))
#define RTL_READ_32(rtl, reg) \
    inl((rtl)->io_port + (reg))



#define TAILREG_TO_TAIL(in) \
    (u_int16_t)(((u_int32_t)(in) + 16) % 0x10000)
#define TAIL_TO_TAILREG(in) \
    (u_int16_t)((u_int16_t)(in) - 16)

#define MYRT_INTS (RT_INT_PCIERR | RT_INT_RX_ERR | RT_INT_RX_OK | RT_INT_TX_ERR | RT_INT_TX_OK | RT_INT_RXBUF_OVERFLOW)

//static int rtl8139_int(void*);

//static rtl8139 *rtl;
# if 0
int rtl8139_detect()
{
#if 1
    return 1;
#else
    int i;
    //pci_module_hooks *pci;
    //pci_info pinfo;
    char foundit = false;

    /*if(module_get(PCI_BUS_MODULE_NAME, 0, (void **)&pci) < 0) {
        printf("rtl8139_detect: no pci bus found..\n");
        return -1;
    }

    for(i = 0; pci->get_nth_pci_info(i, &pinfo) >= NO_ERROR; i++) {
        if(pinfo.vendor_id == RT_VENDORID && pinfo.device_id == RT_DEVICEID_8139) {
            foundit = true;
            break;
        }
        }*/

    pcici_t tag;

    {
        unsigned char device;
        char *vendor, *devname;

        for (device = 0; device < pci_maxdevice; device++) {
            unsigned long id;
            tag = pcibus_tag (0,device,0);
            id = pcibus_read (tag, 0);
            if (id && id != 0xfffffffful) {

                if(1)
                {
                    vendor = get_pci_vendor(id & 0xFFFF);
                    devname = get_pci_device(id & 0xFFFF, (id>>16) & 0xFFFF);

                    printf ("PCI device %d: id=%08lx (%s: %s)\n", device, id, vendor, devname);
                }

                if( (id & 0xFFFF) == RT_VENDORID && ((id>>16) & 0xFFFF) == RT_DEVICEID_8139) {
                    foundit = true;
                    break;
                }

            }
        }

    }




    if(!foundit) {
        printf("rtl8139_detect: didn't find device on pci bus\n");
        return -1;
    }

    // we found one
    printf("rtl8139_detect: found device at pci %d:%d:%d\n", pinfo.bus, pinfo.device, pinfo.function);

    {
        int irq = pcibus_read( tag, RL_PCI_INTLINE );
        int phys_base = pcibus_read( tag, RL_PCI_LOMEM );
        int io_port = pcibus_read( tag, RL_PCI_LOIO );

        printf("rtl8139_detect: irq %d, phys base 0x%X, io base 0x%x\n", irq, phys_base, io_port );
    }

// not impl!
return -1;

    rtl = malloc(sizeof(rtl8139));
    if(rtl == NULL) {
        //return ERR_NO_MEMORY;
        return 1;
    }
    memset(rtl, 0, sizeof(rtl8139));

    /*
    (rtl)->irq = pinfo.u.h0.interrupt_line;
    // find the memory-mapped base
    for(i=0; i<6; i++) {
        if(pinfo.u.h0.base_registers[i] > 0xffff) {
            (rtl)->phys_base = pinfo.u.h0.base_registers[i];
            (rtl)->phys_size = pinfo.u.h0.base_register_sizes[i];
            break;
        } else if(pinfo.u.h0.base_registers[i] > 0) {
            (rtl)->io_port = pinfo.u.h0.base_registers[i];

            if(osenv_io_alloc( pinfo.u.h0.base_registers[i], 256))
            {
                printf("Can't allocate io registers from %d to %d\n",
                       pinfo.u.h0.base_registers[i],
                       pinfo.u.h0.base_registers[i] + 256 );

                // stop me
                (rtl)->phys_base = 0;
                break;
            }

        }
        }
        */
#if 0 // wrong as well :(
    (rtl)->irq = pcibus_read( tag, RL_PCI_INTLINE );

    (rtl)->phys_base = pcibus_read( tag, RL_PCI_LOMEM );

    (rtl)->io_port = pcibus_read( tag, RL_PCI_LOIO );
    if(osenv_io_alloc( (rtl)->io_port, 256))
    {
        printf("Can't allocate io registers from %d to %d\n",
               (rtl)->io_port,
               (rtl)->io_port + 256 );

        // stop me
        (rtl)->phys_base = 0;
        break;
    }
#endif


    if((rtl)->phys_base == 0) {
        free(rtl);
        rtl = NULL;
        return -1;
    }

    printf("detected rtl8139 at irq %d, memory base 0x%lx, size 0x%lx\n", (rtl)->irq, (rtl)->phys_base, (rtl)->phys_size);

    return 0;
#endif
}
#endif





int rtl8139_init(rtl8139 *rtl)
{
    //bigtime_t time;
    int err = -1;
    addr_t temp;

    printf("rtl8139_init: rtl %p\n", rtl);

    //rtl->phys_base = kvtophys();

    //rtl->virt_base = (void *)kphystov( rtl->phys_base );

    /*
    rtl->region = vm_map_physical_memory(vm_get_kernel_aspace_id(), "rtl8139_region", (void **)&rtl->virt_base,
                                         REGION_ADDR_ANY_ADDRESS, rtl->phys_size, LOCK_KERNEL|LOCK_RW, rtl->phys_base);
    if(rtl->region < 0) {
        printf("rtl8139_init: error creating memory mapped region\n");
        err = -1;
        goto err;
        }*/

    int phys_size_pages = (((rtl->phys_size)-1)/4096)+1;
    if(hal_alloc_vaddress( (void **)&rtl->virt_base, phys_size_pages ))
        panic(DEV_NAME "out of addr space");

    hal_pages_control(
                      rtl->phys_base, rtl->virt_base,
                      phys_size_pages,
                      page_map, page_rw );


    printf("rtl8139 mapped at address 0x%lx\n", rtl->virt_base);

    // try to reset the device
    int reset_sleep_times = 100;
    RTL_WRITE_8(rtl, RT_CHIPCMD, RT_CMD_RESET);
    do {
        hal_sleep_msec(10); // 10ms
        if(reset_sleep_times-- <= 0 ) {
            err = -1;
            goto err1;
        }
    } while((RTL_READ_8(rtl, RT_CHIPCMD) & RT_CMD_RESET));


    printf( DEV_NAME "allocating irq %d\n", rtl->irq );
    // set up the interrupt handler
    //int_set_io_interrupt_handler(rtl->irq, &rtl8139_int, rtl, "rtl8139");
    if(hal_irq_alloc( rtl->irq, &rtl8139_int, rtl, HAL_IRQ_SHAREABLE ))
    {
        printf( DEV_NAME "unable to allocate irq %d\n", rtl->irq );
        goto err1;
    }

    rtl->softirq = hal_alloc_softirq();
    if( rtl->softirq < 0 )
    {
        SHOW_ERROR0( 0, "Unable to get softirq" );
        goto err2;
    }

    hal_set_softirq_handler( rtl->softirq, &rtl8139_softint, rtl );

    /* create a rx and tx buf
    rtl->rxbuf_region = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "rtl8139_rxbuf", (void **)&rtl->rxbuf,
                                                   REGION_ADDR_ANY_ADDRESS, 64*1024 + 16, REGION_WIRING_WIRED_CONTIG, LOCK_KERNEL|LOCK_RW);
    rtl->txbuf_region = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "rtl8139_txbuf", (void **)&rtl->txbuf,
                                                   REGION_ADDR_ANY_ADDRESS, 8*1024, REGION_WIRING_WIRED, LOCK_KERNEL|LOCK_RW);
    */

    /*
#define RXBUF_PAGES ((64*1024)/4 + 1)
#define TXBUF_PAGES (8*1024)/4)

    // 64*1024 + 16 bytes <= (64*1024)/4 + 1 pages
    if(hal_alloc_vaddress( (void **)&rtl->rxbuf, RXBUF_PAGES ))
        panic(DEV_NAME "out of addr space");

    if(hal_alloc_vaddress( (void **)&rtl->txbuf, )
        panic(DEV_NAME "out of addr space");
        */

#define RXBUF_SIZE (64*1024 + 16)
#define TXBUF_SIZE 8*1024

    rtl->rxbuf = malloc(RXBUF_SIZE);
    rtl->txbuf = malloc(TXBUF_SIZE);

    if( rtl->rxbuf == 0 || rtl->txbuf == 0)
        panic(DEV_NAME "out of addr space");

    // set up the transmission buf and sem
    //rtl->tx_sem = sem_create(4, "rtl8139_txsem");
    hal_sem_init(&(rtl->tx_sem),"RTL8139 tx");
    hal_sem_init(&(rtl->rx_sem),"RTL8139 rx");
    hal_mutex_init(&rtl->lock,"RTL8139");
    rtl->txbn = 0;
    rtl->last_txbn = 0;
    hal_spin_init( &rtl->reg_spinlock );

    // Or else driver hangs in first tx
    hal_sem_release(&rtl->tx_sem);



    // read the mac address
    rtl->mac_addr[0] = RTL_READ_8(rtl, RT_IDR0);
    rtl->mac_addr[1] = RTL_READ_8(rtl, RT_IDR0 + 1);
    rtl->mac_addr[2] = RTL_READ_8(rtl, RT_IDR0 + 2);
    rtl->mac_addr[3] = RTL_READ_8(rtl, RT_IDR0 + 3);
    rtl->mac_addr[4] = RTL_READ_8(rtl, RT_IDR0 + 4);
    rtl->mac_addr[5] = RTL_READ_8(rtl, RT_IDR0 + 5);

    printf("rtl8139: mac addr %x:%x:%x:%x:%x:%x\n",
            rtl->mac_addr[0], rtl->mac_addr[1], rtl->mac_addr[2],
            rtl->mac_addr[3], rtl->mac_addr[4], rtl->mac_addr[5]);

    // enable writing to the config registers
    RTL_WRITE_8(rtl, RT_CFG9346, 0xc0);

    // reset config 1
    RTL_WRITE_8(rtl, RT_CONFIG1, 0);

    // Enable receive and transmit functions
    RTL_WRITE_8(rtl, RT_CHIPCMD, RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);

    // Set Rx FIFO threashold to 256, Rx size to 64k+16, 256 byte DMA burst
    RTL_WRITE_32(rtl, RT_RXCONFIG, 0x00009c00);

    // Set Tx 256 byte DMA burst
    RTL_WRITE_32(rtl, RT_TXCONFIG, 0x03000400);

    // Turn off lan-wake and set the driver-loaded bit
    RTL_WRITE_8(rtl, RT_CONFIG1, (RTL_READ_8(rtl, RT_CONFIG1) & ~0x30) | 0x20);

    // Enable FIFO auto-clear
    RTL_WRITE_8(rtl, RT_CONFIG4, RTL_READ_8(rtl, RT_CONFIG4) | 0x80);

    // go back to normal mode
    RTL_WRITE_8(rtl, RT_CFG9346, 0);

    // Setup RX buffers
    *(int *)rtl->rxbuf = 0;
    //vm_get_page_mapping(vm_get_kernel_aspace_id(), rtl->rxbuf, &temp);
    temp = kvtophys( rtl->rxbuf );
    printf("rx buffer will be at 0x%lx\n", temp);
    RTL_WRITE_32(rtl, RT_RXBUF, temp);

    // Setup TX buffers
    printf("tx buffer (virtual) is at 0x%lx\n", rtl->txbuf);
    *(int *)rtl->txbuf = 0;
    //vm_get_page_mapping(vm_get_kernel_aspace_id(), rtl->txbuf, &temp);

    temp = kvtophys( rtl->txbuf );

    RTL_WRITE_32(rtl, RT_TXADDR0, temp);
    RTL_WRITE_32(rtl, RT_TXADDR1, temp + 2*1024);
    printf("first half of txbuf at 0x%lx\n", temp);
    *(int *)(rtl->txbuf + 4*1024) = 0;
    //vm_get_page_mapping(vm_get_kernel_aspace_id(), rtl->txbuf + 4*1024, &temp);
    temp = kvtophys( rtl->txbuf + 4*1024 );
    RTL_WRITE_32(rtl, RT_TXADDR2, temp);
    RTL_WRITE_32(rtl, RT_TXADDR3, temp + 2*1024);
    printf("second half of txbuf at 0x%lx\n", temp);

    /*
     RTL_WRITE_32(rtl, RT_TXSTATUS0, RTL_READ_32(rtl, RT_TXSTATUS0) | 0xfffff000);
     RTL_WRITE_32(rtl, RT_TXSTATUS1, RTL_READ_32(rtl, RT_TXSTATUS1) | 0xfffff000);
     RTL_WRITE_32(rtl, RT_TXSTATUS2, RTL_READ_32(rtl, RT_TXSTATUS2) | 0xfffff000);
     RTL_WRITE_32(rtl, RT_TXSTATUS3, RTL_READ_32(rtl, RT_TXSTATUS3) | 0xfffff000);
     */
    // Reset RXMISSED counter
    RTL_WRITE_32(rtl, RT_RXMISSED, 0);

    // Enable receiving broadcast and physical match packets
    //	RTL_WRITE_32(rtl, RT_RXCONFIG, RTL_READ_32(rtl, RT_RXCONFIG) | 0x0000000a);
    RTL_WRITE_32(rtl, RT_RXCONFIG, RTL_READ_32(rtl, RT_RXCONFIG) | 0x0000000f);

    // Filter out all multicast packets
    RTL_WRITE_32(rtl, RT_MAR0, 0);
    RTL_WRITE_32(rtl, RT_MAR0 + 4, 0);

    // Disable all multi-interrupts
    RTL_WRITE_16(rtl, RT_MULTIINTR, 0);

    RTL_WRITE_16(rtl, RT_INTRMASK, MYRT_INTS);
    //	RTL_WRITE_16(rtl, RT_INTRMASK, 0x807f);

    // Enable RX/TX once more
    RTL_WRITE_8(rtl, RT_CHIPCMD, RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);

    RTL_WRITE_8(rtl, RT_CFG9346, 0);

    return 0;

err2:
    hal_irq_free( rtl->irq, &rtl8139_int, rtl );

err1:
    //vm_delete_region(vm_get_kernel_aspace_id(), rtl->region);
//err:
    return err;
}

static void rtl8139_stop(rtl8139 *rtl)
{
    // stop the rx and tx and mask all interrupts
    RTL_WRITE_8(rtl, RT_CHIPCMD, RT_CMD_RESET);
    RTL_WRITE_16(rtl, RT_INTRMASK, 0);
}

static void rtl8139_resetrx(rtl8139 *rtl)
{
    rtl8139_stop(rtl);

    // reset the rx pointers
    RTL_WRITE_16(rtl, RT_RXBUFTAIL, TAIL_TO_TAILREG(0));
    RTL_WRITE_16(rtl, RT_RXBUFHEAD, 0);

    // start it back up
    RTL_WRITE_16(rtl, RT_INTRMASK, MYRT_INTS);

    // Enable RX/TX once more
    RTL_WRITE_8(rtl, RT_CHIPCMD, RT_CMD_RX_ENABLE | RT_CMD_TX_ENABLE);
}



static void rtl8139_dumptxstate(rtl8139 *rtl)
{
    printf("tx state:\n");
    printf("\ttxbn %d\n", rtl->txbn);
    printf("\ttxstatus 0 0x%x\n", RTL_READ_32(rtl, RT_TXSTATUS0));
    printf("\ttxstatus 1 0x%x\n", RTL_READ_32(rtl, RT_TXSTATUS1));
    printf("\ttxstatus 2 0x%x\n", RTL_READ_32(rtl, RT_TXSTATUS2));
    printf("\ttxstatus 3 0x%x\n", RTL_READ_32(rtl, RT_TXSTATUS3));
}

void rtl8139_xmit(rtl8139 *rtl, const char *ptr, ssize_t len)
{
    //printf("rtl8139_xmit...");
    //restart:
    //hal_sem_acquire(rtl->tx_sem, 1);
    hal_sem_acquire(&rtl->tx_sem);

    hal_mutex_lock(&rtl->lock);

#if 0
    {
    printf("XMIT %d %x (%d)\n",rtl->txbn, ptr, len);

    printf("dumping packet (hex):");
    int i;
    for(i=0; i<len; i++) {
        if(i%8 == 0)
            printf("\n");
        printf("%02x ", 0xFFu & ptr[i]);
    }
    printf("\n");
    }
#endif

    int_disable_interrupts();
    hal_spin_lock(&rtl->reg_spinlock);

#if 0
    /* wait for clear-to-send */
    if(!(RTL_READ_32(rtl, RT_TXSTATUS0 + rtl->txbn*4) & RT_TX_HOST_OWNS)) {
        printf("rtl8139_xmit: no txbuf free\n");
        rtl8139_dumptxstate(rtl);
        hal_spin_unlock(&rtl->reg_spinlock);
        int_restore_interrupts();
        hal_mutex_unlock(&rtl->lock);
        hal_sem_release(rtl->tx_sem, 1);
        goto restart;
    }
#endif

    memcpy((void*)(rtl->txbuf + rtl->txbn * 0x800), ptr, len);
    if(len < ETHERNET_MIN_SIZE)
        len = ETHERNET_MIN_SIZE;

    RTL_WRITE_32(rtl, RT_TXSTATUS0 + rtl->txbn*4, len | 0x80000);
    if(++rtl->txbn >= 4)
        rtl->txbn = 0;

    hal_spin_unlock(&rtl->reg_spinlock);
    int_restore_interrupts();

    hal_mutex_unlock(&rtl->lock);
}

typedef struct rx_entry {
    volatile u_int16_t status;
    volatile u_int16_t len;
    volatile u_int8_t data[1];
} rx_entry;

ssize_t rtl8139_rx(rtl8139 *rtl, char *buf, ssize_t buf_len)
{
    rx_entry *entry;
    u_int32_t tail;
    u_int16_t len;
    int rc;
    bool release_sem = false;

    if(DEBUG)	printf("rtl8139_rx: entry\n");

    if(buf_len < 1500)
        return -1;

restart:
    //hal_sem_acquire(&rtl->rx_sem, 1);
    hal_sem_acquire(&rtl->rx_sem);
    hal_mutex_lock(&rtl->lock);

    int_disable_interrupts();
    hal_spin_lock(&rtl->reg_spinlock);

    tail = TAILREG_TO_TAIL(RTL_READ_16(rtl, RT_RXBUFTAIL));
    //	printf("tailreg = 0x%x, actual tail 0x%x\n", RTL_READ_16(rtl, RT_RXBUFTAIL), tail);
    if(tail == RTL_READ_16(rtl, RT_RXBUFHEAD)) {
        hal_spin_unlock(&rtl->reg_spinlock);
        int_restore_interrupts();
        hal_mutex_unlock(&rtl->lock);
        goto restart;
    }

    if(RTL_READ_8(rtl, RT_CHIPCMD) & RT_CMD_RX_BUF_EMPTY) {
        hal_spin_unlock(&rtl->reg_spinlock);
        int_restore_interrupts();
        hal_mutex_unlock(&rtl->lock);
        goto restart;
    }

    // grab another buffer
    entry = (rx_entry *)((u_int8_t *)rtl->rxbuf + tail);
    //	printf("entry->status = 0x%x\n", entry->status);
    if(DEBUG)	printf("entry->len = 0x%x\n", entry->len);

    // see if it's an unfinished buffer
    if(entry->len == 0xfff0) {
        hal_spin_unlock(&rtl->reg_spinlock);
        int_restore_interrupts();
        hal_mutex_unlock(&rtl->lock);
        goto restart;
    }

    // figure the len that we need to copy
    len = entry->len - 4; // minus the crc

    // see if we got an error
    if((entry->status & RT_RX_STATUS_OK) == 0 || len > ETHERNET_MAX_SIZE) {
        // error, lets reset the card
        rtl8139_resetrx(rtl);
        hal_spin_unlock(&rtl->reg_spinlock);
        int_restore_interrupts();
        hal_mutex_unlock(&rtl->lock);
        goto restart;
    }

    // copy the buffer
    if(len > buf_len) {
        printf("rtl8139_rx: packet too large for buffer (len %d, buf_len %ld)\n", len, (long)buf_len);
        RTL_WRITE_16(rtl, RT_RXBUFTAIL, TAILREG_TO_TAIL(RTL_READ_16(rtl, RT_RXBUFHEAD)));
        rc = ERR_TOO_BIG;
        release_sem = true;
        goto out;
    }
    if(tail + len > 0xffff) {
        //		printf("packet wraps around\n");
        memcpy(buf, (const void *)&entry->data[0], 0x10000 - (tail + 4));
        memcpy((u_int8_t *)buf + 0x10000 - (tail + 4), (const void *)rtl->rxbuf, len - (0x10000 - (tail + 4)));
    } else {
        memcpy(buf, (const void *)&entry->data[0], len);
    }
    rc = len;

    // calculate the new tail
    tail = ((tail + entry->len + 4 + 3) & ~3) % 0x10000;
    //	printf("new tail at 0x%x, tailreg will say 0x%x\n", tail, TAIL_TO_TAILREG(tail));
    RTL_WRITE_16(rtl, RT_RXBUFTAIL, TAIL_TO_TAILREG(tail));

    if(tail != RTL_READ_16(rtl, RT_RXBUFHEAD)) {
        // we're at last one more packet behind
        release_sem = true;
    }

out:
    hal_spin_unlock(&rtl->reg_spinlock);
    int_restore_interrupts();

    if(release_sem)
    {
        //hal_sem_release(&rtl->rx_sem, 1);
        hal_sem_release(&rtl->rx_sem);
    }
    hal_mutex_unlock(&rtl->lock);

#if 0
    {
        int i;
        printf("RX %x (%d)\n", buf, len);

        printf("dumping packet:");
        for(i=0; i<len; i++) {
            if(i%8 == 0)
                printf("\n");
            printf("0x%02x ", buf[i]);
        }
        printf("\n");
    }
#endif

    return rc;
}

static int rtl8139_rxint(rtl8139 *rtl, u_int16_t int_status)
{
    (void) int_status;

    int rc = 0;//INT_NO_RESCHEDULE;

    if(DEBUG > 1)	printf("rx\n");

    if(DEBUG > 2)	printf("buf 0x%x, head 0x%x, tail 0x%x\n",
                               RTL_READ_32(rtl, RT_RXBUF), RTL_READ_16(rtl, RT_RXBUFHEAD), RTL_READ_16(rtl, RT_RXBUFTAIL));
    //	printf("BUF_EMPTY = %d\n", RTL_READ_8(rtl, RT_CHIPCMD) & RT_CMD_RX_BUF_EMPTY);

    if(!(RTL_READ_8(rtl, RT_CHIPCMD) & RT_CMD_RX_BUF_EMPTY))
    {
        //hal_sem_release_etc(rtl->rx_sem, 1, SEM_FLAG_NO_RESCHED);
        //hal_sem_release(&rtl->rx_sem);
        //rc = INT_RESCHEDULE;
        rtl->rx_rq++;
        hal_request_softirq( rtl->softirq );
    }

    return rc;
}

static int rtl8139_txint(rtl8139 *rtl, u_int16_t int_status)
{
    u_int32_t txstat;
    int i;
    int rc = 0; //INT_NO_RESCHEDULE;

    // transmit ok
    printf("tx %d\n", int_status);
    if(int_status & RT_INT_TX_ERR) {
        printf("err tx int:\n");
        rtl8139_dumptxstate(rtl);
    }

    for(i=0; i<4; i++) {
        if(i > 0 && rtl->last_txbn == rtl->txbn)
            break;
        txstat = RTL_READ_32(rtl, RT_TXSTATUS0 + rtl->last_txbn*4);
        //		printf("txstat[%d] = 0x%x\n", rtl->last_txbn, txstat);

        if((txstat & (RT_TX_STATUS_OK | RT_TX_UNDERRUN | RT_TX_ABORTED)) == 0)
            break;

        if(++rtl->last_txbn >= 4)
            rtl->last_txbn = 0;
        //hal_sem_release_etc(rtl->tx_sem, 1, SEM_FLAG_NO_RESCHED);
        //hal_sem_release(&rtl->tx_sem);
        //rc = INT_RESCHEDULE;
        rtl->tx_rq++;
        hal_request_softirq( rtl->softirq );
    }

    return rc;
}

static void rtl8139_softint(void* data)
{
    rtl8139 *rtl = (rtl8139 *)data;

    SHOW_FLOW0( 11, "softint" );

    if(rtl->rx_rq)
    {
        hal_sem_release(&rtl->rx_sem);
        //rc = INT_RESCHEDULE;
        rtl->rx_rq--;
    }


    if(rtl->tx_rq)
    {
        hal_sem_release(&rtl->tx_sem);
        //rc = INT_RESCHEDULE;
        rtl->tx_rq--;
    }

}

static void rtl8139_int(void* data)
{
    int rc = 0; //INT_NO_RESCHEDULE;
    rtl8139 *rtl = (rtl8139 *)data;

    //printf(DEV_NAME "interrupt\n");

    hal_spin_lock(&rtl->reg_spinlock);
    // Disable interrupts
    RTL_WRITE_16(rtl, RT_INTRMASK, 0);

    for(;;) {
        u_int16_t status = RTL_READ_16(rtl, RT_INTRSTATUS);
        if(status)
            RTL_WRITE_16(rtl, RT_INTRSTATUS, status);
        else
            break;

        if(status & RT_INT_TX_OK || status & RT_INT_TX_ERR) {
            if(rtl8139_txint(rtl, status) == INT_RESCHEDULE)
                rc = INT_RESCHEDULE;
        }
        if(status & RT_INT_RX_ERR || status & RT_INT_RX_OK) {
            if(rtl8139_rxint(rtl, status) == INT_RESCHEDULE)
                rc = INT_RESCHEDULE;
        }
        if(status & RT_INT_RXBUF_OVERFLOW) {
            printf("RX buffer overflow!\n");
            printf("buf 0x%x, head 0x%x, tail 0x%x\n",
                    RTL_READ_32(rtl, RT_RXBUF), RTL_READ_16(rtl, RT_RXBUFHEAD), RTL_READ_16(rtl, RT_RXBUFTAIL));
            RTL_WRITE_32(rtl, RT_RXMISSED, 0);
            RTL_WRITE_16(rtl, RT_RXBUFTAIL, TAIL_TO_TAILREG(RTL_READ_16(rtl, RT_RXBUFHEAD)));
        }
        if(status & RT_INT_RXFIFO_OVERFLOW) {
            printf("RX fifo overflow!\n");
        }
        if(status & RT_INT_RXFIFO_UNDERRUN) {
            printf("RX fifo underrun\n");
        }
    }

    // reenable interrupts
    RTL_WRITE_16(rtl, RT_INTRMASK, MYRT_INTS);

    hal_spin_unlock(&rtl->reg_spinlock);

    //return rc;
}




#endif // HAVE_NET

