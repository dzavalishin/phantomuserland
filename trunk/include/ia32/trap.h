/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 * $Log:	trap.h,v $
 * Revision 2.5  91/05/14  16:18:26  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:15:33  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:38:51  mrt]
 * 
 * Revision 2.3  90/10/25  14:45:03  rwd
 * 	Added watchpoint support.
 * 	[90/10/18            rpd]
 * 
 * Revision 2.2  90/05/03  15:38:10  dbg
 * 	Created.
 * 	[90/02/08            dbg]
 * 
 */

#ifndef	_I386_TRAP_H_
#define	_I386_TRAP_H_

#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif

#ifndef GENERAL_TRAP_H
#warning include <kernel/trap.h> instead!
#endif


/*
 * Hardware trap vectors for i386.
 */
#define	T_DIVIDE_ERROR		0
#define	T_DEBUG			1
#define	T_NMI			2		/* non-maskable interrupt */
#define	T_INT3			3		/* int 3 instruction */
#define	T_OVERFLOW		4		/* overflow test */
#define	T_OUT_OF_BOUNDS		5		/* bounds check */
#define	T_INVALID_OPCODE	6		/* invalid op code */
#define	T_NO_FPU		7		/* no floating point */
#define	T_DOUBLE_FAULT		8		/* double fault */
#define	T_FPU_FAULT		9
#define T_INVALID_TSS		10
#define	T_SEGMENT_NOT_PRESENT	11
#define	T_STACK_FAULT		12
#define	T_GENERAL_PROTECTION	13
#define	T_PAGE_FAULT		14
/* Pentium Pro generates this due to a bug.  See the eratta sheet.	15 */
#define	T_FLOATING_POINT_ERROR	16
#define	T_WATCHPOINT		17
#define	T_ALIGNMENT_CHECK	17
#define	T_MACHINE_CHECK		18

/*
 * Page-fault trap codes.
 */
#define	T_PF_PROT		0x1		/* protection violation */
#define	T_PF_WRITE		0x2		/* write access */
#define	T_PF_USER		0x4		/* from user state */






#ifndef ASSEMBLER


/* This structure corresponds to the state of user registers
   as saved upon kernel trap/interrupt entry.
   As always, it is only a default implementation;
   a well-optimized microkernel will probably want to override it
   with something that allows better optimization.  */

struct trap_state {

	/* Saved segment registers */
	unsigned int	gs;
	unsigned int	fs;
	unsigned int	es;
	unsigned int	ds;

	/* PUSHA register state frame */
	unsigned int	edi;
	unsigned int	esi;
	unsigned int	ebp;
	unsigned int	cr2;	/* we save cr2 over esp for page faults */
	unsigned int	ebx;
	unsigned int	edx;
	unsigned int	ecx;
	unsigned int	eax;

	unsigned int	trapno;
	unsigned int	err;

	/* Processor state frame */
	unsigned int	eip;
	unsigned int	cs;
	unsigned int	eflags;
	unsigned int	esp;
	unsigned int	ss;

	/* Virtual 8086 segment registers */
	unsigned int	v86_es;
	unsigned int	v86_ds;
	unsigned int	v86_fs;
	unsigned int	v86_gs;
};

#define TS_PROGRAM_COUNTER eip

/* The actual trap_state frame pushed by the processor
   varies in size depending on where the trap came from.  */
#define TR_KSIZE	((int)&((struct trap_state*)0)->esp)
#define TR_USIZE	((int)&((struct trap_state*)0)->v86_es)
#define TR_V86SIZE	sizeof(struct trap_state)

//#define I386_N_TRAPS 32
#define ARCH_N_TRAPS 32


//int (*phantom_trap_handlers[I386_N_TRAPS])(struct trap_state *ts);





#else // ASSEMBLER

#include <ia32/asm.h>

#define UNEXPECTED_TRAP				\
	movw	%ss,%ax				;\
	movw	%ax,%ds				;\
	movw	%ax,%es				;\
	movl	%esp,%eax			;\
	pushl	%eax				;\
	call	EXT(trap_dump_die)		;\


#endif // ASSEMBLER



#endif	// _I386_TRAP_H_

