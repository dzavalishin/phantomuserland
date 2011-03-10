/*
** Copyright 2006, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _RTL8169_DEV_H
#define _RTL8169_DEV_H

/* register defintions, etc go here */
#define REG_IDR0		0x00
#define REG_IDR1		0x01
#define REG_IDR2		0x02
#define REG_IDR3		0x03
#define REG_IDR4		0x04
#define REG_IDR5		0x05
#define REG_MAR0		0x08
#define REG_MAR1		0x09
#define REG_MAR2		0x0a
#define REG_MAR3		0x0b
#define REG_MAR4		0x0c
#define REG_MAR5		0x0d
#define REG_MAR6		0x0e
#define REG_MAR7		0x0f
#define REG_DTCCR		0x10
#define REG_TNPDS_LOW	0x20
#define REG_TNPDS_HIGH	0x24
#define REG_THPDS_LOW	0x28
#define REG_THPDS_HIGH	0x2c
#define REG_CR			0x37
#define REG_TPPOLL		0x38
#define REG_IMR			0x3c
#define REG_ISR			0x3e
#define REG_TCR			0x40
#define REG_RCR			0x44
#define REG_TCTR		0x48
#define REG_MPC			0x4c
#define REG_9346CR		0x50
#define REG_CONFIG0		0x51
#define REG_CONFIG1		0x52
#define REG_CONFIG2		0x53
#define REG_CONFIG3		0x54
#define REG_CONFIG4		0x55
#define REG_CONFIG5		0x56
#define REG_TIMERINT	0x58
#define REG_PHYAR		0x60
#define REG_TBISCR0		0x64
#define REG_TBI_ANAR	0x68
#define REG_TBI_LPAR	0x6a
#define REG_PHYSTATUS	0x6c
#define REG_WAKEUP0		0x84
#define REG_WAKEUP1		0x8c
#define REG_WAKEUP2LD	0x94
#define REG_WAKEUP2HD	0x9C
#define REG_WAKEUP3LD	0xa4
#define REG_WAKEUP3HD	0xaC
#define REG_WAKEUP4LD	0xb4
#define REG_WAKEUP4HD	0xbC
#define REG_CRC0		0xc4
#define REG_CRC1		0xc6
#define REG_CRC2		0xc8
#define REG_CRC3		0xca
#define REG_CRC4		0xcc
#define REG_RMS			0xda
#define REG_CCR			0xe0
#define REG_RDSAR_LOW	0xe4
#define REG_RDSAR_HIGH	0xe8
#define REG_MTPS		0xec

#define IMR_SYSERR  (1<<15)
#define IMR_TIMEOUT (1<<14)
#define IMR_SWINT   (1<<8)
#define IMR_TDU     (1<<7)
#define IMR_RXOVL   (1<<6)
#define IMR_LINKCHG (1<<5)
#define IMR_RDU     (1<<4)
#define IMR_TER     (1<<3)
#define IMR_TOK     (1<<2)
#define IMR_RER     (1<<1)
#define IMR_ROK     (1<<0)

struct rtl_tx_descriptor {
	u_int16_t frame_len;
	u_int16_t flags;
	u_int32_t vlan;
	u_int32_t tx_buffer_low;
	u_int32_t tx_buffer_high;
};

struct rtl_rx_descriptor {
	union {
		u_int16_t buffer_size; /* rx command */
		u_int16_t frame_len;   /* rx status - top 2 bits are checksum offload flags */
	};
	u_int16_t flags;
	u_int32_t vlan;
	u_int32_t rx_buffer_low;
	u_int32_t rx_buffer_high;
};

/* descriptor bits */
#define RTL_DESC_OWN (1<<15)
#define RTL_DESC_EOR (1<<14)
#define RTL_DESC_FS  (1<<13)
#define RTL_DESC_LS  (1<<12)

/* all of the descriptors are 16 bytes long */
#define DESCRIPTOR_LEN 16

#define NUM_TX_DESCRIPTORS 64
#define NUM_RX_DESCRIPTORS 256

#endif
