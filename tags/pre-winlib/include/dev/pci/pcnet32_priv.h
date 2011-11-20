/*
** Copyright 2001, Graham Batty. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _PCNET32_PRIV_H
#define _PCNET32_PRIV_H


#include <phantom_types.h>
#include <hal.h>
#include <compat/newos.h>


struct pcnet32_rxdesc
{
    u_int32_t buffer_addr;
    int16_t   buffer_length;
    u_int16_t status;
    u_int32_t message_length;
    u_int32_t user;
};

struct pcnet32_txdesc
{
    u_int32_t buffer_addr;
    int16_t buffer_length;
    u_int16_t status;
    u_int32_t misc;
    u_int32_t user;
};

struct pcnet32_init
{
    u_int32_t flags;
    u_int8_t paddr[8]; // only 6 bytes used.
    u_int64_t laddr_mask; // __attribute__ ((packed));
    addr_t rxaddr;// __attribute__ ((packed));//  __attribute__ ((aligned (4) )) ;
    addr_t txaddr;// __attribute__ ((packed));//  __attribute__ ((aligned (4) )) ;
} __attribute__ ((packed));

// Both tx and rx ring_count members must be set such that subtracting one from them
// renders a mask of the valid bits up to the last entry in the ring.
// This is so that we don't have to bother wrapping indexes, we just mask them.
struct pcnet32
{
    int active; // is true if driver is active
    //pci_module_hooks *bus;

    hal_spinlock_t control_lock; // lock before doing any register modification.

    int thread;


    hal_sem_t interrupt_sem;

    hal_cond_t 	interrupt_cond;
    int         interrupt_count;

    u_int32_t interrupt_status;

    u_int32_t init_mode;

    int irq;
    addr_t phys_base;

    int softirq;

    //addr_t phys_size;
    u_int32_t phys_size;

    addr_t virt_base;
    u_int16_t io_port;

    // don't need
    //region_id io_region;

    u_int8_t mac_addr[6];

    // receive ring
    u_int16_t rxring_count;
    u_int16_t rxring_tail;
    u_int16_t rxring_head;


    hal_sem_t rxring_sem; // incremented for every packet available for reading

    hal_mutex_t rxring_mutex; // lock this before acting on the contents of the rxring.

    //region_id rxring_region;
    struct pcnet32_rxdesc *rxring;
    //addr_t
    u_int32_t rxring_phys;

    // transmit ring
    u_int16_t txring_count;
    u_int16_t txring_head; // next place to insert a packet


    //region_id txring_region;

    struct pcnet32_txdesc *txring;
    //addr_t
    u_int32_t txring_phys;

    // buffers

    //region_id buffers_region;

    //addr_t buffers;
    //addr_t buffers_phys;
    u_int32_t buffers;
    u_int32_t buffers_phys;

    u_int16_t rx_buffersize;
    u_int8_t *rx_buffers;

    u_int16_t tx_buffersize;
    u_int8_t *tx_buffers;
};

typedef struct pcnet32 pcnet32;


// these allocate and deallocate all the resources associated with
// the network device.
static pcnet32 *pcnet32_new(/*pci_module_hooks *bus, */ u_int32_t initmode, u_int16_t rxbuffer_size, u_int16_t txbuffer_size);
void pcnet32_delete(pcnet32 *nic);

// these detect and initialize a pcnet device.
int pcnet32_detect(pcnet32 *dev);
int pcnet32_init(pcnet32 *nic);

// these start and stop the device. Calling them in sequence
// results in the device being reset. Note that pcnet_start
// will cause any queued send or receive packets to be dropped
// before the device is restarted.
void pcnet32_start(pcnet32 *nic);
void pcnet32_stop(pcnet32 *nic);

ssize_t pcnet32_xmit(pcnet32 *nic, const char *ptr, ssize_t len);
ssize_t pcnet32_rx(pcnet32 *nic, char *buf, ssize_t buf_len);

// PCNET 32-bit IO Resources:
enum {
    PCNET_IO_APROM0     = 0x00,
    PCNET_IO_APROM1     = 0x04,
    PCNET_IO_APROM2     = 0x08,
    PCNET_IO_APROM3     = 0x0C,
    PCNET_IO_DATAPORT   = 0x10,
    PCNET_IO_ADDRPORT   = 0x14,
    PCNET_IO_RESET      = 0x18,
    PCNET_IO_CONFIGPORT = 0x1C
};

// PCNET 32-bit Configuration Status Registers
enum {
    PCNET_CSR_STATUS = 0,
    PCNET_CSR_IADDR0 = 1,
    PCNET_CSR_IADDR1 = 2,

    PCNET_CSR_LADDR0 = 8,
    PCNET_CSR_LADDR1,
    PCNET_CSR_LADDR2,
    PCNET_CSR_LADDR3,

    PCNET_CSR_PADDR0 = 12,
    PCNET_CSR_PADDR1,
    PCNET_CSR_PADDR2,

    PCNET_CSR_MODE = 15,

    PCNET_CSR_RXRING_ADDR0 = 24,
    PCNET_CSR_RXRING_ADDR1,
    PCNET_CSR_RXRING_LEN = 76,

    PCNET_CSR_TXRING_ADDR0 = 30,
    PCNET_CSR_TXRING_ADDR1,

    PCNET_CSR_INT_LINE = 60, // Returns 0 on QEMU

    PCNET_CSR_TXRING_LEN = 78,

    PCNET_CSR_POLLING_INTERVAL = 47,
};

enum {
    PCNET_BCR_BUSCTL = 0x12,
    PCNET_BCR_SWMODE = 0x14,
};

enum {
    PCNET_INIT_MODE_MASK = 0xffff, // first 16 bits is the mode
    PCNET_INIT_RXLEN_MASK = 0x00f00000, // bits 20-23 are rxlen.
    PCNET_INIT_RXLEN_POS = 20, // rxlen is 20 bits in.
    PCNET_INIT_TXLEN_MASK = 0xf0000000, // bits 28-31 are txlen.
    PCNET_INIT_TXLEN_POS = 28, // txlen is 28 bits in.

    PCNET_INIT_MODE0 = 0,

    PCNET_INIT_LEN_1 = 0x0,
    PCNET_INIT_LEN_2 = 0x1,
    PCNET_INIT_LEN_4 = 0x2,
    PCNET_INIT_LEN_8 = 0x3,
    PCNET_INIT_LEN_16 = 0x4,
    PCNET_INIT_LEN_32 = 0x5,
    PCNET_INIT_LEN_64 = 0x6,
    PCNET_INIT_LEN_128 = 0x7,
    PCNET_INIT_LEN_256 = 0x8,
    PCNET_INIT_LEN_512 = 0x9,

    PCNET_INIT_RXLEN_1 = PCNET_INIT_LEN_1 << PCNET_INIT_RXLEN_POS,
    PCNET_INIT_RXLEN_2 = PCNET_INIT_LEN_2 << PCNET_INIT_RXLEN_POS,
    PCNET_INIT_RXLEN_4 = PCNET_INIT_LEN_4 << PCNET_INIT_RXLEN_POS,
    PCNET_INIT_RXLEN_8 = PCNET_INIT_LEN_8 << PCNET_INIT_RXLEN_POS,
    PCNET_INIT_RXLEN_16 = PCNET_INIT_LEN_16 << PCNET_INIT_RXLEN_POS,
    PCNET_INIT_RXLEN_32 = PCNET_INIT_LEN_32 << PCNET_INIT_RXLEN_POS,
    PCNET_INIT_RXLEN_64 = PCNET_INIT_LEN_64 << PCNET_INIT_RXLEN_POS,
    PCNET_INIT_RXLEN_128 = PCNET_INIT_LEN_128 << PCNET_INIT_RXLEN_POS,
    PCNET_INIT_RXLEN_256 = PCNET_INIT_LEN_256 << PCNET_INIT_RXLEN_POS,
    PCNET_INIT_RXLEN_512 = PCNET_INIT_LEN_512 << PCNET_INIT_RXLEN_POS,

    PCNET_INIT_TXLEN_1 = PCNET_INIT_LEN_1 << PCNET_INIT_TXLEN_POS,
    PCNET_INIT_TXLEN_2 = PCNET_INIT_LEN_2 << PCNET_INIT_TXLEN_POS,
    PCNET_INIT_TXLEN_4 = PCNET_INIT_LEN_4 << PCNET_INIT_TXLEN_POS,
    PCNET_INIT_TXLEN_8 = PCNET_INIT_LEN_8 << PCNET_INIT_TXLEN_POS,
    PCNET_INIT_TXLEN_16 = PCNET_INIT_LEN_16 << PCNET_INIT_TXLEN_POS,
    PCNET_INIT_TXLEN_32 = PCNET_INIT_LEN_32 << PCNET_INIT_TXLEN_POS,
    PCNET_INIT_TXLEN_64 = PCNET_INIT_LEN_64 << PCNET_INIT_TXLEN_POS,
    PCNET_INIT_TXLEN_128 = PCNET_INIT_LEN_128 << PCNET_INIT_TXLEN_POS,
    PCNET_INIT_TXLEN_256 = PCNET_INIT_LEN_256 << PCNET_INIT_TXLEN_POS,
    PCNET_INIT_TXLEN_512 = PCNET_INIT_LEN_512 << PCNET_INIT_TXLEN_POS,
};

enum {
    PCNET_STATUS_ERR = 0x8000,
    PCNET_STATUS_BABL = 0x4000,
    PCNET_STATUS_CERR = 0x2000,
    PCNET_STATUS_MISS = 0x1000,
    PCNET_STATUS_MERR = 0x800,
    PCNET_STATUS_RINT = 0x400,
    PCNET_STATUS_TINT = 0x200,
    PCNET_STATUS_IDON = 0x100,
    PCNET_STATUS_INTR = 0x80,
    PCNET_STATUS_IENA = 0x40,
    PCNET_STATUS_RXON = 0x20,
    PCNET_STATUS_TXON = 0x10,
    PCNET_STATUS_TDMD = 0x8,
    PCNET_STATUS_STOP = 0x4,
    PCNET_STATUS_STRT = 0x2,
    PCNET_STATUS_INIT = 0x1,
};

enum {
    PCNET_RXSTATUS_OWN = 0x8000,
    PCNET_RXSTATUS_ERR = 0x4000,
    PCNET_RXSTATUS_FRAM = 0x2000,
    PCNET_RXSTATUS_OFLW = 0x1000,
    PCNET_RXSTATUS_CRC = 0x800,
    PCNET_RXSTATUS_BUFF = 0x400,
    PCNET_RXSTATUS_STP = 0x200,
    PCNET_RXSTATUS_ENP = 0x100,
    PCNET_RXSTATUS_BPE = 0x80,
    PCNET_RXSTATUS_PAM = 0x40,
    PCNET_RXSTATUS_LAFM = 0x20,
    PCNET_RXSTATUS_BAM = 0x10,
};

enum {
    PCNET_TXSTATUS_OWN = 0x8000,
    PCNET_TXSTATUS_ERR = 0x4000,
    PCNET_TXSTATUS_ADD_FCS = 0x2000,
    PCNET_TXSTATUS_MORE = 0x1000,
    PCNET_TXSTATUS_LTINT = 0x1000, // depends on LTINTEN bit of CSR5
    PCNET_TXSTATUS_ONE = 0x800,
    PCNET_TXSTATUS_DEF = 0x400,
    PCNET_TXSTATUS_STP = 0x200,
    PCNET_TXSTATUS_ENP = 0x100,
    PCNET_TXSTATUS_BPE = 0x20,

    PCNET_TXMISC_BUFF = 0x80000000,
    PCNET_TXMISC_UFLW = 0x40000000,
    PCNET_TXMISC_EXDF = 0x20000000,
    PCNET_TXMISC_LCOL = 0x10000000,
    PCNET_TXMISC_LCAR = 0x8000000,
    PCNET_TXMISC_RTRY = 0x4000000,
};


#endif
