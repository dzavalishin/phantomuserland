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

#ifndef _I386AT_IDT_
#define _I386AT_IDT_

#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif


/*
 * Interrupt table must always be at least 32 entries long,
 * to cover the basic i386 exception vectors.
 * More-specific code will probably define it to be longer,
 * to allow separate entrypoints for hardware interrupts.
 */

#define IDTSZ 256

#define PIC_INT_BASE 0x20
// Kolibri syscall entry point
#define KOLIBRI_INT 0x40
#define APIC_INT_BASE 0x60
#define LINUX_INT 0x80




#define set_idt(pseudo_desc) \
    ({ \
	asm volatile("lidt %0" : : "m" ((pseudo_desc)->limit)); \
    })


#include <ia32/seg.h>


extern struct real_gate idt[IDTSZ];

/* Fill a gate in the IDT.  */
#define fill_idt_gate(int_num, entry, selector, access, dword_count) \
	fill_gate(&idt[int_num], entry, selector, access, dword_count)




#endif // _I386AT_IDT_
