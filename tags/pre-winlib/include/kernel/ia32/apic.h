/*
 * Copyright (c) 1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *      Author: Bryan Ford, University of Utah CSL
 */
#ifndef _PHANTOM_APIC_
#define _PHANTOM_APIC_

#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif


#define APIC_BASE_MSR 0x1B
void relocate_apic(void);
void phantom_setup_apic(void);


typedef struct ApicReg
{
	unsigned r;	/* the actual register */
	unsigned p[3];	/* pad to the next 128-bit boundary */
} ApicReg;

typedef struct ApicIoUnit
{
	ApicReg select;
	ApicReg window;
} ApicIoUnit;
#define APIC_IO_UNIT_ID			0x00
#define APIC_IO_VERSION			0x01
#define APIC_IO_REDIR_LOW(int_pin)	(0x10+(int_pin)*2)
#define APIC_IO_REDIR_HIGH(int_pin)	(0x11+(int_pin)*2)

typedef struct ApicLocalUnit
{
	ApicReg reserved0;
	ApicReg reserved1;
        ApicReg 		unit_id;                //
	ApicReg 		version;
	ApicReg reserved4;
	ApicReg reserved5;
	ApicReg reserved6;
	ApicReg reserved7;
	ApicReg 		task_pri;               // TPRI
	ApicReg reservedb; // arb pri
	ApicReg reservedc; // cpu pri
        ApicReg 		eoi;                    // EOI
	ApicReg 		remote;
	ApicReg 		logical_dest;
	ApicReg 		dest_format;
	ApicReg 		spurious_vector;        // SIVR
	ApicReg 		isr[8];
	ApicReg 		tmr[8];
        ApicReg 		irr[8];
        ApicReg                 error_status;
	ApicReg reserved28[7];
	ApicReg			int_command[2]; 	// ICR1. ICR2
	ApicReg 		timer_vector; 		// LVTT
	ApicReg reserved33;
	ApicReg reserved34; // perf count lvt
	ApicReg 		lint0_vector;
	ApicReg 		lint1_vector;
	ApicReg 		error_vector; // err vector
	ApicReg 		init_count; // timer
	ApicReg 		cur_count;  // timer
	ApicReg reserved3a;
	ApicReg reserved3b;
	ApicReg reserved3c;
	ApicReg reserved3d;
	ApicReg 		divider_config; // 3e, timer divider
	ApicReg reserved3f;
} ApicLocalUnit;


/* Address at which the local unit is mapped in kernel virtual memory.
   Must be constant.  */
//#define APIC_LOCAL_VA	0xc1000000

//#define apic_local_unit (*((volatile ApicLocalUnit*)APIC_LOCAL_VA))

extern volatile ApicLocalUnit * apic_local_unit;

/* Set or clear a bit in a 255-bit APIC mask register.
   These registers are spread through eight 32-bit registers.  */
#define APIC_SET_MASK_BIT(reg, bit) \
	((reg)[(bit) >> 5].r |= 1 << ((bit) & 0x1f))
#define APIC_CLEAR_MASK_BIT(reg, bit) \
	((reg)[(bit) >> 5].r &= ~(1 << ((bit) & 0x1f)))
















#define APIC_ICR1_WRITE_MASK    0xfff3f000
#define APIC_ICR1_DELMODE_FIXED 0
#define APIC_ICR1_DELMODE_LOWESTPRI (1 << 8)
#define APIC_ICR1_DESTMODE_LOG (1 << 11)
#define APIC_ICR1_DESTMODE_PHYS 0

#define APIC_ICR1_READ_MASK     0xfff32000
#define APIC_ICR1_DELSTATUS (1 << 12)

#define APIC_ICR1_DEST_FIELD          (0)
#define APIC_ICR1_DEST_SELF           (1 << 18)
#define APIC_ICR1_DEST_ALL            (2 << 18)
#define APIC_ICR1_DEST_ALL_BUT_SELF   (3 << 18)



#define APIC_ICR2_MASK     0x00ffffff

#define APIC_TDCR_2        0x00
#define APIC_TDCR_4        0x01
#define APIC_TDCR_8        0x02
#define APIC_TDCR_16       0x03
#define APIC_TDCR_32       0x08
#define APIC_TDCR_64       0x09
#define APIC_TDCR_128      0x0a
#define APIC_TDCR_1        0x0b

#define APIC_LVTT_MASK     0x000310ff
#define APIC_LVTT_VECTOR   0x000000ff
#define APIC_LVTT_DS       0x00001000
#define APIC_LVTT_M        0x00010000
#define APIC_LVTT_TM       0x00020000

#define APIC_LVT_DM        0x00000700
#define APIC_LVT_IIPP      0x00002000
#define APIC_LVT_TM        0x00008000
#define APIC_LVT_M         0x00010000
#define APIC_LVT_OS        0x00020000

#define APIC_TPR_PRIO      0x000000ff
#define APIC_TPR_INT       0x000000f0
#define APIC_TPR_SUB       0x0000000f

#define APIC_SVR_SWEN      0x00000100
#define APIC_SVR_FOCUS     0x00000200

#define APIC_DEST_STARTUP  0x00600

#define LOPRIO_LEVEL       0x00000010

//#define IOAPIC_ID          0x0
//#define IOAPIC_VERSION     0x1
//#define IOAPIC_ARB         0x2
//#define IOAPIC_REDIR_TABLE 0x10

#define IPI_CACHE_FLUSH    0x40
#define IPI_INV_TLB        0x41
#define IPI_INV_PTE        0x42
#define IPI_INV_RESCHED    0x43
#define IPI_STOP           0x44




















#define MP_EXT_PE          0
#define MP_EXT_BUS         1
#define MP_EXT_IO_APIC     2
#define MP_EXT_IO_INT      3
#define MP_EXT_LOCAL_INT   4

#define MP_EXT_PE_LEN          20
#define MP_EXT_BUS_LEN         8
#define MP_EXT_IO_APIC_LEN     8
#define MP_EXT_IO_INT_LEN      8
#define MP_EXT_LOCAL_INT_LEN   8

struct mp_config_table {
	unsigned int signature;       /* "PCMP" */
	unsigned short table_len;     /* length of this structure */
	unsigned char mp_rev;         /* spec supported, 1 for 1.1 or 4 for 1.4 */
	unsigned char checksum;       /* checksum, all bytes add up to zero */
	char oem[8];                  /* oem identification, not null-terminated */
	char product[12];             /* product name, not null-terminated */
	void *oem_table_ptr;          /* addr of oem-defined table, zero if none */
	unsigned short oem_len;       /* length of oem table */
	unsigned short num_entries;   /* number of entries in base table */
	unsigned int apic;            /* address of apic */
	unsigned short ext_len;       /* length of extended section */
	unsigned char ext_checksum;   /* checksum of extended table entries */
};

struct mp_flt_struct {
	unsigned int signature;       /* "_MP_" */
	struct mp_config_table *mpc;  /* address of mp configuration table */
	unsigned char mpc_len;        /* length of this structure in 16-byte units */
	unsigned char mp_rev;         /* spec supported, 1 for 1.1 or 4 for 1.4 */
	unsigned char checksum;       /* checksum, all bytes add up to zero */
	unsigned char mp_feature_1;   /* mp system configuration type if no mpc */
	unsigned char mp_feature_2;   /* imcrp */
	unsigned char mp_feature_3, mp_feature_4, mp_feature_5; /* reserved */
};

struct mp_ext_pe
{
	unsigned char type;
	unsigned char apic_id;
	unsigned char apic_version;
	unsigned char cpu_flags;
	unsigned int signature;       /* stepping, model, family, each four bits */
	unsigned int feature_flags;
	unsigned int res1, res2;
};

struct mp_ext_ioapic
{
	unsigned char type;
	unsigned char ioapic_id;
	unsigned char ioapic_version;
	unsigned char ioapic_flags;
	unsigned int *addr;
};

void phantom_io_apic_init( physaddr_t pa );

u_int32_t readIoApic(u_int32_t reg);
void writeIoApic( u_int32_t reg, u_int32_t value);

#define IOAPIC_LEVEL_TRIGGERED 1
#define IOAPIC_EDGE_TRIGGERED  0

#define IOAPIC_LO_ACTIVE 1
#define IOAPIC_HI_ACTIVE 0

void setIoApicInput( int input, int vector, int level, int low_active );
void setIoApicId( int id );

void dumpIoApicInputs(void);



void phantom_smp_send_broadcast_ici(void);



#endif //_PHANTOM_APIC_

