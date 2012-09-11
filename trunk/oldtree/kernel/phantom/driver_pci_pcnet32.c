/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * PcNet32 driver.
 *
**/

#include <kernel/config.h>
#if HAVE_NET && HAVE_PCI

#define DEBUG_MSG_PREFIX "PCNET32"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10


#include <phantom_libc.h>
#include <ia32/pio.h>
#include <threads.h>
#include <device.h>
#include <kernel/drivers.h>
#include <kernel/vm.h>

#include <dev/pci/pcnet32_dev.h>
#include <dev/pci/pcnet32_priv.h>

#include <kernel/ethernet_defs.h>

#include <kernel/net.h>

#define WW()
//#define WW() getchar()


#define PCNET_USE_SOFTIRQ 0



#define DEV_NAME "PCNET32: "

static int DEBUG = 0;

#if 0

#define WIRED_ADDRESS 	IPV4_DOTADDR_TO_ADDR(192, 168, 1, 123)
#define WIRED_NETMASK 	0xffffff00
#define WIRED_BROADCAST IPV4_DOTADDR_TO_ADDR(192, 168, 1, 0xFF)

#define WIRED_NET 	IPV4_DOTADDR_TO_ADDR(192, 168, 1, 0)
#define WIRED_ROUTER 	IPV4_DOTADDR_TO_ADDR(192, 168, 1, 1)

#else

#define WIRED_ADDRESS 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 123)
#define WIRED_NETMASK 	0xffffff00
#define WIRED_BROADCAST IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0xFF)

#define WIRED_NET 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0)
#define WIRED_ROUTER 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 123)

#endif

//#define DEF_ROUTE_NET           0x00000000
//#define DEF_ROUTE_MASK   	0x00000000
#define DEF_ROUTE_ROUTER        IPV4_DOTADDR_TO_ADDR(10, 0, 2, 2)

// mapping of initialization block rx and txlengths
static u_int16_t gringlens[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 };




static pcnet32 *pcnet32_new(u_int32_t initmode, u_int16_t rxbuffer_size, u_int16_t txbuffer_size);
static void pcnet32_int(void*);
static void pcnet32_softint(void* data);

// call this to enable a receive buffer so the controller can fill it.
static void rxdesc_init(pcnet32 *nic, u_int16_t index);
static void pcnet32_thread(void *nic);

static int pcnet32_read( struct phantom_device *dev, void *buf, int len);
static int pcnet32_write(struct phantom_device *dev, const void *buf, int len);
static int pcnet32_get_address( struct phantom_device *dev, void *buf, int len);





static int seq_number = 0;





phantom_device_t * driver_pcnet_pchome_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    pcnet32 *nic = NULL;

    SHOW_INFO0( 0, "probe");
WW();
    nic = pcnet32_new( PCNET_INIT_MODE0 | PCNET_INIT_RXLEN_128 | PCNET_INIT_TXLEN_32,
                      2048, 2048);

    if (nic == NULL)
    {
        if(DEBUG) printf(DEV_NAME "pcnet_new returned 0\n");
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

    pcnet32_stop(nic);
    hal_sleep_msec(10);

    if (pcnet32_init(nic) < 0)
    {
        //if(DEBUG)
        printf( DEV_NAME "pcnet_init failed\n");

        pcnet32_delete(nic);
        return 0;
    }

    pcnet32_start(nic);


    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = "pcnet";
    dev->seq_number = seq_number++;
    dev->drv_private = nic;

    dev->dops.read = pcnet32_read;
    dev->dops.write = pcnet32_write;
    dev->dops.get_address = pcnet32_get_address;

    dev->iobase = nic->io_port;
    dev->irq = nic->irq;
    dev->iomem = nic->phys_base;
    dev->iomemsize = nic->phys_size;


    ifnet *interface;
    if( if_register_interface( IF_TYPE_ETHERNET, &interface, dev) )
    {
        printf(DEV_NAME "Failed to register interface for %s", dev->name );
    }
    else
    {
        if_simple_setup(interface, WIRED_ADDRESS, WIRED_NETMASK, WIRED_BROADCAST, WIRED_NET, WIRED_ROUTER, DEF_ROUTE_ROUTER );
    }


WW();
    return dev;

}


static pcnet32 *pcnet32_new(u_int32_t initmode, u_int16_t rxbuffer_size, u_int16_t txbuffer_size)
{
    pcnet32 *nic = NULL;

    u_int16_t rxring_count = gringlens[(initmode & PCNET_INIT_RXLEN_MASK) >> PCNET_INIT_RXLEN_POS];
    u_int16_t txring_count = gringlens[(initmode & PCNET_INIT_TXLEN_MASK) >> PCNET_INIT_TXLEN_POS];

    if(DEBUG) printf( DEV_NAME "initmode: 0x%.8x, rxring: (%d, %d), txring: (%d, %d)\n",
            initmode,
            rxring_count, rxbuffer_size,
            txring_count, txbuffer_size);

    nic = (pcnet32*)malloc(sizeof(pcnet32));
    if (nic == NULL) goto err_none;

    memset(nic, 0, sizeof(pcnet32));
    //nic->bus = bus;
    nic->init_mode = initmode;

    nic->interrupt_count = 0;

    hal_spin_init(&nic->control_lock);

    // Make it a 256-descriptor ring. 256*32 bytes long.
    nic->rxring_count  = rxring_count;
    nic->rx_buffersize = rxbuffer_size;
    nic->rxring_tail   = nic->rxring_head = 0;

    /*nic->rxring_region = vm_create_anonymous_region(
     vm_get_kernel_aspace_id(), "pcnet32_rxring", (void**)&nic->rxring,
     REGION_ADDR_ANY_ADDRESS, nic->rxring_count * sizeof(struct pcnet32_rxdesc),
     REGION_WIRING_WIRED_CONTIG, LOCK_KERNEL | LOCK_RW);*/

    nic->rxring = malloc( nic->rxring_count * sizeof(struct pcnet32_rxdesc) );
    if (nic->rxring == 0) goto err_after_kmalloc;

    memset(nic->rxring, 0, nic->rxring_count * sizeof(struct pcnet32_rxdesc));

    //vm_get_page_mapping(vm_get_kernel_aspace_id(), (addr_t)nic->rxring, &nic->rxring_phys);
    nic->rxring_phys = kvtophys(nic->rxring);
    if(DEBUG) printf( DEV_NAME "rxring physical address: 0x%x\n", nic->rxring_phys);

    //nic->rxring_sem = sem_create(0, "pcnet32_rxring");
    if( hal_sem_init(&(nic->rxring_sem), "pc32 rx") < 0)
        goto err_after_rxring_region;

    if (hal_mutex_init(&nic->rxring_mutex, "pc32 rx" ) < 0)
        goto err_after_rxring_sem;

    // setup_transmit_descriptor_ring;
    nic->txring_count  = txring_count;
    nic->tx_buffersize = txbuffer_size;
    nic->txring_head   = 0;

    nic->txring = malloc(nic->txring_count * sizeof(struct pcnet32_txdesc));
    if(nic->txring == 0)
        goto err_after_rxring_mutex;
    memset(nic->txring, 0, nic->txring_count * sizeof(struct pcnet32_txdesc));
    nic->txring_phys = kvtophys(nic->txring);
    if(DEBUG) printf( DEV_NAME "txring physical address: 0x%x\n", nic->txring_phys);

    /*nic->txring_region = vm_create_anonymous_region(
     vm_get_kernel_aspace_id(), "pcnet32_txring", (void**)&nic->txring,
     REGION_ADDR_ANY_ADDRESS, nic->txring_count * sizeof(struct pcnet32_txdesc),
     REGION_WIRING_WIRED_CONTIG, LOCK_KERNEL | LOCK_RW);

     if (nic->txring_region < 0)
     goto err_after_rxring_mutex;

     memset(nic->txring, 0, nic->txring_count * sizeof(struct pcnet32_txdesc));
     vm_get_page_mapping(vm_get_kernel_aspace_id(),
     (addr_t)nic->txring, &nic->txring_phys);
     if(DEBUG) printf( "txring physical address: 0x%x", nic->txring_phys);*/


    // allocate the actual buffers
    /*nic->buffers_region = vm_create_anonymous_region(
     vm_get_kernel_aspace_id(), "pcnet32_buffers", (void**)&nic->buffers,
     REGION_ADDR_ANY_ADDRESS,
     (nic->rxring_count * nic->rx_buffersize) +
     (nic->txring_count * nic->tx_buffersize),
     REGION_WIRING_WIRED_CONTIG, LOCK_KERNEL | LOCK_RW);

     if (nic->buffers_region < 0)
     goto err_after_txring_region;

     vm_get_page_mapping(vm_get_kernel_aspace_id(), (addr_t)nic->buffers, &nic->buffers_phys);
     */

    nic->buffers = (int)malloc((nic->rxring_count * nic->rx_buffersize) + (nic->txring_count * nic->tx_buffersize));
    if (nic->buffers == 0) goto err_after_txring_region;
    nic->buffers_phys = kvtophys(nic->buffers);

    nic->rx_buffers = (u_int8_t*)nic->buffers;
    nic->tx_buffers = (u_int8_t*)(nic->buffers + nic->rxring_count * nic->rx_buffersize);

    // create the thread
    //nic->interrupt_sem = sem_create(0, "pcnet32_interrupt");
    if( hal_sem_init(&(nic->interrupt_sem), "pc32 IRQ") )
        goto err_after_buffers_region;

    //nic->thread = thread_create_kernel_thread("pcnet32_isr", pcnet32_thread, nic);
    //if (nic->thread < 0)            goto err_after_interrupt_sem;
    nic->thread = hal_start_kernel_thread_arg( pcnet32_thread, nic );

    if(DEBUG) printf( DEV_NAME "thread created, id: 0x%x\n", nic->thread);
    // TODO hi pri thread!
    //thread_set_priority(nic->thread, THREAD_HIGHEST_PRIORITY);

    nic->active = 1; // Right?

    return nic;

//err_after_interrupt_sem:
    hal_sem_destroy( &(nic->interrupt_sem) );

err_after_buffers_region:
    //vm_delete_region(vm_get_kernel_aspace_id(), nic->buffers_region);
    free((void *)nic->buffers);

err_after_txring_region:
    //vm_delete_region(vm_get_kernel_aspace_id(), nic->txring_region);
    free(nic->txring);

err_after_rxring_mutex:
    hal_mutex_destroy(&nic->rxring_mutex);

err_after_rxring_sem:
    hal_sem_destroy( &(nic->rxring_sem) );

err_after_rxring_region:
    //vm_delete_region(vm_get_kernel_aspace_id(), nic->rxring_region);
    free(nic->rxring);

err_after_kmalloc:
    free(nic);

err_none:
    return NULL;
}








#define WRITE_8(nic, reg, dat) 	outb((nic)->io_port + (reg), dat)
#define WRITE_16(nic, reg, dat) outw((nic)->io_port + (reg), dat)
/*void WRITE_16(pcnet32 *nic, int reg, int dat)
{
    if(DEBUG) printf( "w16 io_port 0x%x \n", nic->io_port + (reg));
    hal_sleep_msec(100);
    outw((nic)->io_port + (reg), dat);
}*/

#define WRITE_32(nic, reg, dat) outl((nic)->io_port + (reg), dat)

/*void WRITE_32(pcnet32 *nic, int reg, int dat)
{
    if(DEBUG) printf( "w32 io_port 0x%x \n", nic->io_port + (reg));
    hal_sleep_msec(100);
    outl((nic)->io_port + (reg), dat);
} */


#define READ_8(nic, reg) inb((nic)->io_port + (reg))
#define READ_16(nic, reg) inw((nic)->io_port + (reg))
#define READ_32(nic, reg) inl((nic)->io_port + (reg))

/*u_int32_t READ_32(pcnet32 *nic, int reg)
{
    if(DEBUG) printf( "r32 io_port 0x%x \n", nic->io_port + (reg));
    return inl((nic)->io_port + (reg));
}*/

#define RXRING_INDEX(_nic, _index) ((_index) & (_nic->rxring_count - 1))
#define TXRING_INDEX(_nic, _index) ((_index) & (_nic->txring_count - 1))

#define RXRING_BUFFER(_nic, _index) ((_nic)->rx_buffers + ((_index) * (_nic)->rx_buffersize))
#define TXRING_BUFFER(_nic, _index) ((_nic)->tx_buffers + ((_index) * (_nic)->tx_buffersize))
#define BUFFER_PHYS(_nic, _buffer) (addr_t)((_nic)->buffers_phys + ((_buffer) - (_nic)->buffers))


static u_int16_t read_csr(pcnet32 *nic, u_int32_t reg)
{
    WRITE_32(nic, PCNET_IO_ADDRPORT, reg);
    return READ_32(nic, PCNET_IO_DATAPORT);
}

/* XX this function is not used
 static u_int16_t read_bcr(pcnet32 *nic, u_int32_t reg)
 {
 WRITE_32(nic, PCNET_IO_ADDRPORT, reg);
 return READ_32(nic, PCNET_IO_CONFIGPORT);
 }*/

static void write_csr(pcnet32 *nic, u_int32_t reg, u_int16_t data)
{
    WRITE_32(nic, PCNET_IO_ADDRPORT, reg);
    WRITE_32(nic, PCNET_IO_DATAPORT, data);
}

// XX this function is not used (but perhaps should be in some places?
static void modify_csr(pcnet32 *nic, u_int32_t reg, u_int16_t bits, u_int16_t set)
{
    u_int16_t oldset;

    //acquire_spinlock(&nic->control_lock);
    //int_disable_interrupts();
    int s = hal_save_cli();
    hal_spin_lock(&nic->control_lock);

    // do stuff
    oldset = read_csr(nic, reg);
    oldset |= (bits & set);
    oldset &= ~(bits & ~set);

    write_csr(nic, reg, oldset);

    //int_restore_interrupts();
    //release_spinlock(&nic->control_lock);
    hal_spin_unlock(&nic->control_lock);
    if(s) hal_sti();
}

static void write_bcr(pcnet32 *nic, u_int32_t reg, u_int16_t data)
{
    WRITE_32(nic, PCNET_IO_ADDRPORT, reg);
    WRITE_32(nic, PCNET_IO_CONFIGPORT, data);
}



int pcnet32_init(pcnet32 *nic)
{
    int i;

    if(DEBUG) printf( DEV_NAME "init device at irq %d, memory base 0x%lx, size 0x%lx, io port base 0x%x\n",
            nic->irq, nic->phys_base, nic->phys_size, nic->io_port);

    // set up the interrupt handler
    //int_set_io_interrupt_handler(nic->irq, &pcnet32_int, nic, "pcnet32");
    if( hal_irq_alloc( nic->irq, &pcnet32_int, nic, HAL_IRQ_SHAREABLE ) )
    {
        //if(DEBUG)
        printf(DEV_NAME "IRQ %d is busy\n", nic->irq );
//WW();
        return -1;
    }

    if( hal_irq_alloc( 15, &pcnet32_int, nic, HAL_IRQ_SHAREABLE ) )
    {
        //if(DEBUG)
        printf(DEV_NAME "IRQ %d is busy\n", 15 );
//WW();
        return -1;
    }

    nic->softirq = hal_alloc_softirq();
    if( nic->softirq < 0 )
    {
        SHOW_ERROR0( 0, "Unable to get softirq" );
        goto unmap_irq;
    }

    hal_set_softirq_handler( nic->softirq, &pcnet32_softint, nic );


    if(DEBUG) printf( DEV_NAME "device mapped irq 0x%x\n", nic->irq ); //+ 0x20);

    nic->virt_base = 0; // should generate a panic if someone tries to use it


    if(nic->phys_base > 0)
    {
        int pages = 1 + (nic->phys_size-1)/hal_mem_pagesize();
        void *va;
        if( hal_alloc_vaddress(&va, pages) )
        {
            if(DEBUG) printf(DEV_NAME "can't alloc addr space\n");
            goto unmap_irq; // TODO free softirq!
        }

        hal_pages_control( (int)nic->phys_base, va, pages, page_map, page_rw );

        nic->virt_base = (int)va;

        /*nic->io_region = vm_map_physical_memory(vm_get_kernel_aspace_id(), "pcnet32_region",
                                                (void **)&nic->virt_base, REGION_ADDR_ANY_ADDRESS, nic->phys_size,
                                                LOCK_KERNEL|LOCK_RW, nic->phys_base);*/

        /*if(nic->io_region < 0) {
            if(DEBUG) printf( "error allocating device physical region at %x\n",
                    nic->phys_base);
            return nic->io_region;
        }*/
        if(DEBUG) printf( DEV_NAME "device mapped at virtual address 0x%lx\n", nic->virt_base);
    }
    else
    {
        //nic->io_region = -1;
        nic->virt_base = 0; // should generate a panic if someone tries to use it
    }


    // before we do this we set up our interrupt handler,
    // we want to make sure the device is in a semi-known state, so we
    // do a little control register stuff here.
    write_csr(nic, PCNET_CSR_STATUS, PCNET_STATUS_STOP);

    // fetch ethernet address
    for (i = 0; i < 6; i++)
    {
        nic->mac_addr[i] = READ_8(nic, i);
    }
    if(DEBUG) printf( DEV_NAME "MAC Address: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
            nic->mac_addr[0], nic->mac_addr[1], nic->mac_addr[2],
            nic->mac_addr[3], nic->mac_addr[4], nic->mac_addr[5]);


#if 0 // Prints 0
    int reported_int_line = read_csr(nic, PCNET_CSR_INT_LINE);
    printf(DEV_NAME "reported INT=%d\n", reported_int_line);
#endif

    return 0;

unmap_irq:
    hal_irq_free( nic->irq, &pcnet32_int, nic );
    return -1;
}



static void pcnet32_softint(void* data)
{
    pcnet32 *nic = (pcnet32 *)data;
    hal_sem_release(&(nic->rxring_sem));
    SHOW_FLOW0( 7, "softirq");
}

static void pcnet32_int(void* data)
{
    pcnet32 *nic = (pcnet32 *)data;
    u_int32_t status;

    int s = hal_save_cli();
    hal_spin_lock(&nic->control_lock);

    status = read_csr(nic, PCNET_CSR_STATUS);

    //if(DEBUG) printf( "handling irq: status 0x%x", status);

    // clear the bits that caused this to happen and disable pcnet interrupts
    // for the time being
    write_csr(nic, PCNET_CSR_STATUS, status & ~PCNET_STATUS_IENA);

    hal_spin_unlock(&nic->control_lock);
    if(s) hal_sti();

    nic->interrupt_status = status;

    //sem_release_etc(nic->interrupt_sem, 1, SEM_FLAG_NO_RESCHED);
#if PCNET_USE_SOFTIRQ
    hal_request_softirq( nic->softirq );
#else
    hal_sem_release( &(nic->interrupt_sem) );
#endif

    nic->interrupt_count++;

    if(DEBUG) printf( DEV_NAME "interrupt handled. pcnet status 0x%.8x", status );

    //return INT_RESCHEDULE;
}


void pcnet32_start(pcnet32 *nic)
{
    int i = 0;
    struct pcnet32_init *init;

    //hal_spin_lock(&nic->control_lock);

    if(DEBUG) printf( DEV_NAME "starting %p\n", nic);

    if(DEBUG) printf( DEV_NAME "resetting device\n");
    READ_32(nic, PCNET_IO_RESET);
    WRITE_32(nic, PCNET_IO_RESET, 0);

    // give the device time to figure it out
    //thread_snooze(10000);
    hal_sleep_msec(100);

    if(DEBUG) printf( DEV_NAME "set us up in 32-bit wide structure mode\n");
    // set us up in 32-bit wide structure mode
    WRITE_32(nic, PCNET_IO_DATAPORT, 0);
    write_bcr(nic, PCNET_BCR_SWMODE, 0x0002);

    // now we build the initialization block in the
    // buffer area (which is unused right now)
    init = (struct pcnet32_init*)nic->buffers;
    memset(init, 0, sizeof(struct pcnet32_init));

    init->flags = nic->init_mode;
    memcpy(init->paddr, &nic->mac_addr, sizeof(nic->mac_addr));
    init->rxaddr = (int)nic->rxring_phys;
    init->txaddr = (int)nic->txring_phys;
//hexdump(init, sizeof(struct pcnet32_init), "init", 0);
//WW();
    write_csr(nic, PCNET_CSR_IADDR0, (nic->buffers_phys) & 0xffff);
    write_csr(nic, PCNET_CSR_IADDR1, (nic->buffers_phys >> 16) & 0xffff);
    write_csr(nic, PCNET_CSR_STATUS, PCNET_STATUS_INIT);

    if(DEBUG) printf( DEV_NAME "wait for it to finish initializing." );
    // wait for it to finish initializing.
    while ( !(read_csr(nic, PCNET_CSR_STATUS) & PCNET_STATUS_IDON) )
        ;


    memset(init, 0, sizeof(struct pcnet32_init));

    // Initialize the rxring
    for (i = 0; i < nic->rxring_count; i++)
        rxdesc_init(nic, i);

    // Initialize the txring
    memset(nic->txring, 0, nic->txring_count * sizeof(struct pcnet32_txdesc));


    hal_sleep_msec(100);
    if(DEBUG) printf( DEV_NAME "ei device\n");

    // write the start and interrupt enable bits.
    write_csr(nic, PCNET_CSR_STATUS, PCNET_STATUS_STRT | PCNET_STATUS_IENA);

    // for the sake of verification, we'll read csr0 and dprint it.
    if(DEBUG) printf( DEV_NAME "csr0 (status) = 0x%.4x\n", read_csr(nic, PCNET_CSR_STATUS));

    // start the thread
    //thread_resume_thread(nic->thread);
    nic->active = 1;

    //hal_spin_unlock(&nic->control_lock);

}

void pcnet32_stop(pcnet32 *nic)
{
    if(DEBUG) printf( DEV_NAME "Stopping device %p\n", nic);

    // start the thread
    //thread_suspend_thread(nic->thread);
    nic->active = 0;

    // stop the device.
    write_csr(nic, PCNET_CSR_STATUS, PCNET_STATUS_STOP);
}

static void rxdesc_init(pcnet32 *nic, u_int16_t index)
{
    u_int16_t masked_index = RXRING_INDEX(nic, index);

    struct pcnet32_rxdesc *desc = nic->rxring + masked_index;
    u_int32_t buffer = BUFFER_PHYS(nic, RXRING_BUFFER(nic, masked_index));

    desc->buffer_addr = buffer;
    desc->buffer_length = -nic->rx_buffersize;
    desc->message_length = 0;
    desc->user = 0;

    // enable the controller to write to this receive buffer.
    desc->status = PCNET_RXSTATUS_OWN;
}



ssize_t pcnet32_xmit(pcnet32 *nic, const char *ptr, ssize_t len)
{
    u_int16_t index = 0;
    u_int8_t *buffer = NULL;

    if(DEBUG) printf( DEV_NAME "XMIT nic %p data %p len %d\n", nic, ptr, len);

    index = TXRING_INDEX(nic, nic->txring_head);

    if(DEBUG) printf( DEV_NAME "using txring index %d\n", index);

    // XXX need support for spanning packets in case a size other than 2048 is used.
    // Examine OWN bit of tx descriptor at tx_queue_tail;
    // if (OWN == 1) return "Out of buffer";
    if ((len > nic->tx_buffersize) ||
        (nic->txring[index].status & PCNET_TXSTATUS_OWN))
    {
        if(DEBUG) printf( DEV_NAME "packet was too large or no more txbuffers\n");
        return -1; //ERR_VFS_INSUFFICIENT_BUF;
    }

    // Get buffer address from descriptor at end_tx_queue;
    buffer = TXRING_BUFFER(nic, index);
    nic->txring[index].buffer_addr = BUFFER_PHYS(nic, buffer);

    // Copy packet to tx buffer;
    memcpy(buffer, ptr, len);

    // Set up BCNT field of descriptor;
    nic->txring[index].buffer_length = -len;

    // Set OWN, STP, and ENP bits of descriptor;
    nic->txring[index].status |= PCNET_TXSTATUS_OWN |
        PCNET_TXSTATUS_STP | PCNET_TXSTATUS_ENP;

    // tx_queue_tail = tx_queue_tail + 1;
    // if (tx_queue_tail > last_tx_descriptor) tx_queue_tail = first_tx_descriptor;
    nic->txring_head++;

    // set the transmit demand bit in CSR0
    modify_csr(nic, PCNET_CSR_STATUS, PCNET_STATUS_TDMD, PCNET_STATUS_TDMD);

    // return "OK";
    return len;
}

ssize_t pcnet32_rx(pcnet32 *nic, char *buf, ssize_t buf_len)
{
    u_int16_t index = 0;
    ssize_t real_len = -1;

    if(DEBUG) printf( DEV_NAME "RX nic %p data %p buf_len %d\n", nic, buf, buf_len);

    // here we loop through consuming rxring descriptors until we get
    // one suitable for returning.
    while (1)
    {
        // consume 1 descriptor at a time, whether it's an error or a
        // valid packet.
        hal_sem_acquire( &(nic->rxring_sem) );
        SHOW_FLOW( 9, "nic %p rx semaphore was signalled", nic);
        //if(NET_CHATTY||DEBUG) printf( DEV_NAME "nic %p rx semaphore was signalled\n", nic);

        // the semaphor has been released at least once, so we will
        // lock the rxring and grab the next available descriptor.
        hal_mutex_lock(&nic->rxring_mutex);

        // grab the index we want.
        index = RXRING_INDEX(nic, nic->rxring_tail);

        if (nic->rxring[index].status & PCNET_RXSTATUS_ERR)
        {
            if(DEBUG) printf( DEV_NAME "rxring descriptor %d reported an error: 0x%.4x\n",
                    index, nic->rxring[index].status);
        }

        if (nic->rxring[index].status & PCNET_RXSTATUS_OWN)
        {
            if(DEBUG) printf( DEV_NAME "warning: descriptor %d should have been owned by the software is owned by the hardware\n", index);

            nic->rxring_tail++;
            hal_mutex_unlock(&nic->rxring_mutex);
            continue; // skip this descriptor altogether.
        }
        else if ((size_t)buf_len >= nic->rxring[index].message_length) // got one
        {
            real_len = nic->rxring[index].message_length;

            // copy the buffer
            memcpy(buf, RXRING_BUFFER(nic, index), real_len);

            // reinitialize the buffer and give it back to the controller.
            rxdesc_init(nic, index);

            if(DEBUG) printf( DEV_NAME "Got index %d, len %d and cleared it, returning to caller. rxstatus = 0x%x\n", index,
                    nic->rxring[index].message_length, nic->rxring[index].status);

            // move the tail up.
            nic->rxring_tail++;
            hal_mutex_unlock(&nic->rxring_mutex);
            return real_len;
        } else {
            // there is a packet, but it's too large for the buffer.
            // So we don't want to do anything with the ring, we just
            // want to unlock our mutex and return.
            hal_mutex_unlock(&nic->rxring_mutex);
            return -1; //ERR_TOO_BIG;
        }
    }

    return real_len;
}


















// these two return false when there is nothing left to do.
static int pcnet32_rxint(pcnet32 *nic)
{
    u_int32_t index;

    index = RXRING_INDEX(nic, nic->rxring_head);
    if(DEBUG) printf( DEV_NAME "Next rxring index (%d) status = 0x%x\n", index, nic->rxring[index].status);
    if ( !(nic->rxring[index].status & PCNET_RXSTATUS_OWN) )
    {
        if(DEBUG) printf( DEV_NAME "Got packet len %d, index %d\n", nic->rxring[index].message_length, index);
        nic->rxring_head++;

        hal_sem_release(&(nic->rxring_sem));

        return 1;
    }
    if(DEBUG) printf( DEV_NAME "No more packets\n");

    return 0;
}



static void pcnet32_thread(void *data)
{
    t_current_set_name("PCNet32Drv");

    //volatile
    pcnet32 *nic = (pcnet32 *)data;

    if(DEBUG) printf( DEV_NAME "Thread ready, wait 4 nic active\n");
    while(1)
    {
        while( !nic->active )
            hal_sleep_msec(1000);

        if(DEBUG) printf( DEV_NAME "Thread ready, wait 4 sema\n");

        //hal_sem_acquire( &(nic->interrupt_sem) );

        // XXX BUG DUMB CODE
        while(nic->interrupt_count <= 0)
            hal_sleep_msec(200);

        nic->interrupt_count--;

        if(DEBUG) printf( "Acquired semaphore, status = 0x%x\n", nic->interrupt_status);

        // check if there is a 'fatal error' (BABL and MERR)
        if ((nic->interrupt_status & PCNET_STATUS_ERR) &&
            (nic->interrupt_status & PCNET_STATUS_BABL ||
             nic->interrupt_status & PCNET_STATUS_MERR))
        {
            // fatal error. Stop everything and reinitialize the device.
            pcnet32_stop(nic);
            pcnet32_start(nic);
        }

        hal_mutex_lock(&nic->rxring_mutex);
        while (pcnet32_rxint(nic));
        hal_mutex_unlock(&nic->rxring_mutex);

        // re-enable pcnet interrupts
        write_csr(nic, PCNET_CSR_STATUS, PCNET_STATUS_IENA);

        if(DEBUG) printf( DEV_NAME "Finished iteration, status = 0x%x\n", read_csr(nic, PCNET_CSR_STATUS));
    }

}





void pcnet32_delete(pcnet32 *nic)
{
    hal_sem_destroy(&(nic->interrupt_sem));

    //vm_delete_region(vm_get_kernel_aspace_id(), nic->buffers_region);
    //vm_delete_region(vm_get_kernel_aspace_id(), nic->txring_region);
    free((void *)nic->buffers);
    free(nic->txring);

    hal_mutex_destroy(&nic->rxring_mutex);
    hal_sem_destroy(&(nic->rxring_sem));

    //vm_delete_region(vm_get_kernel_aspace_id(), nic->rxring_region);
    free(nic->rxring);

    free(nic);
}

















static int pcnet32_read( struct phantom_device *dev, void *buf, int len)
{
    pcnet32 *nic = (pcnet32 *)dev->drv_private;

    if(len < ETHERNET_MAX_SIZE)
        return ERR_VFS_INSUFFICIENT_BUF;
    return pcnet32_rx(nic, buf, len);
}

static int pcnet32_write(struct phantom_device *dev, const void *buf, int len)
{
    pcnet32 *nic = (pcnet32 *)dev->drv_private;

    if(len < 0)
        return ERR_INVALID_ARGS;

    return pcnet32_xmit(nic, buf, len);
}


static int pcnet32_get_address( struct phantom_device *dev, void *buf, int len)
{
    pcnet32 *nic = (pcnet32 *)dev->drv_private;
    int err = NO_ERROR;

    if(!nic)        return ERR_IO_ERROR;

    if(len >= (int)sizeof(nic->mac_addr)) {
        memcpy(buf, nic->mac_addr, sizeof(nic->mac_addr));
    } else {
        err = ERR_VFS_INSUFFICIENT_BUF;
    }

    return err;
}



#if 0


/*
 ** Copyright 2001-2002, Graham Batty. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */



static int pcnet32_ioctl(dev_cookie cookie, int op, void *buf, size_t len)
{
    pcnet32 *nic = (pcnet32 *)cookie;
    int err = NO_ERROR;

    if(DEBUG) printf( DEV_NAME "op %d, buf %p, len %Ld\n", op, buf, (long long)len);

    if(!nic)
        return ERR_IO_ERROR;

    switch(op) {
    case IOCTL_NET_IF_GET_ADDR: // get the ethernet MAC address
        if(len >= sizeof(nic->mac_addr)) {
            memcpy(buf, nic->mac_addr, sizeof(nic->mac_addr));
        } else {
            err = ERR_VFS_INSUFFICIENT_BUF;
        }
        break;
    case IOCTL_NET_IF_GET_TYPE:
        if (len >= sizeof(int)) {
            *(int *)buf = IF_TYPE_ETHERNET;
        } else {
            err = ERR_VFS_INSUFFICIENT_BUF;
        }
        break;
    default:
        err = ERR_INVALID_ARGS;
    }

    return err;
}




#endif

#endif // HAVE_NET && HAVE_PCI

