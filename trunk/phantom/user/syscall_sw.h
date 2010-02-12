/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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

#ifndef	_MACH_I386_SYSCALL_SW_H_
#define _MACH_I386_SYSCALL_SW_H_

#include <mach/asm.h>

#if BSD_TRAP
#define kernel_trap(trap_name,trap_number,number_args) \
ENTRY(trap_name) \
	movl	$ trap_number,%eax; \
	SVC; \
	jb LCL(cerror); \
	ret;
#else
#define kernel_trap(trap_name,trap_number,number_args) \
ENTRY(trap_name) \
	movl	$ trap_number,%eax; \
	SVC; \
	ret;
#endif

#endif	/* _MACH_I386_SYSCALL_SW_H_ */
