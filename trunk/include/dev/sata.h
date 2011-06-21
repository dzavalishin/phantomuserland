/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Serial ATA defs.
 *
**/

#ifndef SATA_H
#define SATA_H

#include <sys/types.h>

// There are four kinds of SATA devices, and their signatures are defined
// as below. The Port Signature register contains the device signature,
// just read this register to find which kind of device is attached at the port.

#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	0x96690101	// Port multiplier
 

typedef enum
{
	FIS_TYPE_REG_H2D	= 0x27,	// Register FIS - host to device
	FIS_TYPE_REG_D2H	= 0x34,	// Register FIS - device to host
	FIS_TYPE_DMA_ACT	= 0x39,	// DMA activate FIS - device to host
	FIS_TYPE_DMA_SETUP	= 0x41,	// DMA setup FIS - bidirectional
	FIS_TYPE_DATA		= 0x46,	// Data FIS - bidirectional
	FIS_TYPE_BIST		= 0x58,	// BIST activate FIS - bidirectional
	FIS_TYPE_PIO_SETUP	= 0x5F,	// PIO setup FIS - device to host
	FIS_TYPE_DEV_BITS	= 0xA1,	// Set device bits FIS - device to host
} fis_type_t;


// A host to device register FIS is used by the host to send command 
// or control to a device. As illustrated in the following data structure, 
// it contains the IDE registers such as command, LBA, device, feature, 
// count and control. An ATA command is constructed in this structure 
// and issued to the device. All reserved fields in an FIS should be 
// cleared to zero. 

struct tagFIS_REG_H2D
{
	// DWORD 0
	u_int8_t	fis_type;	// FIS_TYPE_REG_H2D
 
	u_int8_t	pmport:4;	// Port multiplier
	u_int8_t	rsv0:3;		// Reserved
	u_int8_t	c:1;		// 1: Command, 0: Control
 
	u_int8_t	command;	// Command register
	u_int8_t	featurel;	// Feature register, 7:0
 
	// DWORD 1
	u_int8_t	lba0;		// LBA low register, 7:0
	u_int8_t	lba1;		// LBA mid register, 15:8
	u_int8_t	lba2;		// LBA high register, 23:16
	u_int8_t	device;		// Device register
 
	// DWORD 2
	u_int8_t	lba3;		// LBA register, 31:24
	u_int8_t	lba4;		// LBA register, 39:32
	u_int8_t	lba5;		// LBA register, 47:40
	u_int8_t	featureh;	// Feature register, 15:8
 
	// DWORD 3
	u_int8_t	countl;		// Count register, 7:0
	u_int8_t	counth;		// Count register, 15:8
	u_int8_t	icc;		// Isochronous command completion
	u_int8_t	control;	// Control register
 
	// DWORD 4
	u_int8_t	rsv1[4];	// Reserved
} __attribute__((packed));

typedef struct tagFIS_REG_H2D fis_reg_h2d_t;




// A device to host register FIS is used by the device to notify the 
// host that some ATA register has changed. It contains the updated 
// task files such as status, error and other registers.

struct tagFIS_REG_D2H
{
	// DWORD 0
	u_int8_t	fis_type;    // FIS_TYPE_REG_D2H
 
	u_int8_t	pmport:4;    // Port multiplier
	u_int8_t	rsv0:2;      // Reserved
	u_int8_t	i:1;         // Interrupt bit
	u_int8_t	rsv1:1;      // Reserved
 
	u_int8_t	status;      // Status register
	u_int8_t	error;       // Error register
 
	// DWORD 1
	u_int8_t	lba0;        // LBA low register, 7:0
	u_int8_t	lba1;        // LBA mid register, 15:8
	u_int8_t	lba2;        // LBA high register, 23:16
	u_int8_t	device;      // Device register
 
	// DWORD 2
	u_int8_t	lba3;        // LBA register, 31:24
	u_int8_t	lba4;        // LBA register, 39:32
	u_int8_t	lba5;        // LBA register, 47:40
	u_int8_t	rsv2;        // Reserved
 
	// DWORD 3
	u_int8_t	countl;      // Count register, 7:0
	u_int8_t	counth;      // Count register, 15:8
	u_int8_t	rsv3[2];     // Reserved
 
	// DWORD 4
	u_int8_t	rsv4[4];     // Reserved
} __attribute__((packed));

typedef struct tagFIS_REG_D2H fis_reg_d2h_t;


#if 0

// this FIS is used by the host or device to send data payload. 
// The data size can be varied.

typedef struct tagFIS_DATA
{
	// DWORD 0
	u_int8_t	fis_type;	// FIS_TYPE_DATA
 
	u_int8_t	pmport:4;	// Port multiplier
	u_int8_t	rsv0:4;		// Reserved
 
	u_int8_t	rsv1[2];	// Reserved
 
	// DWORD 1 ~ N
	u_int32_t	data[1];	// Payload
} FIS_DATA;



// This FIS is used by the device to tell the host that it’s about to send 
// or ready to receive a PIO data payload.

typedef struct tagFIS_PIO_SETUP
{
	// DWORD 0
	u_int8_t	fis_type;	// FIS_TYPE_PIO_SETUP
 
	u_int8_t	pmport:4;	// Port multiplier
	u_int8_t	rsv0:1;		// Reserved
	u_int8_t	d:1;		// Data transfer direction, 1 - device to host
	u_int8_t	i:1;		// Interrupt bit
	u_int8_t	rsv1:1;
 
	u_int8_t	status;		// Status register
	u_int8_t	error;		// Error register
 
	// DWORD 1
	u_int8_t	lba0;		// LBA low register, 7:0
	u_int8_t	lba1;		// LBA mid register, 15:8
	u_int8_t	lba2;		// LBA high register, 23:16
	u_int8_t	device;		// Device register
 
	// DWORD 2
	u_int8_t	lba3;		// LBA register, 31:24
	u_int8_t	lba4;		// LBA register, 39:32
	u_int8_t	lba5;		// LBA register, 47:40
	u_int8_t	rsv2;		// Reserved
 
	// DWORD 3
	u_int8_t	countl;		// Count register, 7:0
	u_int8_t	counth;		// Count register, 15:8
	u_int8_t	rsv3;		// Reserved
	u_int8_t	e_status;	// New value of status register
 
	// DWORD 4
	u_it16_t	tc;		// Transfer count
	u_int8_t	rsv4[2];	// Reserved
} FIS_PIO_SETUP;

#endif




#endif // SATA_H


