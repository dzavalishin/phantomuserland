/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log:	tss.h,v $
 * Revision 2.6  91/05/14  16:18:40  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/05/08  12:43:45  dbg
 * 	Protect against multiple includes.
 * 	[91/04/26  14:39:49  dbg]
 * 
 * Revision 2.4  91/02/05  17:15:37  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:38:56  mrt]
 * 
 * Revision 2.3  91/01/08  17:32:27  rpd
 * 	Add bit_map
 * 	[90/12/20  10:21:17  rvb]
 * 
 * Revision 2.2  90/05/03  15:38:14  dbg
 * 	Created.
 * 	[90/02/08            dbg]
 * 
 */

#ifndef	_I386_TSS_H_
#define	_I386_TSS_H_

#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif

/*
 *	i386 Task State Segment
 */
struct i386_tss {
	int		back_link;	/* segment number of previous task,
					   if nested */
	int		esp0;		/* initial stack pointer ... */
	int		ss0;		/* and segment for ring 0 */
	int		esp1;		/* initial stack pointer ... */
	int		ss1;		/* and segment for ring 1 */
	int		esp2;		/* initial stack pointer ... */
	int		ss2;		/* and segment for ring 2 */
	int		cr3;		/* CR3 - page table directory
						 physical address */
	int		eip;
	int		eflags;
	int		eax;
	int		ecx;
	int		edx;
	int		ebx;
	int		esp;		/* current stack pointer */
	int		ebp;
	int		esi;
	int		edi;
	int		es;
	int		cs;
	int		ss;		/* current stack segment */
	int		ds;
	int		fs;
	int		gs;
	int		ldt;		/* local descriptor table segment */
	unsigned short	trace_trap;	/* trap on switch to this task */
	unsigned short	io_bit_map_offset;
					/* offset to start of IO permission
					   bit map */
};

#endif	/* _I386_TSS_H_ */
