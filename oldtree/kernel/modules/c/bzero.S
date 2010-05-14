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

#include <mach/machine/asm.h>

/*
 * memset(char *addr, int ch, unsigned int length)
 */
ENTRY(memset)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	movl	B_ARG0,%edi
	movb	B_ARG1,%al
	movb	%al,%ah
	movl	%eax,%edx
	shll	$16,%eax
	movw	%dx,%ax
	movl	B_ARG2,%edx
bzero_common:
	cld
/* zero longs */
	movl	%edx,%ecx
	shrl	$2,%ecx
	rep
	stosl
/* zero bytes */
	movl	%edx,%ecx
	andl	$3,%ecx
	rep
	stosb
/* return pointer to dest */
	movl	B_ARG0,%eax
	popl	%edi
	leave
	ret

/*
 * bzero(char * addr, unsigned int length)
 */
ENTRY(blkclr)
ENTRY(bzero)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	movl	B_ARG0,%edi
	movl	B_ARG1,%edx
	xorl	%eax,%eax
	jmp	bzero_common

