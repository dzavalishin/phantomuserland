#if 0


/*
 =========================================================================
 r8169.c: A RealTek RTL-8169 Gigabit Ethernet driver for Linux kernel 2.4.x.
 --------------------------------------------------------------------
 Ported to Syllable by Kristian Van Der Vliet <vanders@liqwyd.com>

 History:
 Feb  4 2002	- created initially by ShuChen <shuchen@realtek.com.tw>.
 May 20 2002	- Add link status force-mode and TBI mode support.
 2004	- Massive updates. See kernel SCM system for details.
 =========================================================================
 1. [DEPRECATED: use ethtool instead] The media can be forced in 5 modes.
 Command: 'insmod r8169 media = SET_MEDIA'
 Ex:	  'insmod r8169 media = 0x04' will force PHY to operate in 100Mpbs Half-duplex.

 SET_MEDIA can be:
 _10_Half	= 0x01
 _10_Full	= 0x02
 _100_Half	= 0x04
 _100_Full	= 0x08
 _1000_Full	= 0x10

 2. Support TBI mode.
 =========================================================================
 VERSION 1.1	<2002/10/4>

 The bit4:0 of MII register 4 is called "selector field", and have to be
 00001b to indicate support of IEEE std 802.3 during NWay process of
 exchanging Link Code Word (FLP).

 VERSION 1.2	<2002/11/30>

 - Large style cleanup
 - Use ether_crc in stock kernel (linux/crc32.h)
 - Copy mc_filter setup code from 8139cp
 (includes an optimization, and avoids set_bit use)

 VERSION 1.6LK	<2004/04/14>

 - Merge of Realtek's version 1.6
 - Conversion to DMA API
 - Suspend/resume
 - Endianness
 - Misc Rx/Tx bugs

 VERSION 2.2LK	<2005/01/25>

 - RX csum, TX csum/SG, TSO
 - VLAN
 - baby (< 7200) Jumbo frames support
 - Merge of Realtek's version 2.2 (new phy)
 */


#define DEBUG_MSG_PREFIX "rtl8169"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10


#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <hal.h>

#include <kernel/debug.h>
#include <kernel/timedcall.h>
#include <kernel/ethernet_defs.h>

#include <errno.h>

// ALIGN
#include <x86/phantom_page.h>

/*
#include <kernel/irq.h>
#include <kernel/udelay.h>
#include <kernel/timer.h>
#include <kernel/pci.h>
#include <kernel/semaphore.h>
#include <kernel/spinlock.h>
#include <kernel/ctype.h>
#include <kernel/device.h>
#include <kernel/time.h>
#include <kernel/bitops.h>
#include <kernel/isa_io.h>
#include <kernel/signal.h>
#include <kernel/net.h>
#include <kernel/ip.h>

#include <posix/unistd.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <net/sockios.h>
*/


// temp, replace w hdr
#define MAX_ADDR_LEN 32 
#define	PCI_VENDOR_ID_REALTEK				0x10EC
#define	PCI_ANY_ID -1 // right?

#define dma_addr_t physaddr_t

#define status_t errno_t

#define u64 uint64



#define NO_DEBUG_STUBS 1
#include <kernel/linux_compat.h>

#if 0
#undef kerndbg
#define kerndbg(level,format,arg...) printk(format, ## arg);
#endif

#define DMA_32BIT_MASK	0x00000000ffffffffUL

/* XXXKV: Taken from ethtool.h: should be someplace else */
#define SPEED_10		10
#define SPEED_100		100
#define SPEED_1000		1000

#define DUPLEX_HALF		0x00
#define DUPLEX_FULL		0x01

#define	AUTONEG_DISABLE	0x00
#define	AUTONEG_ENABLE	0x01

#define NET_IP_ALIGN	2

//static DeviceOperations_s g_sDevOps;
//PCI_bus_s* g_psBus;

/* XXXKV: From dp83815.c (Which I think is from somewhere else..) */
struct net_device
{

    /*
     * This is the first field of the "visible" part of this structure
     * (i.e. as seen by users in the "Space.c" file).  It is the name
     * the interface.
     */
    const char            *name;

    /*
     *  I/O specific fields
     *  FIXME: Merge these and struct ifmap into one
     */
    unsigned long   rmem_end; /* shmem "recv" end */
    unsigned long   rmem_start; /* shmem "recv" start */
    unsigned long   mem_end;  /* shared mem end */
    unsigned long   mem_start;  /* shared mem start */
    unsigned long   base_addr;  /* device I/O address   */
    unsigned int    irq;        /* device IRQ number    */

    /* Low-level status flags. */
    volatile unsigned char  start;      /* start an operation   */
    /*
     * These two are just single-bit flags, but due to atomicity
     * reasons they have to be inside a "unsigned long". However,
     * they should be inside the SAME unsigned long instead of
     * this wasteful use of memory..
     */
    unsigned char   if_port;  /* Selectable AUI, TP,..*/
    unsigned char   dma;    /* DMA channel    */

    unsigned long       interrupt;  /* bitops.. */
    unsigned long       tbusy;      /* transmitter busy */

    //struct device       *next;

    /*
     * This marks the end of the "visible" part of the structure. All
     * fields hereafter are internal to the system, and may change at
     * will (read: may be cleaned up at will).
     */

    /* These may be needed for future network-power-down code. */
    unsigned long       trans_start;    /* Time (in jiffies) of last Tx */
    unsigned long           last_rx;        /* Time of last Rx      */
    unsigned    mtu;  /* interface MTU value    */
    unsigned short    type; /* interface hardware type  */
    unsigned short    hard_header_len;  /* hardware hdr length  */
    unsigned short      flags;  /* interface flags (a la BSD) */
    void            *priv;  /* pointer to private data  */

    /* Interface address info. */
    unsigned char   broadcast[MAX_ADDR_LEN];  /* hw bcast add */
    unsigned char   pad;    /* make dev_addr aligned to 8 bytes */
    unsigned char   dev_addr[MAX_ADDR_LEN]; /* hw address */
    unsigned char   perm_addr[MAX_ADDR_LEN];
    unsigned char   addr_len; /* hardware address length  */

    int			mc_count;	/* Number of installed mcasts	*/

    //NetQueue_s		*packet_queue;
    int irq_handle; /* IRQ handler handle */

    int device_handle; /* device handle from probing */

    int node_handle;    /* handle of device node in /dev */
};

#define R8169_MSG_DEFAULT \
    (NETIF_MSG_DRV | NETIF_MSG_PROBE | NETIF_MSG_IFUP | NETIF_MSG_IFDOWN)

#define TX_BUFFS_AVAIL(tp) \
    (tp->dirty_tx + NUM_TX_DESC - tp->cur_tx - 1)

#define rtl8169_rx_skb			netif_rx
#define rtl8169_rx_hwaccel_skb		vlan_hwaccel_rx
#define rtl8169_rx_quota(count, quota)	count

/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static int max_interrupt_work = 20;

#if 0
/* Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
 The RTL chips use a 64 element hash table based on the Ethernet CRC. */
static int multicast_filter_limit = 32;
#endif

/* MAC address length */
#define MAC_ADDR_LEN	6

#define RX_FIFO_THRESH	7	/* 7 means NO threshold, Rx buffer level before first PCI xfer. */
#define RX_DMA_BURST	6	/* Maximum PCI burst, '6' is 1024 */
#define TX_DMA_BURST	6	/* Maximum PCI burst, '6' is 1024 */
#define EarlyTxThld 	0x3F	/* 0x3F means NO early transmit */
#define RxPacketMaxSize	0x3FE8	/* 16K - 1 - ETH_HLEN - VLAN - CRC... */
#define SafeMtu		0x1c20	/* ... actually life sucks beyond ~7k */
#define InterFrameGap	0x03	/* 3 means InterFrameGap = the shortest one */

#define R8169_REGS_SIZE		256
#define R8169_NAPI_WEIGHT	64
#define NUM_TX_DESC	64	/* Number of Tx descriptor registers */
#define NUM_RX_DESC	256	/* Number of Rx descriptor registers */
#define RX_BUF_SIZE	1536	/* Rx Buffer size */
#define R8169_TX_RING_BYTES	(NUM_TX_DESC * sizeof(struct TxDesc))
#define R8169_RX_RING_BYTES	(NUM_RX_DESC * sizeof(struct RxDesc))

#define RTL8169_TX_TIMEOUT	(6*1000)
#define RTL8169_PHY_TIMEOUT	(10*1000)

/* write/read MMIO register */
#define RTL_W8(reg, val8)	writeb ((val8), ioaddr + (reg))
#define RTL_W16(reg, val16)	writew ((val16), ioaddr + (reg))
#define RTL_W32(reg, val32)	writel ((val32), ioaddr + (reg))
#define RTL_R8(reg)		readb (ioaddr + (reg))
#define RTL_R16(reg)		readw (ioaddr + (reg))
#define RTL_R32(reg)		((unsigned long) readl (ioaddr + (reg)))

enum mac_version {
    RTL_GIGA_MAC_VER_01 = 0x00,
    RTL_GIGA_MAC_VER_02 = 0x01,
    RTL_GIGA_MAC_VER_03 = 0x02,
    RTL_GIGA_MAC_VER_04 = 0x03,
    RTL_GIGA_MAC_VER_05 = 0x04,
    RTL_GIGA_MAC_VER_11 = 0x0b,
    RTL_GIGA_MAC_VER_12 = 0x0c,
    RTL_GIGA_MAC_VER_13 = 0x0d,
    RTL_GIGA_MAC_VER_14 = 0x0e,
    RTL_GIGA_MAC_VER_15 = 0x0f
};

enum phy_version {
    RTL_GIGA_PHY_VER_C = 0x03, /* PHY Reg 0x03 bit0-3 == 0x0000 */
    RTL_GIGA_PHY_VER_D = 0x04, /* PHY Reg 0x03 bit0-3 == 0x0000 */
    RTL_GIGA_PHY_VER_E = 0x05, /* PHY Reg 0x03 bit0-3 == 0x0000 */
    RTL_GIGA_PHY_VER_F = 0x06, /* PHY Reg 0x03 bit0-3 == 0x0001 */
    RTL_GIGA_PHY_VER_G = 0x07, /* PHY Reg 0x03 bit0-3 == 0x0002 */
    RTL_GIGA_PHY_VER_H = 0x08, /* PHY Reg 0x03 bit0-3 == 0x0003 */
};

#define _R(NAME,MAC,MASK) \
    { .name = NAME, .mac_version = MAC, .RxConfigMask = MASK }

static const struct {
    const char *name;
    uint8 mac_version;
    uint32 RxConfigMask;	/* Clears the bits supported by this chip */
} rtl_chip_info[] = {
    _R("RTL8169",		RTL_GIGA_MAC_VER_01, 0xff7e1880),
    _R("RTL8169s/8110s",	RTL_GIGA_MAC_VER_02, 0xff7e1880),
    _R("RTL8169s/8110s",	RTL_GIGA_MAC_VER_03, 0xff7e1880),
    _R("RTL8169sb/8110sb",	RTL_GIGA_MAC_VER_04, 0xff7e1880),
    _R("RTL8169sc/8110sc",	RTL_GIGA_MAC_VER_05, 0xff7e1880),
    _R("RTL8168b/8111b",	RTL_GIGA_MAC_VER_11, 0xff7e1880), // PCI-E
    _R("RTL8168b/8111b",	RTL_GIGA_MAC_VER_12, 0xff7e1880), // PCI-E
    _R("RTL8101e",		RTL_GIGA_MAC_VER_13, 0xff7e1880), // PCI-E 8139
    _R("RTL8100e",		RTL_GIGA_MAC_VER_14, 0xff7e1880), // PCI-E 8139
    _R("RTL8100e",		RTL_GIGA_MAC_VER_15, 0xff7e1880)  // PCI-E 8139
};
#undef _R

/* XXXKV: These should be in pci_vendors.h */
#define PCI_VENDOR_ID_USR		0x16ec
#define PCI_VENDOR_ID_DLINK		0x1186
#define PCI_VENDOR_ID_LINKSYS	0x1737

enum cfg_version {
    RTL_CFG_0 = 0x00,
    RTL_CFG_1,
    RTL_CFG_2
};

static const struct {
    unsigned int region;
    unsigned int align;
} rtl_cfg_info[] = {
    [RTL_CFG_0] = { 1, NET_IP_ALIGN },
    [RTL_CFG_1] = { 2, NET_IP_ALIGN },
    [RTL_CFG_2] = { 2, 8 }
};


/*
static struct pci_device_id rtl8169_pci_tbl[] = {
    { PCI_VENDOR_ID_REALTEK,	0x8129, 0, 0, 0, 0, RTL_CFG_0 },
    { PCI_VENDOR_ID_REALTEK,	0x8136, 0, 0, 0, 0, RTL_CFG_2 },
    { PCI_VENDOR_ID_REALTEK,	0x8167, 0, 0, 0, 0, RTL_CFG_0 },
    { PCI_VENDOR_ID_REALTEK,	0x8168, 0, 0, 0, 0, RTL_CFG_2 },
    { PCI_VENDOR_ID_REALTEK,	0x8169, 0, 0, 0, 0, RTL_CFG_0 },
    { PCI_VENDOR_ID_DLINK,		0x4300, 0, 0, 0, 0, RTL_CFG_0 },
    { PCI_VENDOR_ID_USR,		0x0116, 0, 0, 0, 0, RTL_CFG_0 },
    { PCI_VENDOR_ID_LINKSYS,	0x1032, 0, 0, 0, 0, RTL_CFG_0 },
    { PCI_ANY_ID, 				0x0024, 0, 0, 0, 0, RTL_CFG_0 },
    {0,},
};
*/

static int rx_copybreak = 200;

enum RTL8169_registers {
    MAC0 = 0,		/* Ethernet hardware address. */
    MAR0 = 8,		/* Multicast filter. */
    CounterAddrLow = 0x10,
    CounterAddrHigh = 0x14,
    TxDescStartAddrLow = 0x20,
    TxDescStartAddrHigh = 0x24,
    TxHDescStartAddrLow = 0x28,
    TxHDescStartAddrHigh = 0x2c,
    FLASH = 0x30,
    ERSR = 0x36,
    ChipCmd = 0x37,
    TxPoll = 0x38,
    IntrMask = 0x3C,
    IntrStatus = 0x3E,
    TxConfig = 0x40,
    RxConfig = 0x44,
    RxMissed = 0x4C,
    Cfg9346 = 0x50,
    Config0 = 0x51,
    Config1 = 0x52,
    Config2 = 0x53,
    Config3 = 0x54,
    Config4 = 0x55,
    Config5 = 0x56,
    MultiIntr = 0x5C,
    PHYAR = 0x60,
    TBICSR = 0x64,
    TBI_ANAR = 0x68,
    TBI_LPAR = 0x6A,
    PHYstatus = 0x6C,
    RxMaxSize = 0xDA,
    CPlusCmd = 0xE0,
    IntrMitigate = 0xE2,
    RxDescAddrLow = 0xE4,
    RxDescAddrHigh = 0xE8,
    EarlyTxThres = 0xEC,
    FuncEvent = 0xF0,
    FuncEventMask = 0xF4,
    FuncPresetState = 0xF8,
    FuncForceEvent = 0xFC,
};

enum RTL8169_register_content {
    /* InterruptStatusBits */
    SYSErr = 0x8000,
    PCSTimeout = 0x4000,
    SWInt = 0x0100,
    TxDescUnavail = 0x80,
    RxFIFOOver = 0x40,
    LinkChg = 0x20,
    RxOverflow = 0x10,
    TxErr = 0x08,
    TxOK = 0x04,
    RxErr = 0x02,
    RxOK = 0x01,

    /* RxStatusDesc */
    RxRES = 0x00200000,
    RxCRC = 0x00080000,
    RxRUNT = 0x00100000,
    RxRWT = 0x00400000,

    /* ChipCmdBits */
    CmdReset = 0x10,
    CmdRxEnb = 0x08,
    CmdTxEnb = 0x04,
    RxBufEmpty = 0x01,

    /* Cfg9346Bits */
    Cfg9346_Lock = 0x00,
    Cfg9346_Unlock = 0xC0,

    /* rx_mode_bits */
    AcceptErr = 0x20,
    AcceptRunt = 0x10,
    AcceptBroadcast = 0x08,
    AcceptMulticast = 0x04,
    AcceptMyPhys = 0x02,
    AcceptAllPhys = 0x01,

    /* RxConfigBits */
    RxCfgFIFOShift = 13,
    RxCfgDMAShift = 8,

    /* TxConfigBits */
    TxInterFrameGapShift = 24,
    TxDMAShift = 8,	/* DMA burst value (0-7) is shift this many bits */

    /* TBICSR p.28 */
    TBIReset	= 0x80000000,
    TBILoopback	= 0x40000000,
    TBINwEnable	= 0x20000000,
    TBINwRestart	= 0x10000000,
    TBILinkOk	= 0x02000000,
    TBINwComplete	= 0x01000000,

    /* CPlusCmd p.31 */
    RxVlan		= (1 << 6),
    RxChkSum	= (1 << 5),
    PCIDAC		= (1 << 4),
    PCIMulRW	= (1 << 3),

    /* rtl8169_PHYstatus */
    TBI_Enable = 0x80,
    TxFlowCtrl = 0x40,
    RxFlowCtrl = 0x20,
    _1000bpsF = 0x10,
    _100bps = 0x08,
    _10bps = 0x04,
    LinkStatus = 0x02,
    FullDup = 0x01,

    /* GIGABIT_PHY_registers */
    PHY_CTRL_REG = 0,
    PHY_STAT_REG = 1,
    PHY_AUTO_NEGO_REG = 4,
    PHY_1000_CTRL_REG = 9,

    /* GIGABIT_PHY_REG_BIT */
    PHY_Restart_Auto_Nego = 0x0200,
    PHY_Enable_Auto_Nego = 0x1000,

    /* PHY_STAT_REG = 1 */
    PHY_Auto_Neco_Comp = 0x0020,

    /* PHY_AUTO_NEGO_REG = 4 */
    PHY_Cap_10_Half = 0x0020,
    PHY_Cap_10_Full = 0x0040,
    PHY_Cap_100_Half = 0x0080,
    PHY_Cap_100_Full = 0x0100,

    /* PHY_1000_CTRL_REG = 9 */
    PHY_Cap_1000_Half = 0x0100,
    PHY_Cap_1000_Full = 0x0200,

    PHY_Cap_Null = 0x0,

    /* _MediaType */
    _10_Half = 0x01,
    _10_Full = 0x02,
    _100_Half = 0x04,
    _100_Full = 0x08,
    _1000_Full = 0x10,

    /* _TBICSRBit */
    TBILinkOK = 0x02000000,

    /* DumpCounterCommand */
    CounterDump = 0x8,
};

enum _DescStatusBit {
    DescOwn		= (1 << 31), /* Descriptor is owned by NIC */
    RingEnd		= (1 << 30), /* End of descriptor ring */
    FirstFrag	= (1 << 29), /* First segment of a packet */
    LastFrag	= (1 << 28), /* Final segment of a packet */

    /* Tx private */
    LargeSend	= (1 << 27), /* TCP Large Send Offload (TSO) */
    MSSShift	= 16,        /* MSS value position */
    MSSMask		= 0xfff,     /* MSS value + LargeSend bit: 12 bits */
    IPCS		= (1 << 18), /* Calculate IP checksum */
    UDPCS		= (1 << 17), /* Calculate UDP/IP checksum */
    TCPCS		= (1 << 16), /* Calculate TCP/IP checksum */
    TxVlanTag	= (1 << 17), /* Add VLAN tag */

    /* Rx private */
    PID1		= (1 << 18), /* Protocol ID bit 1/2 */
    PID0		= (1 << 17), /* Protocol ID bit 2/2 */

#define RxProtoUDP	(PID1)
#define RxProtoTCP	(PID0)
#define RxProtoIP	(PID1 | PID0)
#define RxProtoMask	RxProtoIP

    IPFail		= (1 << 16), /* IP checksum failed */
    UDPFail		= (1 << 15), /* UDP/IP checksum failed */
    TCPFail		= (1 << 14), /* TCP/IP checksum failed */
    RxVlanTag	= (1 << 16), /* VLAN tag available */
};

#define RsvdMask	0x3fffc000

struct TxDesc {
    uint32 opts1;
    uint32 opts2;
    uint64 addr;
};

struct RxDesc {
    uint32 opts1;
    uint32 opts2;
    uint64 addr;
};

struct ring_info {
    //PacketBuf_s	*skb;
    uint32		len;
    uint8		__pad[sizeof(void *) - sizeof(uint32)];
};

struct rtl8169_private {
    void *mmio_addr;	/* memory map physical address */

    //PCI_Info_s *pci_dev;	/* Index of PCI device */

    hal_spinlock_t lock;		/* spin lock flag */

    uint32 msg_enable;
    int chipset;
    int mac_version;
    int phy_version;
    uint32 cur_rx; /* Index into the Rx descriptor buffer of next Rx pkt. */
    uint32 cur_tx; /* Index into the Tx descriptor buffer of next Rx pkt. */
    uint32 dirty_rx;
    uint32 dirty_tx;

    void *TxDesc;
    void *RxDesc;

    struct TxDesc *TxDescArray;	/* 256-aligned Tx descriptor ring */
    struct RxDesc *RxDescArray;	/* 256-aligned Rx descriptor ring */
    dma_addr_t TxPhyAddr;
    dma_addr_t RxPhyAddr;
    //PacketBuf_s *Rx_skbuff[NUM_RX_DESC];	/* Rx data buffers */
    struct ring_info tx_skb[NUM_TX_DESC];	/* Tx data buffers */
    unsigned int align;
    unsigned int rx_buf_sz;
    unsigned int region;
    //area_id	reg_area;	/* registers */

    timedcall_t timer;

    uint16 cp_cmd;
    uint16 intr_mask;
    int phy_auto_nego_reg;
    int phy_1000_ctrl_reg;
    int (*set_speed)(struct net_device *, uint8 autoneg, uint16 speed, uint8 duplex);
    void (*phy_reset_enable)(void *);
    unsigned int (*phy_reset_pending)(void *);
    unsigned int (*link_ok)(void *);
};

static int rtl8169_open(struct net_device *dev);
//static int rtl8169_start_xmit(PacketBuf_s *skb, struct net_device *dev);
static int rtl8169_interrupt(int irq, void *dev_instance, SysCallRegs_s *regs);
static int rtl8169_init_ring(struct net_device *dev);
static void rtl8169_hw_start(struct net_device *dev);
static int rtl8169_close(struct net_device *dev);
static void rtl8169_set_rx_mode(struct net_device *dev);
static int rtl8169_rx_interrupt(struct net_device *, struct rtl8169_private *, void *);
static void rtl8169_down(struct net_device *dev);

#if 0
static int rtl8169_change_mtu(struct net_device *dev, int new_mtu);
#endif

static void rtl8169_phy_timer(void * __opaque);

static const uint16 rtl8169_intr_mask =
    SYSErr | LinkChg | RxOverflow | RxFIFOOver | TxErr | TxOK | RxErr | RxOK;
static const uint16 rtl8169_napi_event =
    RxOK | RxOverflow | RxFIFOOver | TxOK | TxErr;
static const unsigned int rtl8169_rx_config =
    (RX_FIFO_THRESH << RxCfgFIFOShift) | (RX_DMA_BURST << RxCfgDMAShift);

#define PHY_Cap_10_Half_Or_Less PHY_Cap_10_Half
#define PHY_Cap_10_Full_Or_Less PHY_Cap_10_Full | PHY_Cap_10_Half_Or_Less
#define PHY_Cap_100_Half_Or_Less PHY_Cap_100_Half | PHY_Cap_10_Full_Or_Less
#define PHY_Cap_100_Full_Or_Less PHY_Cap_100_Full | PHY_Cap_100_Half_Or_Less

/* XXXKV: This is a workaround for an optimisation bug in GCC, which optimises
 a block of code to a memcpy() *after* the pre-processor has run, which creates
 an undefined reference because normally memcpy() is a macro. Doing it this way
 at least means the linker will be able to resolve the reference that GCC
 helpfully creates for us. * /

#undef memcpy
static void *memcpy(void *to, const void *from, size_t size)
{
    return __memcpy(to,from,size);
}
*/

static void mdio_write(void *ioaddr, int RegAddr, int value)
{
    int i;

    RTL_W32(PHYAR, 0x80000000 | (RegAddr & 0xFF) << 16 | value);
    udelay(1000);

    for (i = 2000; i > 0; i--) {
        /* Check if the RTL8169 has completed writing to the specified MII register */
        if (!(RTL_R32(PHYAR) & 0x80000000))
            break;
        udelay(100);
    }
}

static int mdio_read(void *ioaddr, int RegAddr)
{
    int i, value = -1;

    RTL_W32(PHYAR, 0x0 | (RegAddr & 0xFF) << 16);
    udelay(1000);

    for (i = 2000; i > 0; i--) {
        /* Check if the RTL8169 has completed retrieving data from the specified MII register */
        if (RTL_R32(PHYAR) & 0x80000000) {
            value = (int) (RTL_R32(PHYAR) & 0xFFFF);
            break;
        }
        udelay(100);
    }
    return value;
}

static void rtl8169_irq_mask_and_ack(void *ioaddr)
{
    RTL_W16(IntrMask, 0x0000);

    RTL_W16(IntrStatus, 0xffff);
}

static void rtl8169_asic_down(void *ioaddr)
{
    RTL_W8(ChipCmd, 0x00);
    rtl8169_irq_mask_and_ack(ioaddr);
    RTL_R16(CPlusCmd);
}

static unsigned int rtl8169_tbi_reset_pending(void *ioaddr)
{
    return RTL_R32(TBICSR) & TBIReset;
}

static unsigned int rtl8169_xmii_reset_pending(void *ioaddr)
{
    return mdio_read(ioaddr, 0) & 0x8000;
}

static unsigned int rtl8169_tbi_link_ok(void *ioaddr)
{
    return RTL_R32(TBICSR) & TBILinkOk;
}

static unsigned int rtl8169_xmii_link_ok(void *ioaddr)
{
    return RTL_R8(PHYstatus) & LinkStatus;
}

static void rtl8169_tbi_reset_enable(void *ioaddr)
{
    RTL_W32(TBICSR, RTL_R32(TBICSR) | TBIReset);
}

static void rtl8169_xmii_reset_enable(void *ioaddr)
{
    unsigned int val;

    val = (mdio_read(ioaddr, PHY_CTRL_REG) | 0x8000) & 0xffff;
    mdio_write(ioaddr, PHY_CTRL_REG, val);
}

static void rtl8169_check_link_status(struct net_device *dev, struct rtl8169_private *tp, void *ioaddr)
{
    unsigned long flags;

    spinlock_cli(&tp->lock, flags);
    if (tp->link_ok(ioaddr)) {
        netif_carrier_on(dev);
        SHOW_INFO( 0, "%s: link up", dev->name);
    } else {
        SHOW_INFO( 0, "%s: link down", dev->name);
        netif_carrier_off(dev);
    }
    spinunlock_restore(&tp->lock, flags);
}

static int rtl8169_set_speed_tbi(struct net_device *dev, uint8 autoneg, uint16 speed, uint8 duplex)
{
    struct rtl8169_private *tp = dev->priv;
    void *ioaddr = tp->mmio_addr;
    int ret = 0;
    uint32 reg;

    reg = RTL_R32(TBICSR);
    if ((autoneg == AUTONEG_DISABLE) && (speed == SPEED_1000) &&
        (duplex == DUPLEX_FULL)) {
        RTL_W32(TBICSR, reg & ~(TBINwEnable | TBINwRestart));
    } else if (autoneg == AUTONEG_ENABLE)
        RTL_W32(TBICSR, reg | TBINwEnable | TBINwRestart);
    else {
        SHOW_ERROR( 0, "%s: incorrect speed setting refused in TBI mode", dev->name);
        ret = -EOPNOTSUPP;
    }

    return ret;
}

static int rtl8169_set_speed_xmii(struct net_device *dev, uint8 autoneg, uint16 speed, uint8 duplex)
{
    struct rtl8169_private *tp = dev->priv;
    void *ioaddr = tp->mmio_addr;
    int auto_nego, giga_ctrl;

    auto_nego = mdio_read(ioaddr, PHY_AUTO_NEGO_REG);
    auto_nego &= ~(PHY_Cap_10_Half | PHY_Cap_10_Full |
                   PHY_Cap_100_Half | PHY_Cap_100_Full);
    giga_ctrl = mdio_read(ioaddr, PHY_1000_CTRL_REG);
    giga_ctrl &= ~(PHY_Cap_1000_Full | PHY_Cap_1000_Half | PHY_Cap_Null);

    if (autoneg == AUTONEG_ENABLE) {
        auto_nego |= (PHY_Cap_10_Half | PHY_Cap_10_Full |
                      PHY_Cap_100_Half | PHY_Cap_100_Full);
        giga_ctrl |= PHY_Cap_1000_Full | PHY_Cap_1000_Half;
    } else {
        if (speed == SPEED_10)
            auto_nego |= PHY_Cap_10_Half | PHY_Cap_10_Full;
        else if (speed == SPEED_100)
            auto_nego |= PHY_Cap_100_Half | PHY_Cap_100_Full;
        else if (speed == SPEED_1000)
            giga_ctrl |= PHY_Cap_1000_Full | PHY_Cap_1000_Half;

        if (duplex == DUPLEX_HALF)
            auto_nego &= ~(PHY_Cap_10_Full | PHY_Cap_100_Full);

        /* This tweak comes straight from Realtek's driver. */
        if ((speed == SPEED_100) && (duplex == DUPLEX_HALF) &&
            (tp->mac_version == RTL_GIGA_MAC_VER_13)) {
            auto_nego = PHY_Cap_100_Half | 0x01;
        }
    }

    /* The 8100e/8101e do Fast Ethernet only. */
    if ((tp->mac_version == RTL_GIGA_MAC_VER_13) ||
        (tp->mac_version == RTL_GIGA_MAC_VER_14) ||
        (tp->mac_version == RTL_GIGA_MAC_VER_15)) {
        if ((giga_ctrl & (PHY_Cap_1000_Full | PHY_Cap_1000_Half))) {
            SHOW_FLOW( 0, "%s: PHY does not support 1000Mbps.",
                     dev->name);
        }
        giga_ctrl &= ~(PHY_Cap_1000_Full | PHY_Cap_1000_Half);
    }

    tp->phy_auto_nego_reg = auto_nego;
    tp->phy_1000_ctrl_reg = giga_ctrl;

    mdio_write(ioaddr, PHY_AUTO_NEGO_REG, auto_nego);
    mdio_write(ioaddr, PHY_1000_CTRL_REG, giga_ctrl);
    mdio_write(ioaddr, PHY_CTRL_REG, PHY_Enable_Auto_Nego |
               PHY_Restart_Auto_Nego);
    return 0;
}

static int rtl8169_set_speed(struct net_device *dev, uint8 autoneg, uint16 speed, uint8 duplex)
{
    struct rtl8169_private *tp = dev->priv;
    int ret;

    ret = tp->set_speed(dev, autoneg, speed, duplex);

    if (netif_running(dev) && (tp->phy_1000_ctrl_reg & PHY_Cap_1000_Full))
    {
        delete_timer( tp->timer );

        //tp->timer = create_timer();
        start_timer( &(tp->timer),  &rtl8169_phy_timer, dev, (RTL8169_PHY_TIMEOUT)*100, true );
    }

    return ret;
}


#if 0
/* !CONFIG_R8169_VLAN */

static inline uint32 rtl8169_tx_vlan_tag(struct rtl8169_private *tp, PacketBuf_s *skb)
{
    return 0;
}

static int rtl8169_rx_vlan_skb(struct rtl8169_private *tp, struct RxDesc *desc, PacketBuf_s *skb)
{
    return -1;
}
#endif

static void rtl8169_write_gmii_reg_bit(void *ioaddr, int reg, int bitnum, int bitval)
{
    int val;

    val = mdio_read(ioaddr, reg);
    val = (bitval == 1) ?
        val | (bitval << bitnum) :  val & ~(0x0001 << bitnum);
    mdio_write(ioaddr, reg, val & 0xffff);
}

static void rtl8169_get_mac_version(struct rtl8169_private *tp, void *ioaddr)
{
    const struct {
        uint32 mask;
        int mac_version;
    } mac_info[] = {
        { 0x38800000,	RTL_GIGA_MAC_VER_15 },
        { 0x38000000,	RTL_GIGA_MAC_VER_12 },
        { 0x34000000,	RTL_GIGA_MAC_VER_13 },
        { 0x30800000,	RTL_GIGA_MAC_VER_14 },
        { 0x30000000,   RTL_GIGA_MAC_VER_11 },
        { 0x18000000,	RTL_GIGA_MAC_VER_05 },
        { 0x10000000,	RTL_GIGA_MAC_VER_04 },
        { 0x04000000,	RTL_GIGA_MAC_VER_03 },
        { 0x00800000,	RTL_GIGA_MAC_VER_02 },
        { 0x00000000,	RTL_GIGA_MAC_VER_01 }	/* Catch-all */
    }, *p = mac_info;
    uint32 reg;

    reg = RTL_R32(TxConfig) & 0x7c800000;
    while ((reg & p->mask) != p->mask)
        p++;
    tp->mac_version = p->mac_version;
}

static void rtl8169_print_mac_version(struct rtl8169_private *tp)
{
    SHOW_FLOW( 0, "mac_version = 0x%02x", tp->mac_version);
}

static void rtl8169_get_phy_version(struct rtl8169_private *tp, void *ioaddr)
{
    const struct {
        uint16 mask;
        uint16 set;
        int phy_version;
    } phy_info[] = {
        { 0x000f, 0x0002, RTL_GIGA_PHY_VER_G },
        { 0x000f, 0x0001, RTL_GIGA_PHY_VER_F },
        { 0x000f, 0x0000, RTL_GIGA_PHY_VER_E },
        { 0x0000, 0x0000, RTL_GIGA_PHY_VER_D } /* Catch-all */
    }, *p = phy_info;
    uint16 reg;

    reg = mdio_read(ioaddr, 3) & 0xffff;
    while ((reg & p->mask) != p->set)
        p++;
    tp->phy_version = p->phy_version;
}

static void rtl8169_print_phy_version(struct rtl8169_private *tp)
{
    struct {
        int version;
        char *msg;
        uint32 reg;
    } phy_print[] = {
        { RTL_GIGA_PHY_VER_G, "RTL_GIGA_PHY_VER_G", 0x0002 },
        { RTL_GIGA_PHY_VER_F, "RTL_GIGA_PHY_VER_F", 0x0001 },
        { RTL_GIGA_PHY_VER_E, "RTL_GIGA_PHY_VER_E", 0x0000 },
        { RTL_GIGA_PHY_VER_D, "RTL_GIGA_PHY_VER_D", 0x0000 },
        { 0, NULL, 0x0000 }
    }, *p;

    for (p = phy_print; p->msg; p++) {
        if (tp->phy_version == p->version) {
            SHOW_FLOW( 0,  "phy_version == %s (%04x)", p->msg, p->reg);
            return;
        }
    }
    SHOW_FLOW0( 0, "phy_version == Unknown");
}

static void rtl8169_hw_phy_config(struct net_device *dev)
{
    struct rtl8169_private *tp = dev->priv;
    void *ioaddr = tp->mmio_addr;
    struct {
        uint16 regs[5]; /* Beware of bit-sign propagation */
    } phy_magic[5] = { {
        { 0x0000,	//w 4 15 12 0
        0x00a1,	//w 3 15 0 00a1
        0x0008,	//w 2 15 0 0008
        0x1020,	//w 1 15 0 1020
        0x1000 } },{	//w 0 15 0 1000
        { 0x7000,	//w 4 15 12 7
        0xff41,	//w 3 15 0 ff41
        0xde60,	//w 2 15 0 de60
        0x0140,	//w 1 15 0 0140
        0x0077 } },{	//w 0 15 0 0077
        { 0xa000,	//w 4 15 12 a
        0xdf01,	//w 3 15 0 df01
        0xdf20,	//w 2 15 0 df20
        0xff95,	//w 1 15 0 ff95
        0xfa00 } },{	//w 0 15 0 fa00
        { 0xb000,	//w 4 15 12 b
        0xff41,	//w 3 15 0 ff41
        0xde20,	//w 2 15 0 de20
        0x0140,	//w 1 15 0 0140
        0x00bb } },{	//w 0 15 0 00bb
        { 0xf000,	//w 4 15 12 f
        0xdf01,	//w 3 15 0 df01
        0xdf20,	//w 2 15 0 df20
        0xff95,	//w 1 15 0 ff95
        0xbf00 }	//w 0 15 0 bf00
        }
    }, *p = phy_magic;
    int i;

    rtl8169_print_mac_version(tp);
    rtl8169_print_phy_version(tp);

    if (tp->mac_version <= RTL_GIGA_MAC_VER_01)
        return;
    if (tp->phy_version >= RTL_GIGA_PHY_VER_H)
        return;

    SHOW_FLOW0( 0,  "MAC version != 0 && PHY version == 0 or 1");
    SHOW_FLOW0( 0, "Do final_reg2.cfg");

    /* Shazam ! */

    if (tp->mac_version == RTL_GIGA_MAC_VER_04) {
        mdio_write(ioaddr, 31, 0x0001);
        mdio_write(ioaddr,  9, 0x273a);
        mdio_write(ioaddr, 14, 0x7bfb);
        mdio_write(ioaddr, 27, 0x841e);

        mdio_write(ioaddr, 31, 0x0002);
        mdio_write(ioaddr,  1, 0x90d0);
        mdio_write(ioaddr, 31, 0x0000);
        return;
    }

    /* phy config for RTL8169s mac_version C chip */
    mdio_write(ioaddr, 31, 0x0001);			//w 31 2 0 1
    mdio_write(ioaddr, 21, 0x1000);			//w 21 15 0 1000
    mdio_write(ioaddr, 24, 0x65c7);			//w 24 15 0 65c7
    rtl8169_write_gmii_reg_bit(ioaddr, 4, 11, 0);	//w 4 11 11 0

    for (i = 0; i < ARRAY_SIZE(phy_magic); i++, p++) {
        int val, pos = 4;

        val = (mdio_read(ioaddr, pos) & 0x0fff) | (p->regs[0] & 0xffff);
        mdio_write(ioaddr, pos, val);
        while (--pos >= 0)
            mdio_write(ioaddr, pos, p->regs[4 - pos] & 0xffff);
        rtl8169_write_gmii_reg_bit(ioaddr, 4, 11, 1); //w 4 11 11 1
        rtl8169_write_gmii_reg_bit(ioaddr, 4, 11, 0); //w 4 11 11 0
    }
    mdio_write(ioaddr, 31, 0x0000); //w 31 2 0 0
}

static void rtl8169_phy_timer(void * __opaque)
{
    struct net_device *dev = (struct net_device *)__opaque;
    struct rtl8169_private *tp = dev->priv;
    void *ioaddr = tp->mmio_addr;
    unsigned long timeout = RTL8169_PHY_TIMEOUT;

    assert(tp->mac_version > RTL_GIGA_MAC_VER_01);
    assert(tp->phy_version < RTL_GIGA_PHY_VER_H);

    if (!(tp->phy_1000_ctrl_reg & PHY_Cap_1000_Full))
        return;

    hal_spin_lock(&tp->lock);

    if (tp->phy_reset_pending(ioaddr)) {
        /*
         * A busy loop could burn quite a few cycles on nowadays CPU.
         * Let's delay the execution of the timer for a few ticks.
         */
        timeout = 100;
        goto out_mod_timer;
    }

    if (tp->link_ok(ioaddr))
        goto out_unlock;

    SHOW_FLOW( 0, "%s: PHY reset until link up", dev->name);

    tp->phy_reset_enable(ioaddr);

out_mod_timer:
    delete_timer( tp->timer );

    //tp->timer = create_timer();
    start_timer( &(tp->timer),  &rtl8169_phy_timer, dev, (timeout)*100, true );
out_unlock:
    hal_spin_unlock(&tp->lock);
}

static inline void rtl8169_delete_timer(struct net_device *dev)
{
    struct rtl8169_private *tp = dev->priv;

    if ((tp->mac_version <= RTL_GIGA_MAC_VER_01) ||
        (tp->phy_version >= RTL_GIGA_PHY_VER_H))
        return;

    delete_timer(tp->timer);
}

static inline void rtl8169_request_timer(struct net_device *dev)
{
    struct rtl8169_private *tp = dev->priv;

    if ((tp->mac_version <= RTL_GIGA_MAC_VER_01) ||
        (tp->phy_version >= RTL_GIGA_PHY_VER_H))
        return;

    //tp->timer = create_timer();
    start_timer( &(tp->timer),  &rtl8169_phy_timer, dev, (RTL8169_PHY_TIMEOUT)*10, true );
}

#if 0
static void rtl8169_release_board(PCI_Info_s *pdev, struct net_device *dev, void *ioaddr)
{
    struct rtl8169_private *tp = dev->priv;

    delete_area( tp->reg_area );
    release_device( dev->device_handle );
    kfree( tp );
    kfree( dev );
}
#endif

static int rtl8169_init_board(PCI_Info_s *pdev, struct net_device *dev, void **ioaddr_out)
{
    void *ioaddr = NULL;
    struct rtl8169_private *tp = dev->priv;
    int rc = 0, i;

    assert(ioaddr_out != NULL);

    /* enable device (incl. PCI PM wakeup and hotplug setup) */
    int pci_command;
    int new_command;

    pci_command = g_psBus->read_pci_config( pdev->nBus, pdev->nDevice, pdev->nFunction, PCI_COMMAND, 2 );
    new_command = pci_command | ( PCI_COMMAND_MEMORY & 7);
    if (pci_command != new_command) {
        SHOW_INFO( 0, "  The PCI BIOS has not enabled the"
                " device at %d/%d/%d!  Updating PCI command %4.4x->%4.4x.",
                pdev->nBus, pdev->nDevice, pdev->nFunction, pci_command, new_command );
        g_psBus->write_pci_config( pdev->nBus, pdev->nDevice, pdev->nFunction, PCI_COMMAND, 2, new_command);
    }

    tp->cp_cmd = PCIMulRW | RxChkSum;

    g_psBus->enable_pci_master( pdev->nBus, pdev->nDevice, pdev->nFunction );

    /* allocate register area (Syllable way) */
    long addr;
    switch( tp->region )
    {
    default:
    case 1:
        addr = pdev->u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK;
        break;
    case 2:
        addr = pdev->u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK;
        break;
    }
    tp->reg_area = create_area ("r8169_register", (void **)&ioaddr,
                                PAGE_SIZE * 2, PAGE_SIZE * 2, AREA_KERNEL|AREA_ANY_ADDRESS, AREA_NO_LOCK);
    if( tp->reg_area < 0 ) {
        SHOW_FLOW( 0, "r8169: failed to create register area (%d)", tp->reg_area);
        goto out;
    }

    if( remap_area (tp->reg_area, (void *)(addr & PAGE_MASK)) < 0 ) {
        SHOW_FLOW( 0, "r8169: failed to remap register area (%d)", tp->reg_area);
        goto out;
    }

    dev->base_addr = (unsigned long)ioaddr + ( addr - ( addr & PAGE_MASK ) );
    ioaddr = (void*)dev->base_addr;

    /* Unneeded ? Don't mess with Mrs. Murphy. */
    rtl8169_irq_mask_and_ack(ioaddr);

    /* Soft reset the chip. */
    RTL_W8(ChipCmd, CmdReset);

    /* Check that the chip has finished the reset. */
    for (i = 1000; i > 0; i--) {
        if ((RTL_R8(ChipCmd) & CmdReset) == 0)
            break;
        udelay(10);
    }

    /* Identify chip attached to board */
    rtl8169_get_mac_version(tp, ioaddr);
    rtl8169_get_phy_version(tp, ioaddr);

    rtl8169_print_mac_version(tp);
    rtl8169_print_phy_version(tp);

    for (i = ARRAY_SIZE(rtl_chip_info) - 1; i >= 0; i--) {
        if (tp->mac_version == rtl_chip_info[i].mac_version)
            break;
    }
    if (i < 0) {
        /* Unknown chip: assume array element #0, original RTL-8169 */
        SHOW_FLOW( 0, "r8169: unknown chip version, assuming %s", rtl_chip_info[0].name);
        i++;
    }
    tp->chipset = i;
    dev->name = rtl_chip_info[tp->chipset].name;

    *ioaddr_out = ioaddr;
out:
    return rc;
}

static int rtl8169_init_one(PCI_Info_s *pdev, struct net_device *dev)
{
    struct rtl8169_private *tp;
    void *ioaddr = NULL;
    static int board_idx = -1;
    int i, rc;

    assert(pdev != NULL);

    board_idx++;

    rc = rtl8169_init_board(pdev, dev, &ioaddr);
    if (rc)
        return rc;

    tp = dev->priv;
    assert(ioaddr != NULL);

    if (RTL_R8(PHYstatus) & TBI_Enable) {
        tp->set_speed = rtl8169_set_speed_tbi;
        tp->phy_reset_enable = rtl8169_tbi_reset_enable;
        tp->phy_reset_pending = rtl8169_tbi_reset_pending;
        tp->link_ok = rtl8169_tbi_link_ok;

        tp->phy_1000_ctrl_reg = PHY_Cap_1000_Full; /* Implied by TBI */
    } else {
        tp->set_speed = rtl8169_set_speed_xmii;
        tp->phy_reset_enable = rtl8169_xmii_reset_enable;
        tp->phy_reset_pending = rtl8169_xmii_reset_pending;
        tp->link_ok = rtl8169_xmii_link_ok;
    }

    /* Get MAC address.  FIXME: read EEPROM */
    for (i = 0; i < MAC_ADDR_LEN; i++)
        dev->dev_addr[i] = RTL_R8(MAC0 + i);
    memcpy(dev->perm_addr, dev->dev_addr, dev->addr_len);

    tp->pci_dev = malloc( sizeof( PCI_Info_s ) );
    if( NULL == tp->pci_dev )
        return ENOMEM;
    memcpy( tp->pci_dev, pdev, sizeof( pdev ) );

    dev->irq = pdev->u.h0.nInterruptLine;
    tp->intr_mask = 0xffff;
    tp->mmio_addr = ioaddr;

    //spinlock_init(&tp->lock, "r8169_lock");
    hal_spin_init(&tp->lock);

    SHOW_FLOW( 0, "%s: Identified chip type is '%s'.", dev->name, rtl_chip_info[tp->chipset].name);

    SHOW_FLOW( 0, "%s: RTL8169 at 0x%lx, "
            "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x, "
            "IRQ %d",
            dev->name,
            dev->base_addr,
            dev->dev_addr[0], dev->dev_addr[1],
            dev->dev_addr[2], dev->dev_addr[3],
            dev->dev_addr[4], dev->dev_addr[5], dev->irq);

    rtl8169_hw_phy_config(dev);

    SHOW_FLOW0( 0, "Set MAC Reg C+CR Offset 0x82h = 0x01h");
    RTL_W8(0x82, 0x01);

    if (tp->mac_version < RTL_GIGA_MAC_VER_03) {
        SHOW_FLOW0( 0, "Set PCI Latency=0x40");
        g_psBus->write_pci_config( pdev->nBus, pdev->nDevice, pdev->nFunction, PCI_LATENCY, 1, 0x40);
    }

    if (tp->mac_version == RTL_GIGA_MAC_VER_02) {
        SHOW_FLOW0( 0, "Set MAC Reg C+CR Offset 0x82h = 0x01h");
        RTL_W8(0x82, 0x01);
        SHOW_FLOW0( 0, "Set PHY Reg 0x0bh = 0x00h");
        mdio_write(ioaddr, 0x0b, 0x0000); //w 0x0b 15 0 0
    }

    /* XXXKV: Always autoneg */
    rtl8169_set_speed( dev, AUTONEG_ENABLE, SPEED_1000, DUPLEX_FULL );

    if (RTL_R8(PHYstatus) & TBI_Enable)
        SHOW_FLOW( 0, "%s: TBI auto-negotiating", dev->name);

    return 0;
}

#if 0
static void rtl8169_remove_one(PCI_Info_s *pdev, struct net_device *dev)
{
    struct rtl8169_private *tp = dev->priv;

    assert(dev != NULL);
    assert(tp != NULL);

    rtl8169_release_board(pdev, dev, tp->mmio_addr);
}
#endif

static void rtl8169_set_rxbufsize(struct rtl8169_private *tp, struct net_device *dev)
{
    unsigned int mtu = dev->mtu;

    tp->rx_buf_sz = (mtu > RX_BUF_SIZE) ? mtu + ETHERNET_HEADER_SIZE + 8 : RX_BUF_SIZE;
}

static int rtl8169_open(struct net_device *dev)
{
    struct rtl8169_private *tp = dev->priv;
    int retval = 0;

    SHOW_FLOW( 0, "rtl8169_open() for %s on IRQ %d", dev->name, dev->irq );

    rtl8169_set_rxbufsize(tp, dev);

    dev->irq_handle = request_irq(dev->irq, &rtl8169_interrupt, NULL, SA_SHIRQ, dev->name, dev);
    if (dev->irq_handle < 0)
        goto out;

    retval = -ENOMEM;

    /*
     * Rx and Tx desscriptors needs 256 bytes alignment.
     * pci_alloc_consistent provides more.
     */
    tp->TxDesc = malloc( R8169_TX_RING_BYTES );
    tp->TxDescArray = ALIGN( ((long)tp->TxDesc), 255 );
    tp->TxPhyAddr = (dma_addr_t)tp->TxDescArray;
    if (!tp->TxDescArray)
        goto err_free_irq;

    tp->RxDesc = malloc( R8169_RX_RING_BYTES );
    tp->RxDescArray = ALIGN( ((long)tp->RxDesc), 255 );
    tp->RxPhyAddr = (dma_addr_t)tp->RxDescArray;
    if (!tp->RxDescArray)
        goto err_free_tx;

    retval = rtl8169_init_ring(dev);
    if (retval < 0)
        goto err_free_rx;

    rtl8169_hw_start(dev);

    rtl8169_request_timer(dev);

    rtl8169_check_link_status(dev, tp, tp->mmio_addr);
out:
    return retval;

err_free_rx:
    pci_free_consistent(pdev, R8169_RX_RING_BYTES, tp->RxDescArray,
                        tp->RxPhyAddr);
err_free_tx:
    pci_free_consistent(pdev, R8169_TX_RING_BYTES, tp->TxDescArray,
                        tp->TxPhyAddr);
err_free_irq:
    release_irq(dev->irq, dev->irq_handle);
    goto out;
}

static void rtl8169_hw_reset(void *ioaddr)
{
    /* Disable interrupts */
    rtl8169_irq_mask_and_ack(ioaddr);

    /* Reset the chipset */
    RTL_W8(ChipCmd, CmdReset);

    /* PCI commit */
    RTL_R8(ChipCmd);
}

static void rtl8169_hw_start(struct net_device *dev)
{
    struct rtl8169_private *tp = dev->priv;
    void *ioaddr = tp->mmio_addr;
    uint32 i;
    PCI_Info_s *pdev = tp->pci_dev;

    /* Soft reset the chip. */
    RTL_W8(ChipCmd, CmdReset);

    /* Check that the chip has finished the reset. */
    for (i = 1000; i > 0; i--) {
        if ((RTL_R8(ChipCmd) & CmdReset) == 0)
            break;
        udelay(10);
    }

    if (tp->mac_version == RTL_GIGA_MAC_VER_13) {
        g_psBus->write_pci_config( pdev->nBus, pdev->nDevice, pdev->nFunction, 0x68, 2, 0x00);
        g_psBus->write_pci_config( pdev->nBus, pdev->nDevice, pdev->nFunction, 0x69, 2, 0x08);
    }

    /* Undocumented stuff. */
    if (tp->mac_version == RTL_GIGA_MAC_VER_05) {
        uint16 cmd;

        /* Realtek's r1000_n.c driver uses '&& 0x01' here. Well... */
        if ((RTL_R8(Config2) & 0x07) & 0x01)
            RTL_W32(0x7c, 0x0007ffff);

        RTL_W32(0x7c, 0x0007ff00);

        cmd = g_psBus->read_pci_config(pdev->nBus, pdev->nDevice, pdev->nFunction, PCI_COMMAND, 2);
        cmd = cmd & 0xef;
        g_psBus->write_pci_config( pdev->nBus, pdev->nDevice, pdev->nFunction, PCI_COMMAND, 2, cmd);
    }


    RTL_W8(Cfg9346, Cfg9346_Unlock);
    RTL_W8(EarlyTxThres, EarlyTxThld);

    /* Low hurts. Let's disable the filtering. */
    RTL_W16(RxMaxSize, 16383);

    /* Set Rx Config register */
    i = rtl8169_rx_config |
        (RTL_R32(RxConfig) & rtl_chip_info[tp->chipset].RxConfigMask);
    RTL_W32(RxConfig, i);

    /* Set DMA burst size and Interframe Gap Time */
    RTL_W32(TxConfig,
            (TX_DMA_BURST << TxDMAShift) | (InterFrameGap <<
                                            TxInterFrameGapShift));
    tp->cp_cmd |= RTL_R16(CPlusCmd) | PCIMulRW;

    if ((tp->mac_version == RTL_GIGA_MAC_VER_02) ||
        (tp->mac_version == RTL_GIGA_MAC_VER_03)) {
        SHOW_FLOW0( 0, "Set MAC Reg C+CR Offset 0xE0. "
                 "Bit-3 and bit-14 MUST be 1");
        tp->cp_cmd |= (1 << 14);
    }

    RTL_W16(CPlusCmd, tp->cp_cmd);

    /*
     * Undocumented corner. Supposedly:
     * (TxTimer << 12) | (TxPackets << 8) | (RxTimer << 4) | RxPackets
     */
    RTL_W16(IntrMitigate, 0x0000);

    RTL_W32(TxDescStartAddrLow, ((u64) tp->TxPhyAddr & DMA_32BIT_MASK));
    RTL_W32(TxDescStartAddrHigh, ((u64) tp->TxPhyAddr >> 32));
    RTL_W32(RxDescAddrLow, ((u64) tp->RxPhyAddr & DMA_32BIT_MASK));
    RTL_W32(RxDescAddrHigh, ((u64) tp->RxPhyAddr >> 32));
    RTL_W8(ChipCmd, CmdTxEnb | CmdRxEnb);
    RTL_W8(Cfg9346, Cfg9346_Lock);
    udelay(10);

    RTL_W32(RxMissed, 0);

    rtl8169_set_rx_mode(dev);

    /* no early-rx interrupts */
    RTL_W16(MultiIntr, RTL_R16(MultiIntr) & 0xF000);

    /* Enable all known interrupts by setting the interrupt mask. */
    RTL_W16(IntrMask, rtl8169_intr_mask);

    netif_start_queue(dev);
}

#if 0
static int rtl8169_change_mtu(struct net_device *dev, int new_mtu)
{
    struct rtl8169_private *tp = dev->priv;
    int ret = 0;

    if (new_mtu < ETH_ZLEN || new_mtu > SafeMtu)
        return -EINVAL;

    dev->mtu = new_mtu;

    if (!netif_running(dev))
        goto out;

    rtl8169_down(dev);

    rtl8169_set_rxbufsize(tp, dev);

    ret = rtl8169_init_ring(dev);
    if (ret < 0)
        goto out;

    rtl8169_hw_start(dev);

    rtl8169_request_timer(dev);

out:
    return ret;
}
#endif

static inline void rtl8169_make_unusable_by_asic(struct RxDesc *desc)
{
    desc->addr = 0x0badbadbadbadbadull;
    desc->opts1 &= ~cpu_to_le32(DescOwn | RsvdMask);
}

/*
static void rtl8169_free_rx_skb(struct rtl8169_private *tp, PacketBuf_s **sk_buff, struct RxDesc *desc)
{
    free_pkt_buffer(*sk_buff);
    *sk_buff = NULL;
    rtl8169_make_unusable_by_asic(desc);
}
*/

static inline void rtl8169_mark_to_asic(struct RxDesc *desc, uint32 rx_buf_sz)
{
    uint32 eor = le32_to_cpu(desc->opts1) & RingEnd;

    desc->opts1 = cpu_to_le32(DescOwn | eor | rx_buf_sz);
}

static inline void rtl8169_map_to_asic(struct RxDesc *desc, dma_addr_t mapping, uint32 rx_buf_sz)
{
    desc->addr = cpu_to_le64(mapping);
    //wmb();
    rtl8169_mark_to_asic(desc, rx_buf_sz);
}

#if 0
static int rtl8169_alloc_rx_skb(PCI_Info_s *pdev, PacketBuf_s **sk_buff, struct RxDesc *desc, int rx_buf_sz, unsigned int align)
{
    PacketBuf_s *skb;
    dma_addr_t mapping;
    int ret = 0;

    skb = alloc_pkt_buffer(rx_buf_sz + align);
    if (!skb)
        goto err_out;

    *sk_buff = skb;

    /* XXXKV: The following is the equivilent of skb_reserve() and means the MAC header starts at pb_pHead */
    skb->pb_pHead = skb->pb_pData + ( (align - 1) & (unsigned long)skb->pb_pData );
    mapping = pci_map_single(pdev, skb->pb_pHead, rx_buf_sz,
                             PCI_DMA_FROMDEVICE);

    rtl8169_map_to_asic(desc, mapping, rx_buf_sz);

out:
    return ret;

err_out:
    ret = -ENOMEM;
    rtl8169_make_unusable_by_asic(desc);
    goto out;
}
#endif

static void rtl8169_rx_clear(struct rtl8169_private *tp)
{
    int i;

    for (i = 0; i < NUM_RX_DESC; i++) {
        if (tp->Rx_skbuff[i]) {
            rtl8169_free_rx_skb(tp, tp->Rx_skbuff + i,
                                tp->RxDescArray + i);
        }
    }
}

static uint32 rtl8169_rx_fill(struct rtl8169_private *tp, struct net_device *dev, uint32 start, uint32 end)
{
    uint32 cur;

    for (cur = start; end - cur > 0; cur++) {
        int ret, i = cur % NUM_RX_DESC;

        if (tp->Rx_skbuff[i])
            continue;

        ret = rtl8169_alloc_rx_skb(tp->pci_dev, tp->Rx_skbuff + i,
                                   tp->RxDescArray + i, tp->rx_buf_sz, tp->align);
        if (ret < 0)
            break;
    }
    return cur - start;
}

static inline void rtl8169_mark_as_last_descriptor(struct RxDesc *desc)
{
    desc->opts1 |= cpu_to_le32(RingEnd);
}

static void rtl8169_init_ring_indexes(struct rtl8169_private *tp)
{
    tp->dirty_tx = tp->dirty_rx = tp->cur_tx = tp->cur_rx = 0;
}

static int rtl8169_init_ring(struct net_device *dev)
{
    struct rtl8169_private *tp = dev->priv;

    rtl8169_init_ring_indexes(tp);

    memset(tp->tx_skb, 0x0, NUM_TX_DESC * sizeof(struct ring_info));
    memset(tp->Rx_skbuff, 0x0, NUM_RX_DESC * sizeof(PacketBuf_s *));

    if (rtl8169_rx_fill(tp, dev, 0, NUM_RX_DESC) != NUM_RX_DESC)
        goto err_out;

    rtl8169_mark_as_last_descriptor(tp->RxDescArray + NUM_RX_DESC - 1);

    return 0;

err_out:
    rtl8169_rx_clear(tp);
    return -ENOMEM;
}

static void rtl8169_unmap_tx_skb(PCI_Info_s *pdev, struct ring_info *tx_skb, struct TxDesc *desc)
{
    pci_unmap_single(pdev, le64_to_cpu(desc->addr), tx_skb->len, PCI_DMA_TODEVICE);
    desc->opts1 = 0x00;
    desc->opts2 = 0x00;
    desc->addr = 0x00;
    tx_skb->len = 0;
}

static void rtl8169_tx_clear(struct rtl8169_private *tp)
{
    unsigned int i;

    for (i = tp->dirty_tx; i < tp->dirty_tx + NUM_TX_DESC; i++) {
        unsigned int entry = i % NUM_TX_DESC;
        struct ring_info *tx_skb = tp->tx_skb + entry;
        unsigned int len = tx_skb->len;

        if (len) {
            PacketBuf_s *skb = tx_skb->skb;

            rtl8169_unmap_tx_skb(tp->pci_dev, tx_skb,
                                 tp->TxDescArray + entry);
            if (skb) {
                free_pkt_buffer(skb);
                tx_skb->skb = NULL;
            }
        }
    }
    tp->cur_tx = tp->dirty_tx = 0;
}

static void rtl8169_schedule_work(struct net_device *dev, void (*task)(void *))
{
    /* XXXKV: Warning!  If rtl8169_reinit_task() is ever scheduled it may get into a tight loop by calling itself */
    task( dev );
}

/* Only called on PCI error; see tl8169_pcierr_interrupt() */
static void rtl8169_reinit_task(void *_data)
{
    struct net_device *dev = _data;
    int ret;

    if (netif_running(dev)) {
        rtl8169_close(dev);
    }

    ret = rtl8169_open(dev);
    if (ret < 0) {
        rtl8169_schedule_work(dev, rtl8169_reinit_task);
    }
}

#if 0
static inline uint32 rtl8169_tso_csum(PacketBuf_s *skb, struct net_device *dev)
{
    /* XXXV: We don't have an option for hardware checksumming */
    return 0;
}
#endif

static int rtl8169_start_xmit(PacketBuf_s *skb, struct net_device *dev)
{
    struct rtl8169_private *tp = dev->priv;
    unsigned int entry = tp->cur_tx % NUM_TX_DESC;
    struct TxDesc *txd = tp->TxDescArray + entry;
    void *ioaddr = tp->mmio_addr;
    dma_addr_t mapping;
    uint32 status, len;
    uint32 opts1;
    int ret = 0;

    if (le32_to_cpu(txd->opts1) & DescOwn)
        goto err_stop;

    opts1 = DescOwn | rtl8169_tso_csum(skb, dev);
    len = skb->pb_nSize;

    opts1 |= FirstFrag | LastFrag;
    tp->tx_skb[entry].skb = skb;

    mapping = pci_map_single(tp->pci_dev, skb->pb_pData, len, PCI_DMA_TODEVICE);

    tp->tx_skb[entry].len = len;
    txd->addr = cpu_to_le64(mapping);
    txd->opts2 = cpu_to_le32(rtl8169_tx_vlan_tag(tp, skb));

    //wmb();

    /* anti gcc 2.95.3 bugware (sic) */
    status = opts1 | len | (RingEnd * !((entry + 1) % NUM_TX_DESC));
    txd->opts1 = cpu_to_le32(status);

    dev->trans_start = jiffies;

    tp->cur_tx += 1;

    //smp_wmb();

    RTL_W8(TxPoll, 0x40);	/* set polling bit */

    if (TX_BUFFS_AVAIL(tp) < MAX_SKB_FRAGS) {
        netif_stop_queue(dev);
        //smp_rmb();
        if (TX_BUFFS_AVAIL(tp) >= MAX_SKB_FRAGS)
            netif_wake_queue(dev);
    }

    return ret;

err_stop:
    netif_stop_queue(dev);
    return 1;
}

static void rtl8169_pcierr_interrupt(struct net_device *dev)
{
    struct rtl8169_private *tp = dev->priv;
    PCI_Info_s *pdev = tp->pci_dev;
    void *ioaddr = tp->mmio_addr;
    uint16 pci_status, pci_cmd;

    pci_cmd = g_psBus->read_pci_config( pdev->nBus, pdev->nDevice, pdev->nFunction, PCI_COMMAND, 2 );
    pci_status = g_psBus->read_pci_config( pdev->nBus, pdev->nDevice, pdev->nFunction, PCI_STATUS, 2 );

    SHOW_FLOW( 0, "%s: PCI error (cmd = 0x%04x, status = 0x%04x).", dev->name, pci_cmd, pci_status);

    /*
     * The recovery sequence below admits a very elaborated explanation:
     * - it seems to work;
     * - I did not see what else could be done.
     *
     * Feel free to adjust to your needs.
     */
    g_psBus->write_pci_config( pdev->nBus, pdev->nDevice, pdev->nFunction, PCI_COMMAND, 2, pci_cmd | PCI_COMMAND_SERR | PCI_COMMAND_PARITY);

    g_psBus->write_pci_config( pdev->nBus, pdev->nDevice, pdev->nFunction, PCI_STATUS, 2,
                               pci_status & (PCI_STATUS_PARITY_ERROR_DETECTED |
                                             PCI_STATUS_SERR_SIGNALLED | PCI_STATUS_MASTER_ABORT_RECEIVED |
                                             PCI_STATUS_TARGET_ABORT_RECEIVED | PCI_STATUS_TARGET_ABORT_SIGNALLED));

    /* The infamous DAC f*ckup only happens at boot time */
    if ((tp->cp_cmd & PCIDAC) && !tp->dirty_rx && !tp->cur_rx) {
        SHOW_FLOW( 0, "%s: disabling PCI DAC.", dev->name);
        tp->cp_cmd &= ~PCIDAC;
        RTL_W16(CPlusCmd, tp->cp_cmd);
        rtl8169_schedule_work(dev, rtl8169_reinit_task);
    }

    rtl8169_hw_reset(ioaddr);
}

static void rtl8169_tx_interrupt(struct net_device *dev, struct rtl8169_private *tp, void *ioaddr)
{
    unsigned int dirty_tx, tx_left;

    assert(dev != NULL);
    assert(tp != NULL);
    assert(ioaddr != NULL);

    SHOW_FLOW0( 0, "rtl8169_tx_interrupt()" );

    dirty_tx = tp->dirty_tx;
    //smp_rmb();
    tx_left = tp->cur_tx - dirty_tx;

    while (tx_left > 0) {
        unsigned int entry = dirty_tx % NUM_TX_DESC;
        struct ring_info *tx_skb = tp->tx_skb + entry;
        uint32 status;

        //rmb();
        status = le32_to_cpu(tp->TxDescArray[entry].opts1);
        if (status & DescOwn)
            break;

        rtl8169_unmap_tx_skb(tp->pci_dev, tx_skb, tp->TxDescArray + entry);

        if (status & LastFrag) {
            free_pkt_buffer(tx_skb->skb);
            tx_skb->skb = NULL;
        }
        dirty_tx++;
        tx_left--;
    }

    if (tp->dirty_tx != dirty_tx) {
        tp->dirty_tx = dirty_tx;
        //smp_wmb();
        if (netif_queue_stopped(dev) &&
            (TX_BUFFS_AVAIL(tp) >= MAX_SKB_FRAGS)) {
            netif_wake_queue(dev);
        }
    }
}

static inline int rtl8169_fragmented_frame(uint32 status)
{
    return (status & (FirstFrag | LastFrag)) != (FirstFrag | LastFrag);
}

static inline int rtl8169_try_rx_copy(PacketBuf_s **sk_buff, int pkt_size, struct RxDesc *desc, int rx_buf_sz, unsigned int align)
{
    int ret = -1;

    if (pkt_size < rx_copybreak) {
        PacketBuf_s *skb;

        skb = alloc_pkt_buffer(pkt_size + align);
        if (skb) {
            skb->pb_nSize = 0;
            skb->pb_pHead = skb->pb_pData + ( (align - 1) & (unsigned long)skb->pb_pData );
#if 0
            /* XXXKV: Dump the incoming packet; useful for debugging */
            int i;
            for(i=0;i<pkt_size;i+=8)
            {
                uint8 *p = (uint8*)desc->addr;
                printk( "%x:%x:%x:%x:%x:%x:%x:%x\n", p[i], p[i+1], p[i+2], p[i+3], p[i+4], p[i+5], p[i+6], p[i+7] );
            }
#endif
            skb_put(skb, pkt_size);
            memcpy(skb->pb_pHead, sk_buff[0]->pb_pHead, pkt_size);

            *sk_buff = skb;
            rtl8169_mark_to_asic(desc, rx_buf_sz);
            ret = 0;
        }
    }

    return ret;
}

static int rtl8169_rx_interrupt(struct net_device *dev, struct rtl8169_private *tp, void *ioaddr)
{
    unsigned int cur_rx, rx_left;
    unsigned int delta, count;

    assert(dev != NULL);
    assert(tp != NULL);
    assert(ioaddr != NULL);

    cur_rx = tp->cur_rx;
    rx_left = NUM_RX_DESC + tp->dirty_rx - cur_rx;
    rx_left = rtl8169_rx_quota(rx_left, (uint32) dev->quota);

    SHOW_FLOW( 0, "rtl8169_rx_interrupt() rx_left=%d", rx_left );

    for (; rx_left > 0; rx_left--, cur_rx++) {
        unsigned int entry = cur_rx % NUM_RX_DESC;
        struct RxDesc *desc = tp->RxDescArray + entry;
        uint32 status;

        //rmb();
        status = le32_to_cpu(desc->opts1);

        if (status & DescOwn)
            break;
        if (status & RxRES) {
            SHOW_FLOW( 0, "%s: Rx ERROR. status = %08x", dev->name, status);
            rtl8169_mark_to_asic(desc, tp->rx_buf_sz);
        } else {
            PacketBuf_s *skb = tp->Rx_skbuff[entry];
            int pkt_size = (status & 0x00001FFF) - 4;

            /*
             * The driver does not support incoming fragmented
             * frames. They are seen as a symptom of over-mtu
             * sized frames.
             */
            if (rtl8169_fragmented_frame(status)) {
                rtl8169_mark_to_asic(desc, tp->rx_buf_sz);
                continue;
            }

            if (rtl8169_try_rx_copy(&skb, pkt_size, desc,
                                    tp->rx_buf_sz, tp->align)) {
                tp->Rx_skbuff[entry] = NULL;
            }

            skb_put(skb, pkt_size);

            if ( dev->packet_queue != NULL ) {
                skb->pb_uMacHdr.pRaw = skb->pb_pHead;
                enqueue_packet( dev->packet_queue, skb );
            } else {
                SHOW_FLOW0( 0, "Error: rtl8169_rx_interrupt() received packet to downed device!" );
                free_pkt_buffer( skb );
            }

            dev->last_rx = jiffies;
        }
    }

    count = cur_rx - tp->cur_rx;
    tp->cur_rx = cur_rx;

    delta = rtl8169_rx_fill(tp, dev, tp->dirty_rx, tp->cur_rx);
    if (!delta && count )
        SHOW_FLOW( 0, "%s: no Rx buffer allocated", dev->name);
    tp->dirty_rx += delta;

    /*
     * FIXME: until there is periodic timer to try and refill the ring,
     * a temporary shortage may definitely kill the Rx process.
     * - disable the asic to try and avoid an overflow and kick it again
     *   after refill ?
     * - how do others driver handle this condition (Uh oh...).
     */
    if (tp->dirty_rx + NUM_RX_DESC == tp->cur_rx)
        SHOW_ERROR( 0, "%s: Rx buffers exhausted", dev->name);

    return count;
}

/* The interrupt handler does all of the Rx thread work and cleans up after the Tx thread. */
static int rtl8169_interrupt(int irq, void *dev_instance, SysCallRegs_s *regs)
{
    struct net_device *dev = (struct net_device *) dev_instance;
    struct rtl8169_private *tp = dev->priv;
    int boguscnt = max_interrupt_work;
    void*ioaddr = tp->mmio_addr;
    int status;
    int handled = 0;

    do {
        status = RTL_R16(IntrStatus);

        /* hotplug/major error/no more work/shared irq */
        if ((status == 0xFFFF) || !status)
            break;

        handled = 1;

        if (!netif_running(dev)) {
            rtl8169_asic_down(ioaddr);
            goto out;
        }

        status &= tp->intr_mask;
        RTL_W16(IntrStatus,
                (status & RxFIFOOver) ? (status | RxOverflow) : status);

        if (!(status & rtl8169_intr_mask))
            break;

        SHOW_FLOW( 0, "rtl8169_interrupt() status is 0x%.8x", status );

        if (status & SYSErr) {
            rtl8169_pcierr_interrupt(dev);
            break;
        }

        if (status & LinkChg)
            rtl8169_check_link_status(dev, tp, ioaddr);

        /* Rx interrupt */
        if (status & (RxOK | RxOverflow | RxFIFOOver)) {
            rtl8169_rx_interrupt(dev, tp, ioaddr);
        }

        /* Tx interrupt */
        if (status & (TxOK | TxErr))
            rtl8169_tx_interrupt(dev, tp, ioaddr);

        boguscnt--;
    } while (boguscnt > 0);

    if (boguscnt <= 0) {
        SHOW_ERROR( 0, "%s: Too much work at interrupt!", dev->name);

        /* Clear all interrupt sources. */
        RTL_W16(IntrStatus, 0xffff);
    }
out:
    return 0;
}

static void rtl8169_down(struct net_device *dev)
{
    struct rtl8169_private *tp = dev->priv;
    void *ioaddr = tp->mmio_addr;
    unsigned int poll_locked = 0;

    rtl8169_delete_timer(dev);

    netif_stop_queue(dev);

core_down:
    hal_spin_lock(&tp->lock);

    rtl8169_asic_down(ioaddr);

    /* Update the error counts. */
    RTL_W32(RxMissed, 0);

    hal_spin_unlock(&tp->lock);

    if (!poll_locked) {
        poll_locked++;
    }

    /*
     * And now for the 50k$ question: are IRQ disabled or not ?
     *
     * Two paths lead here:
     * 1) dev->close
     *    -> netif_running() is available to sync the current code and the
     *       IRQ handler. See rtl8169_interrupt for details.
     * 2) dev->change_mtu
     *    -> rtl8169_poll can not be issued again and re-enable the
     *       interruptions. Let's simply issue the IRQ down sequence again.
     */
    if (RTL_R16(IntrMask))
        goto core_down;

    rtl8169_tx_clear(tp);

    rtl8169_rx_clear(tp);
}

static int rtl8169_close(struct net_device *dev)
{
    struct rtl8169_private *tp = dev->priv;

    rtl8169_down(dev);

    release_irq(dev->irq, dev->irq_handle);

    pci_free_consistent(pdev, R8169_RX_RING_BYTES, tp->RxDesc,
                        tp->RxPhyAddr);
    pci_free_consistent(pdev, R8169_TX_RING_BYTES, tp->TxDesc,
                        tp->TxPhyAddr);
    tp->TxDescArray = NULL;
    tp->RxDescArray = NULL;

    return 0;
}

static void rtl8169_set_rx_mode(struct net_device *dev)
{
    struct rtl8169_private *tp = dev->priv;
    void *ioaddr = tp->mmio_addr;
    unsigned long flags;
    uint32 mc_filter[2];	/* Multicast hash filter */
    int rx_mode;
    uint32 tmp = 0;

    /* Too many to filter perfectly -- accept all multicasts. */
    rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
    mc_filter[1] = mc_filter[0] = 0xffffffff;

    spinlock_cli(&tp->lock, flags);

    tmp = rtl8169_rx_config | rx_mode |
        (RTL_R32(RxConfig) & rtl_chip_info[tp->chipset].RxConfigMask);

    if ((tp->mac_version == RTL_GIGA_MAC_VER_11) ||
        (tp->mac_version == RTL_GIGA_MAC_VER_12) ||
        (tp->mac_version == RTL_GIGA_MAC_VER_13) ||
        (tp->mac_version == RTL_GIGA_MAC_VER_14) ||
        (tp->mac_version == RTL_GIGA_MAC_VER_15)) {
        mc_filter[0] = 0xffffffff;
        mc_filter[1] = 0xffffffff;
    }

    RTL_W32(RxConfig, tmp);
    RTL_W32(MAR0 + 0, mc_filter[0]);
    RTL_W32(MAR0 + 4, mc_filter[1]);

    spinunlock_restore(&tp->lock, flags);
}

/* Device interface */
static status_t r8169_open_dev( void* pNode, uint32 nFlags, void **pCookie )
{
    return 0;
}

static status_t r8169_close_dev( void* pNode, void* pCookie )
{
    return 0;
}

static status_t r8169_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
    struct net_device* psDev = pNode;
    int nError = 0;
#if 0
    switch( nCommand )
    {
    case SIOC_ETH_START:
        {
            psDev->packet_queue = pArgs;
            rtl8169_open( psDev );
            break;
        }
    case SIOC_ETH_STOP:
        {
            rtl8169_close( psDev );
            psDev->packet_queue = NULL;
            break;
        }
    case SIOCSIFHWADDR:
        nError = -ENOSYS;
        break;
    case SIOCGIFHWADDR:
        memcpy( ((struct ifreq*)pArgs)->ifr_hwaddr.sa_data, psDev->dev_addr, 6 );
        break;
    default:
        SHOW_ERROR( 0, "r8169_ioctl() unknown command %d", (int)nCommand );
        nError = -ENOSYS;
        break;
    }

    return nError;
#endif
    return EINVAL;
}

static int r8169_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
    return -ENOSYS;
}

static int r8169_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
    struct net_device* dev = pNode;
    PacketBuf_s* psBuffer = alloc_pkt_buffer( nSize );
    if ( psBuffer != NULL ) {
        memcpy( psBuffer->pb_pData, pBuffer, nSize );
        psBuffer->pb_nSize = nSize;
        rtl8169_start_xmit( psBuffer, dev );
    }
    return nSize;
}

static DeviceOperations_s g_sDevOps = {
    r8169_open_dev,
    r8169_close_dev,
    r8169_ioctl,
    r8169_read,
    r8169_write,
    NULL,	// dop_readv
    NULL,	// dop_writev
    NULL,	// dop_add_select_req
    NULL	// dop_rem_select_req
};

static int r8169_probe( int nDeviceID )
{
    struct net_device *dev;
    struct rtl8169_private *tp = NULL;
    int cards_found = 0;

    int i;
    PCI_Info_s sInfo;

    for ( i = 0 ; g_psBus->get_pci_info( &sInfo, i ) == 0 ; ++i )
    {
        int chip_idx;
        for ( chip_idx = 0 ; rtl8169_pci_tbl[chip_idx].vendor_id ; chip_idx++ )
        {
            if ( sInfo.nVendorID == rtl8169_pci_tbl[chip_idx].vendor_id &&
                 sInfo.nDeviceID == rtl8169_pci_tbl[chip_idx].device_id)
            {
                char node_path[64];

                if( claim_device( nDeviceID, sInfo.nHandle, "Realtek 8169", DEVICE_NET ) < 0 )
                    continue;

                dev = calloc( sizeof(struct net_device), 1 );
                if (!dev)
                    continue;
                dev->name = "r8169";
                dev->device_handle = nDeviceID;
                dev->mtu = ETH_FRAME_LEN;

                if ((dev->priv = malloc(sizeof(struct rtl8169_private))) == NULL)
                    continue;
                tp = dev->priv;

                int cfg = rtl8169_pci_tbl[chip_idx].driver_data;
                tp->region = rtl_cfg_info[cfg].region;
                tp->align = rtl_cfg_info[cfg].align;

                if( rtl8169_init_one( &sInfo, dev ) == 0 )
                {
                    snprintf( node_path, 64, "net/eth/r8169-%d", cards_found );
                    dev->node_handle = create_device_node( nDeviceID, tp->pci_dev->nHandle,
                                                           node_path, &g_sDevOps, dev );
                    SHOW_FLOW( 0, "r8169_probe() Create node: %s", node_path );

                    cards_found++;
                }
            }
        }
    }

    if( !cards_found )
        disable_device( nDeviceID );

    return cards_found ? 0 : -ENODEV;
}

status_t device_init( int nDeviceID )
{
    /* Get PCI bus */
    // g_psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
    // if( g_psBus == NULL )        return( -1 );

    return r8169_probe( nDeviceID );
}

status_t device_uninit( int nDeviceID )
{
    return 0;
}


#endif
