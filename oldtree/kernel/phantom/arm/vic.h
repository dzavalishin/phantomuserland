/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * arm VIC - PL192 (and, maybe, 190)
 *
**/


#define VIC_REG_IRQ_STATUS	0x000
#define VIC_REG_FIQ_STATUS	0x004
#define VIC_REG_RAWINTR         0x008
#define VIC_REG_INTSELECT       0x00C
#define VIC_REG_INTENABLE       0x010
#define VIC_REG_INTENCLEAR      0x014
#define VIC_REG_SOFTINT         0x018
#define VIC_REG_SOFTINTCLEAR    0x01C
#define VIC_REG_PROTECTION      0x020
#define VIC_REG_WPRIOMASK       0x024
#define VIC_REG_PRIORITYDAISY	0x028

// 32 times, step = 4 bytes
#define VIC_REG_VECTADDR0	0x100

// 32 times, step = 4 bytes
#define VIC_REG_VECTPRIORITY0   0x200

// begin/end of interrupt handling
#define VIC_REG_VECTOR_ADDR     0xF00

#define VIC_REG_HID0    	0xFE0
#define VIC_REG_HID1 		0xFE4
#define VIC_REG_HID2 		0xFE8
#define VIC_REG_HID3 		0xFEC

#define VIC_REG_PCELLID0     	0xFF0
#define VIC_REG_PCELLID1     	0xFF4
#define VIC_REG_PCELLID2     	0xFF8
#define VIC_REG_PCELLID3     	0xFFC

// Undocumented, got from Linux drv
#define VIC_ITCR			0x300	/* VIC test control register */

// 190 support
//#define VIC_PL190_VECT_ADDR		0x30	/* PL190 only */
//#define VIC_PL190_DEF_VECT_ADDR		0x34	/* PL190 only */

