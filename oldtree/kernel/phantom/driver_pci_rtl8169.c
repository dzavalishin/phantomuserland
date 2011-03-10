/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Realtek rtl 8169 net driver.
 *
 **/

#include <kernel/config.h>
#if HAVE_NET && defined(ARCH_ia32)

#define DEBUG_MSG_PREFIX "RTL8169 "
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


/*
 ** Original code copyright 2001-2006, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */

#include "rtl8169_priv.h"
#include "rtl8169_dev.h"

#include <hal.h>
#include <phantom_libc.h>
#include <i386/pio.h>
#include <device.h>

#include <kernel/vm.h>
#include <kernel/drivers.h>
#include <kernel/ethernet_defs.h>

#include "newos.h"
#include "net.h"


#if 1
#define RTL_WRITE_8(r, reg, dat) \
    *(volatile uint8 *)((r)->virt_base + (reg)) = (dat)
#define RTL_WRITE_16(r, reg, dat) \
    *(volatile uint16 *)((r)->virt_base + (reg)) = (dat)
#define RTL_WRITE_32(r, reg, dat) \
    *(volatile uint32 *)((r)->virt_base + (reg)) = (dat)

#define RTL_READ_8(r, reg) \
    *(volatile uint8 *)((r)->virt_base + reg)
#define RTL_READ_16(r, reg) \
    *(volatile uint16 *)((r)->virt_base + reg)
#define RTL_READ_32(r, reg) \
    *(volatile uint32 *)((r)->virt_base + reg)
#else
#define RTL_WRITE_8(r, reg, dat) \
    out8(dat, (r)->io_port + (reg))
#define RTL_WRITE_16(r, reg, dat) \
    out16(dat, (r)->io_port + (reg))
#define RTL_WRITE_32(r, reg, dat) \
    out32(dat, (r)->io_port + (reg))

#define RTL_READ_8(r, reg) \
    in8((r)->io_port + (reg))
#define RTL_READ_16(r, reg) \
    in16((r)->io_port + (reg))
#define RTL_READ_32(r, reg) \
    in32((r)->io_port + (reg))
#endif

#define RTL_SETBITS_8(r, reg, bits) \
    RTL_WRITE_8(r, reg, RTL_READ_8(r, reg) | (bits))
#define RTL_SETBITS_16(r, reg, bits) \
    RTL_WRITE_16(r, reg, RTL_READ_16(r, reg) | (bits))
#define RTL_SETBITS_32(r, reg, bits) \
    RTL_WRITE_32(r, reg, RTL_READ_32(r, reg) | (bits))
#define RTL_CLRBITS_8(r, reg, bits) \
    RTL_WRITE_8(r, reg, RTL_READ_8(r, reg) | ~(bits))
#define RTL_CLRBITS_16(r, reg, bits) \
    RTL_WRITE_16(r, reg, RTL_READ_16(r, reg) | ~(bits))
#define RTL_CLRBITS_32(r, reg, bits) \
    RTL_WRITE_32(r, reg, RTL_READ_32(r, reg) | ~(bits))

#define RXDESC(r, num) ((r)->rxdesc[num])
#define TXDESC(r, num) ((r)->txdesc[num])
#define RXDESC_PHYS(r, num) ((r)->rxdesc_phys + (num) * sizeof(rtl_rx_descriptor))
#define TXDESC_PHYS(r, num) ((r)->txdesc_phys + (num) * sizeof(rtl_tx_descriptor))

#define RXBUF(r, num) (&(r)->rxbuf[(num) * BUFSIZE_PER_FRAME])
#define TXBUF(r, num) (&(r)->txbuf[(num) * BUFSIZE_PER_FRAME])

#define RXBUF_P(r, num) ((r)->rxbuf_phys + (num) * BUFSIZE_PER_FRAME)
#define TXBUF_P(r, num) ((r)->txbuf_phys + (num) * BUFSIZE_PER_FRAME)


static int rtl8169_int(void*);

struct vendor_dev_match {
    int vendor;
    int device;
};
static const struct vendor_dev_match match[] = {
    { 0x10ec, 0x8169 },
    { 0x10ec, 0x8129 },
};

/*
static addr_t vtophys(const void *virt)
{
    addr_t phys = 0;

    vm_get_page_mapping(vm_get_kernel_aspace_id(), (addr_t)virt, &phys);
    return phys;
}
*/

static inline int inc_rx_idx_full(rtl8169 *r)
{
    return (r->rx_idx_full = (r->rx_idx_full + 1) % NUM_RX_DESCRIPTORS);
}

static inline int inc_rx_idx_free(rtl8169 *r)
{
    return (r->rx_idx_free = (r->rx_idx_free + 1) % NUM_RX_DESCRIPTORS);
}

static inline int inc_tx_idx_full(rtl8169 *r)
{
    return (r->tx_idx_full = (r->tx_idx_full + 1) % NUM_TX_DESCRIPTORS);
}

static inline int inc_tx_idx_free(rtl8169 *r)
{
    return (r->tx_idx_free = (r->tx_idx_free + 1) % NUM_TX_DESCRIPTORS);
}

#if 0
int rtl8169_detect(rtl8169 **rtl8169_list)
{
    unsigned int i, j;
    //pci_module_hooks *pci;
    //pci_info pinfo;
    rtl8169 *r;

    *rtl8169_list = NULL;
    if(module_get(PCI_BUS_MODULE_NAME, 0, (void **)(void *)&pci) < 0) {
        SHOW_INFO0(1, "rtl8169_detect: no pci bus found..\n");
        return -1;
    }

    for (i = 0; pci->get_nth_pci_info(i, &pinfo) >= NO_ERROR; i++) {
        for (j = 0; j < sizeof(match)/sizeof(match[0]); j++) {
            if (pinfo.vendor_id == match[j].vendor && pinfo.device_id == match[j].device) {
                // we found one
                SHOW_INFO(1, "rtl8169_detect: found device at pci %d:%d:%d\n", pinfo.bus, pinfo.device, pinfo.function);

                r = kmalloc(sizeof(rtl8169));
                if (r == NULL) {
                    SHOW_ERROR0(1, "rtl8169_detect: error allocating memory for rtl8169 structure\n");
                    continue;
                }

                memset(r, 0, sizeof(rtl8169));
                r->irq = pinfo.u.h0.interrupt_line;
                // find the memory-mapped base
                int range;
                for (range = 0; range < 6; range++) {
                    if (pinfo.u.h0.base_registers[range] > 0xffff) {
                        r->phys_base = pinfo.u.h0.base_registers[range];
                        r->phys_size = pinfo.u.h0.base_register_sizes[range];
                        break;
                    } else if (pinfo.u.h0.base_registers[range] > 0) {
                        r->io_port = pinfo.u.h0.base_registers[range];
                    }
                }
                if (r->phys_base == 0) {
                    kfree(r);
                    r = NULL;
                    continue;
                }

                SHOW_INFO(1, "detected rtl8169 at irq %d, memory base 0x%lx, size 0x%lx, io base 0x%lx\n", r->irq, r->phys_base, r->phys_size, r->io_port);

                // add it to the list
                r->next = *rtl8169_list;
                *rtl8169_list = r;
            }
        }
    }

    module_put(PCI_BUS_MODULE_NAME);

    return *rtl8169_list ? 0 : ERR_NOT_FOUND;
}
#endif


static void rtl8169_setup_descriptors(rtl8169 *r)
{
    int i;

    /* set up the rx/tx descriptors */
    for (i=0; i < NUM_RX_DESCRIPTORS; i++) {
        addr_t physaddr;

        physaddr = RXBUF_P(r, i);
        SHOW_FLOW(2, "setup_descriptors: rx buffer at %p, addr 0x%x\n", RXBUF(r, i), physaddr);

        r->rxdesc[i].rx_buffer_low = physaddr;
        r->rxdesc[i].rx_buffer_high = physaddr >> 32;
        r->rxdesc[i].frame_len = BUFSIZE_PER_FRAME;
        r->rxdesc[i].flags = RTL_DESC_OWN | ((i == (NUM_RX_DESCRIPTORS - 1)) ? RTL_DESC_EOR : 0);
    }
    for (i=0; i < NUM_TX_DESCRIPTORS; i++) {
        addr_t physaddr;

        physaddr = TXBUF_P(r, i);
        SHOW_FLOW(2, "setup_descriptors: tx buffer at %p, addr 0x%x\n", TXBUF(r, i), physaddr);

        r->txdesc[i].tx_buffer_low = physaddr;
        r->txdesc[i].tx_buffer_high = physaddr >> 32;
        r->txdesc[i].frame_len = 0; // will need to be filled in when transmitting
        r->txdesc[i].flags = (i == (NUM_TX_DESCRIPTORS - 1)) ? RTL_DESC_EOR : 0;
    }

    /* set up our index pointers */
    r->rx_idx_free = 0;
    r->rx_idx_full = 0;
    r->tx_idx_free = 0;
    r->tx_idx_full = 0;

    /* point the nic at the descriptors */
    RTL_WRITE_32(r, REG_TNPDS_LOW, r->txdesc_phys);
    RTL_WRITE_32(r, REG_TNPDS_HIGH, r->txdesc_phys >> 32);
    RTL_WRITE_32(r, REG_RDSAR_LOW, r->rxdesc_phys);
    RTL_WRITE_32(r, REG_RDSAR_HIGH, r->rxdesc_phys >> 32);
}

int rtl8169_init(rtl8169 *r)
{
    //bigtime_t time;
    int err = -1;
    //addr_t temp;
    //int i;

    SHOW_FLOW(2, "rtl8169_init: r %p\n", r);

    /*
     r->region = vm_map_physical_memory(vm_get_kernel_aspace_id(), "rtl8169_region", (void **)&r->virt_base, REGION_ADDR_ANY_ADDRESS, r->phys_size, LOCK_KERNEL|LOCK_RW, r->phys_base);
    if(r->region < 0) {
        SHOW_ERROR0(1, "rtl8169_init: error creating memory mapped region\n");
        err = -1;
        goto err;
    }*/

    size_t n_pages = BYTES_TO_PAGES(r->phys_size);

    hal_alloc_vaddress( (void **)&r->virt_base, n_pages); // alloc address of a page, but not memory
    hal_pages_control_etc( r->phys_base, r->virt_base, n_pages, page_map_io, page_rw, 0 );

    SHOW_INFO(2, "rtl8169 mapped at address 0x%lx\n", r->virt_base);

#if 0
    /* create regions for tx and rx descriptors */
    r->rxdesc_region = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "rtl8169_rxdesc", (void **)&r->rxdesc,
                                                  REGION_ADDR_ANY_ADDRESS, NUM_RX_DESCRIPTORS * DESCRIPTOR_LEN, REGION_WIRING_WIRED_CONTIG, LOCK_KERNEL|LOCK_RW);
    r->rxdesc_phys = vtophys(r->rxdesc);
    SHOW_INFO(2, "rtl8169: rx descriptors at %p, phys 0x%x\n", r->rxdesc, r->rxdesc_phys);
    r->txdesc_region = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "rtl8169_txdesc", (void **)&r->txdesc,
                                                  REGION_ADDR_ANY_ADDRESS, NUM_TX_DESCRIPTORS * DESCRIPTOR_LEN, REGION_WIRING_WIRED_CONTIG, LOCK_KERNEL|LOCK_RW);
    r->txdesc_phys = vtophys(r->txdesc);
    SHOW_INFO(2, "rtl8169: tx descriptors at %p, phys 0x%x\n", r->txdesc, r->txdesc_phys);
    r->reg_spinlock = 0;

    /* create a large tx and rx buffer for the descriptors to point to */
    r->rxbuf_region = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "rtl8169_rxbuf", (void **)&r->rxbuf,
                                                 REGION_ADDR_ANY_ADDRESS, NUM_RX_DESCRIPTORS * BUFSIZE_PER_FRAME, REGION_WIRING_WIRED, LOCK_KERNEL|LOCK_RW);
    r->txbuf_region = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "rtl8169_txbuf", (void **)&r->txbuf,
                                                 REGION_ADDR_ANY_ADDRESS, NUM_TX_DESCRIPTORS * BUFSIZE_PER_FRAME, REGION_WIRING_WIRED, LOCK_KERNEL|LOCK_RW);
#endif

    hal_pv_alloc( &r->rxdesc_phys, (void**)&r->rxdesc, NUM_RX_DESCRIPTORS * DESCRIPTOR_LEN );
    hal_pv_alloc( &r->txdesc_phys, (void**)&r->txdesc, NUM_TX_DESCRIPTORS * DESCRIPTOR_LEN );

    SHOW_INFO(2, "rx descriptors at %p, phys 0x%x\n", r->rxdesc, r->rxdesc_phys);
    SHOW_INFO(2, "tx descriptors at %p, phys 0x%x\n", r->txdesc, r->txdesc_phys);

    hal_pv_alloc( &r->rxbuf_phys, (void**)&r->rxbuf, NUM_RX_DESCRIPTORS * BUFSIZE_PER_FRAME );
    hal_pv_alloc( &r->txbuf_phys, (void**)&r->txbuf, NUM_TX_DESCRIPTORS * BUFSIZE_PER_FRAME );

    /* create a receive sem */
    hal_sem_init( &r->rx_sem, "rtl8169 rx_sem");

    /* transmit sem */
    hal_sem_init(  &r->tx_sem, "rtl8169 tx_sem");

    /* reset the chip */
    int repeats = 100;
    RTL_WRITE_8(r, REG_CR, (1<<4)); // reset the chip, disable tx/rx
    do {
        hal_sleep_msec(10); // 10ms
        if(repeats -- <= 0 )
            break;
    } while(RTL_READ_8(r, REG_CR) & (1<<4));

    /* read in the mac address */
    r->mac_addr[0] = RTL_READ_8(r, REG_IDR0);
    r->mac_addr[1] = RTL_READ_8(r, REG_IDR1);
    r->mac_addr[2] = RTL_READ_8(r, REG_IDR2);
    r->mac_addr[3] = RTL_READ_8(r, REG_IDR3);
    r->mac_addr[4] = RTL_READ_8(r, REG_IDR4);
    r->mac_addr[5] = RTL_READ_8(r, REG_IDR5);
    SHOW_INFO(2, "rtl8169: mac addr %x:%x:%x:%x:%x:%x\n",
              r->mac_addr[0], r->mac_addr[1], r->mac_addr[2],
              r->mac_addr[3], r->mac_addr[4], r->mac_addr[5]);

    /* some voodoo from BSD driver */
    RTL_WRITE_16(r, REG_CCR, RTL_READ_16(r, REG_CCR));
    RTL_SETBITS_16(r, REG_CCR, 0x3);

    /* mask all interrupts */
    RTL_WRITE_16(r, REG_IMR, 0);

    /* set up the tx/rx descriptors */
    rtl8169_setup_descriptors(r);

    /* enable tx/rx */
    RTL_SETBITS_8(r, REG_CR, (1<<3)|(1<<2));

    /* set up the rx state */
    /* 1024 byte dma threshold, 1024 dma max burst, CRC calc 8 byte+, accept all packets */
    RTL_WRITE_32(r, REG_RCR, (1<<16) | (6<<13) | (6<<8) | (0xf << 0));
    RTL_SETBITS_16(r, REG_CCR, (1<<5)); // rx checksum enable
    RTL_WRITE_16(r, REG_RMS, 1518); // rx mtu

    /* set up the tx state */
    RTL_WRITE_32(r, REG_TCR, (RTL_READ_32(r, REG_TCR) & ~0x1ff) | (6<<8)); // 1024 max burst dma
    RTL_WRITE_8(r, REG_MTPS, 0x3f); // max tx packet size (must be careful to not actually transmit more than mtu)

    /* set up the interrupt handler */
    //int_set_io_interrupt_handler(r->irq, &rtl8169_int, r, "rtl8169");
    if(hal_irq_alloc( r->irq, &rtl8169_int, r, HAL_IRQ_SHAREABLE ))
    {
        SHOW_ERROR( 0, "unable to allocate irq %d", r->irq );
        goto err1;
    }

    /* clear all pending interrupts */
    RTL_WRITE_16(r, REG_ISR, 0xffff);

    /* unmask interesting interrupts */
    RTL_WRITE_16(r, REG_IMR, IMR_SYSERR | IMR_LINKCHG | IMR_TER | IMR_TOK | IMR_RER | IMR_ROK | IMR_RXOVL);

    return 0;

err1:
    // TODO free what?
    //vm_delete_region(vm_get_kernel_aspace_id(), r->region);
//err:
    return err;
}

void rtl8169_xmit(rtl8169 *r, const char *ptr, ssize_t len)
{
    //int i;

#if debug_level_flow >= 3
    dprintf("rtl8169_xmit dumping packet:");
    hexdump(ptr, len, 0, 0);
#endif

restart:
    hal_sem_acquire(&r->tx_sem);
    hal_mutex_lock(&r->lock);

    int_disable_interrupts();
    acquire_spinlock(&r->reg_spinlock);

    /* look at the descriptor pointed to by tx_idx_free */
    if (r->txdesc[r->tx_idx_free].flags & RTL_DESC_OWN) {
        /* card owns this one, wait and try again later */
        release_spinlock(&r->reg_spinlock);
        int_restore_interrupts();
        mutex_unlock(&r->lock);
        //		sem_release(r->tx_sem, 1);
        goto restart;
    }

    /* queue it up */
    memcpy(TXBUF(r, r->tx_idx_free), ptr, len);
    if (len < 64)
        len = 64;

    r->txdesc[r->tx_idx_free].frame_len = len;
    r->txdesc[r->tx_idx_free].flags = (r->txdesc[r->tx_idx_free].flags & RTL_DESC_EOR) | RTL_DESC_FS | RTL_DESC_LS | RTL_DESC_OWN;
    inc_tx_idx_free(r);
    RTL_WRITE_8(r, REG_TPPOLL, (1<<6)); // something is on the normal queue

    release_spinlock(&r->reg_spinlock);
    int_restore_interrupts();

    mutex_unlock(&r->lock);
}

ssize_t rtl8169_rx(rtl8169 *r, char *buf, ssize_t buf_len)
{
    //uint32 tail;
    ssize_t len;
    int rc;
    bool release_sem = false;

    SHOW_FLOW0(3, "rtl8169_rx: entry\n");

    if(buf_len < 1500)
        return -1;

restart:
    hal_sem_acquire(&r->rx_sem);
    mutex_lock(&r->lock);

    int_disable_interrupts();
    acquire_spinlock(&r->reg_spinlock);

    /* look at the descriptor pointed to by rx_idx_free */
    if (r->rxdesc[r->rx_idx_free].flags & RTL_DESC_OWN) {
        /* for some reason it's owned by the card, wait for more packets */
        release_spinlock(&r->reg_spinlock);
        int_restore_interrupts();
        mutex_unlock(&r->lock);
        goto restart;
    }

    /* process this packet */
    len =  r->rxdesc[r->rx_idx_free].frame_len & 0x3fff;
    SHOW_FLOW(3, "rtl8169_rx: desc idx %d: len %d\n", r->rx_idx_free, len);

    if (len > buf_len) {
        rc = ERR_TOO_BIG;
        release_sem = true;
        goto out;
    }

    memcpy(buf, RXBUF(r, r->rx_idx_free), len);
    rc = len;

#if debug_level_flow >= 3
    hexdump(RXBUF(r, r->rx_idx_free), len, 0, 0);
#endif

    /* stick it back in the free list */
    r->rxdesc[r->rx_idx_free].buffer_size = BUFSIZE_PER_FRAME;
    r->rxdesc[r->rx_idx_free].flags = (r->rxdesc[r->rx_idx_free].flags & RTL_DESC_EOR) | RTL_DESC_OWN;
    inc_rx_idx_free(r);

    /* see if there are more packets pending */
    if ((r->rxdesc[r->rx_idx_free].flags & RTL_DESC_OWN) == 0)
        release_sem = true; // if so, release the rx sem so the next reader gets a shot

out:
    release_spinlock(&r->reg_spinlock);
    int_restore_interrupts();

    if(release_sem)
        hal_sem_release(&r->rx_sem);
    mutex_unlock(&r->lock);

    return rc;
}

static int rtl8169_rxint(rtl8169 *r, uint16 int_status)
{
    int rc = INT_NO_RESCHEDULE;

    if (int_status & (IMR_ROK|IMR_RER)) {
        int i;

        /* see how many frames we got, adjust our index */
        i = 0;
        while ((r->rxdesc[r->rx_idx_full].flags & RTL_DESC_OWN) == 0) {
            i++;
            inc_rx_idx_full(r);
            if (r->rx_idx_full == r->rx_idx_free) {
                /* we just used up the last descriptor */
                SHOW_ERROR0(1, "rtl8169_rxint: used up last descriptor, chip is gonna blow.\n");
                /* XXX deal with this somehow */
                break; // no more frames left on the ring
            }
        }
        SHOW_FLOW(3, "rxint: got %d frames, idx_full = %d, idx_free = %d\n", i, r->rx_idx_full, r->rx_idx_free);

        if (i > 0) {
#warning SEM_FLAG_NO_RESCHED
            //hal_sem_release_etc( &r->rx_sem, 1, SEM_FLAG_NO_RESCHED);
            hal_sem_release( &r->rx_sem );
            rc = INT_RESCHEDULE;
        }
    }

    return rc;
}

static int rtl8169_txint(rtl8169 *r, uint16 int_status)
{
    //uint32 txstat;
    //int i;
    int rc = INT_NO_RESCHEDULE;

    if (int_status & (IMR_TOK|IMR_TER)) {
        int i;

        /* see how many frames were transmitted */
        i = 0;
        while ((r->txdesc[r->tx_idx_full].flags & RTL_DESC_OWN) == 0) {

            i++;
            inc_tx_idx_full(r);
            if (r->tx_idx_full == r->tx_idx_free) {
                break;
            }
        }
        SHOW_FLOW(3, "txint: sent %d frames, idx_full = %d, idx_free = %d\n", i, r->tx_idx_full, r->tx_idx_free);

        if (i > 0) {
#warning SEM_FLAG_NO_RESCHED
            //hal_sem_release_etc(r->tx_sem, 1, SEM_FLAG_NO_RESCHED);
            hal_sem_release(&r->tx_sem);
            rc = INT_RESCHEDULE;
        }
    }

    return rc;
}

static int rtl8169_int(void* data)
{
    int rc = INT_NO_RESCHEDULE;
    rtl8169 *r = (rtl8169 *)data;
    uint16 istat;

    acquire_spinlock(&r->reg_spinlock);

    istat = RTL_READ_16(r, REG_ISR);
    SHOW_FLOW(3, "rtl8169_int: istat 0x%x\n", istat);
    if (istat == 0)
        goto done;

    if (istat & (IMR_ROK|IMR_RER|IMR_RDU|IMR_RXOVL)) {
        rc |= rtl8169_rxint(r, istat);
    }
    if (istat & (IMR_TOK|IMR_TER|IMR_TDU)) {
        rc |= rtl8169_txint(r, istat);
    }

    RTL_WRITE_16(r, REG_ISR, istat);

done:
    release_spinlock(&r->reg_spinlock);

    return rc;
}


#endif
