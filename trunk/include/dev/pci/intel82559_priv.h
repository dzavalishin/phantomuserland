#ifndef INTEL82559_PRIV_H
#define INTEL82559_PRIV_H

#include <hal.h>

/* The ring sizes should be a power of two for efficiency. */
#define TX_RING_SIZE	32		/* Effectively 2 entries fewer. */
#define RX_RING_SIZE	32
/* Actual number of TX packets queued, must be <= TX_RING_SIZE-2. */
#define TX_QUEUE_LIMIT  12
#define TX_QUEUE_UNFULL 8		/* Hysteresis marking queue as no longer full. */


/* The Speedo3 Rx and Tx buffer descriptors. */
struct RxFD {					/* Receive frame descriptor. */
	int32_t status;
	u_int32_t link;					/* struct RxFD * */
	u_int32_t rx_buf_addr;			/* void * */
	u_int32_t count;
};

/* Selected elements of the Tx/RxFD.status word. */
enum RxFD_bits {
	RxComplete=0x8000, RxOK=0x2000,
	RxErrCRC=0x0800, RxErrAlign=0x0400, RxErrTooBig=0x0200, RxErrSymbol=0x0010,
	RxEth2Type=0x0020, RxNoMatch=0x0004, RxNoIAMatch=0x0002,
	TxUnderrun=0x1000,  StatusComplete=0x8000,
};

struct TxFD {					/* Transmit frame descriptor set. */
	int32_t status;
	u_int32_t link;					/* void * */
	u_int32_t tx_desc_addr;			/* Always points to the tx_buf_addr element. */
	int32_t count;					/* # of TBD (=1), Tx start thresh., etc. */
	/* This constitutes two "TBD" entries. Non-zero-copy uses only one. */
	u_int32_t tx_buf_addr0;			/* void *, frame to be transmitted.  */
	int32_t tx_buf_size0;			/* Length of Tx frame. */
	u_int32_t tx_buf_addr1;			/* Used only for zero-copy data section. */
	int32_t tx_buf_size1;			/* Length of second data buffer (0). */
};

/* Elements of the dump_statistics block. This block must be lword aligned. */
struct intel82559_stats {
	u_int32_t tx_good_frames;
	u_int32_t tx_coll16_errs;
	u_int32_t tx_late_colls;
	u_int32_t tx_underruns;
	u_int32_t tx_lost_carrier;
	u_int32_t tx_deferred;
	u_int32_t tx_one_colls;
	u_int32_t tx_multi_colls;
	u_int32_t tx_total_colls;
	u_int32_t rx_good_frames;
	u_int32_t rx_crc_errs;
	u_int32_t rx_align_errs;
	u_int32_t rx_resource_errs;
	u_int32_t rx_overrun_errs;
	u_int32_t rx_colls_errs;
	u_int32_t rx_runt_errs;
	u_int32_t done_marker;
};

/* The Speedo3 Rx and Tx frame/buffer descriptors. */
struct descriptor {			/* A generic descriptor. */
	int32_t		cmd_status;			/* All command and status fields. */
	u_int32_t 	link;					/* struct descriptor *  */
	unsigned char 	params[0];
};




typedef struct intel82559 {


    struct TxFD		tx_ring[TX_RING_SIZE];	/* Commands (usually CmdTxPacket). */
    struct RxFD *	rx_ringp[RX_RING_SIZE];	/* Rx descriptor, used as ring. */
    struct intel82559_stats lstats;			/* Statistics and self-test region */

    /* The addresses of a Tx/Rx-in-place packets/buffers. */
    //struct sk_buff* 	tx_skbuff[TX_RING_SIZE];
    //struct sk_buff* 	rx_skbuff[RX_RING_SIZE];

    /* Transmit and other commands control. */
    struct descriptor  *last_cmd;	/* Last command sent. */
    unsigned int 	cur_tx, dirty_tx;	/* The ring entries to be free()ed. */

    //spinlock_t lock;				/* Group with Tx control cache line. */
    hal_spinlock_t      lock;

    u_int32_t 		tx_threshold;					/* The value for txdesc.count. */
    unsigned long 	last_cmd_time;

    /* Rx control, one cache line. */
    struct RxFD *	last_rxf;				/* Most recent Rx frame. */
    unsigned int 	cur_rx, dirty_rx;		/* The next free ring entry */
    unsigned int 	rx_buf_sz;				/* Based on MTU+slack. */
    long 		last_rx_time;			/* Last Rx, in jiffies, to handle Rx hang. */
    //int 		rx_copybreak;

    int 		msg_level;
    int 		max_interrupt_work;

    //void *		priv_addr;					/* Unaligned address for kfree */

    //struct net_device_stats stats;

    int 		alloc_failures;
    //int 		chip_id;
    int 		drv_flags;

    //struct pci_dev *pci_dev;

    //unsigned char 	acpi_pwr;
    //struct timer_list timer;	/* Media selection timer. */

    /* Multicast filter command. */
    int 		mc_setup_frm_len;			 	/* The length of an allocated.. */
    struct descriptor *	mc_setup_frm; 	/* ..multicast setup frame. */
    int 		mc_setup_busy;					/* Avoid double-use of setup frame. */
    int 		multicast_filter_limit;

    int 		in_interrupt;					/* Word-aligned dev->interrupt */
    int 		rx_mode;						/* Current PROMISC/ALLMULTI setting. */

    unsigned int 	tx_full:1;				/* The Tx queue is full. */
    unsigned int 	full_duplex:1;			/* Full-duplex operation requested. */
    unsigned int 	flow_ctrl:1;			/* Use 802.3x flow control. */
    unsigned int 	rx_bug:1;				/* Work around receiver hang errata. */
    unsigned int 	rx_bug10:1;			/* Receiver might hang at 10mbps. */
    unsigned int 	rx_bug100:1;			/* Receiver might hang at 100mbps. */
    unsigned int 	polling:1;				/* Hardware blocked interrupt line. */
    unsigned int 	medialock:1;			/* The media speed/duplex is fixed. */
    unsigned char 	default_port;			/* Last dev->if_port value. */
    unsigned short 	phy[2];				/* PHY media interfaces available. */
    unsigned short 	advertising;			/* Current PHY advertised caps. */
    unsigned short 	partner;				/* Link partner caps. */

    long 		last_reset;


    u_int8_t 		mac_addr[6];
    int 		irq;
    int                 io_port;
    physaddr_t          phys_base;
    int                 phys_size;

} intel82559;



static intel82559 *intel82559_new(void);
static int intel82559_init(intel82559 *nic, int card_idx);

static int intel82559_start(intel82559 *nic);
static int intel82559_stop(intel82559 *nic);

static int intel82559_read(struct phantom_device *dev, void *buf, int len);
static int intel82559_write(struct phantom_device *dev, const void *buf, int len);
static int intel82559_get_address(struct phantom_device *dev, void *buf, int len);























enum chip_capability_flags { ResetMII=1, HasChksum=2};


enum SCBCmdBits {
	SCBMaskCmdDone=0x8000, SCBMaskRxDone=0x4000, SCBMaskCmdIdle=0x2000,
	SCBMaskRxSuspend=0x1000, SCBMaskEarlyRx=0x0800, SCBMaskFlowCtl=0x0400,
	SCBTriggerIntr=0x0200, SCBMaskAll=0x0100,
	/* The rest are Rx and Tx commands. */
	CUStart=0x0010, CUResume=0x0020, CUHiPriStart=0x0030, CUStatsAddr=0x0040,
	CUShowStats=0x0050,
	CUCmdBase=0x0060,  /* CU Base address (set to zero) . */
	CUDumpStats=0x0070, /* Dump then reset stats counters. */
	CUHiPriResume=0x00b0, /* Resume for the high priority Tx queue. */
	RxStart=0x0001, RxResume=0x0002, RxAbort=0x0004, RxAddrLoad=0x0006,
	RxResumeNoResources=0x0007,
};

enum intr_status_bits {
	IntrCmdDone=0x8000,  IntrRxDone=0x4000, IntrCmdIdle=0x2000,
	IntrRxSuspend=0x1000, IntrMIIDone=0x0800, IntrDrvrIntr=0x0400,
	IntrAllNormal=0xfc00,
};

enum SCBPort_cmds {
	PortReset=0, PortSelfTest=1, PortPartialReset=2, PortDump=3,
};



/* Our internal RxMode state, not tied to the hardware bits. */
enum rx_mode_bits {
	AcceptAllMulticast=0x01, AcceptAllPhys=0x02, 
	AcceptErr=0x80, AcceptRunt=0x10,
	AcceptBroadcast=0x08, AcceptMulticast=0x04,
	AcceptMyPhys=0x01, RxInvalidMode=0x7f
};







/* Offsets to the various registers.
   All accesses need not be longword aligned. */
enum speedo_offsets {
	SCBStatus = 0, SCBCmd = 2,	/* Rx/Command Unit command and status. */
	SCBPointer = 4,				/* General purpose pointer. */
	SCBPort = 8,				/* Misc. commands and operands.  */
	SCBflash = 12, SCBeeprom = 14, /* EEPROM and flash memory control. */
	SCBCtrlMDI = 16,			/* MDI interface control. */
	SCBEarlyRx = 20,			/* Early receive byte count. */
};
/* Commands that can be put in a command list entry. */
enum commands {
	CmdNOp = 0, CmdIASetup = 0x10000, CmdConfigure = 0x20000,
	CmdMulticastList = 0x30000, CmdTx = 0x40000, CmdTDR = 0x50000,
	CmdDump = 0x60000, CmdDiagnose = 0x70000,
	CmdSuspend = 0x40000000,	/* Suspend after completion. */
	CmdIntr = 0x20000000,		/* Interrupt after completion. */
	CmdTxFlex = 0x00080000,		/* Use "Flexible mode" for CmdTx command. */
};






/* Standard serial configuration EEPROM commands. */
#define EE_READ_CMD		(6)

/* Serial EEPROM section.
   A "bit" grungy, but we work our way through bit-by-bit :->. */
/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK	0x01	/* EEPROM shift clock. */
#define EE_CS			0x02	/* EEPROM chip select. */
#define EE_DATA_WRITE	0x04	/* EEPROM chip data in. */
#define EE_DATA_READ	0x08	/* EEPROM chip data out. */
#define EE_ENB			(0x4800 | EE_CS)
#define EE_WRITE_0		0x4802
#define EE_WRITE_1		0x4806
#define EE_OFFSET		SCBeeprom


static int do_eeprom_cmd(long ioaddr, int cmd, int cmd_len);

//static int mdio_read(struct net_device *dev, int phy_id, int location);
//static int mdio_write(long ioaddr, int phy_id, int location, int value);


#endif // INTEL82559_PRIV_H

