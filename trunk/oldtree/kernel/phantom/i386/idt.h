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


/*
 * Interrupt table must always be at least 32 entries long,
 * to cover the basic i386 exception vectors.
 * More-specific code will probably define it to be longer,
 * to allow separate entrypoints for hardware interrupts.
 */

/* On a standard PC, we only need 16 interrupt vectors,
   because that's all the PIC hardware supports.  */
/* XX But for some reason we program the PIC
   to use vectors 0x40-0x4f rather than 0x20-0x2f.  Fix.  */
//#define IDTSZ (0x20+0x20+0x10)
#define IDTSZ 256

#define PIC_INT_BASE 0x20
#define APIC_INT_BASE 0x40


//#include_next "idt.h"


#define set_idt(pseudo_desc) \
    ({ \
	asm volatile("lidt %0" : : "m" ((pseudo_desc)->limit)); \
    })




//#include <mach/vm_param.h>

#include <i386/seg.h>


extern struct real_gate idt[IDTSZ];

/* Fill a gate in the IDT.  */
#define fill_idt_gate(int_num, entry, selector, access, dword_count) \
	fill_gate(&idt[int_num], entry, selector, access, dword_count)


/*
// Fill a gate in a CPU's IDT.
#define fill_idt_gate(cpu, int_num, entry, selector, access) \
	fill_gate(&(cpu)->tables.idt[int_num], entry, selector, access, 0)

 */



#endif // _I386AT_IDT_
