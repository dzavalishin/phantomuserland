/* 
 * Mach Operating System
 */
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)SYS.h	5.4 (Berkeley) 6/27/88
 */

#include <mach/asm.h>
#include <kernel/syscalls.h>


#ifdef __STDC__

//#define	SYSCALL(x)	ENTRY(x); movl	$(SYS_ ## x), %eax; SVC; jb LCL(cerror)
#define	SYSCALL(x)	ENTRY(x); movl	$(SYS_ ## x), %eax; SVC; ret
#define	PSEUDO(x,y)	ENTRY(x); movl	$(SYS_ ## y), %eax; SVC; ret

#else // __STDC__

//#define	SYSCALL(x)	ENTRY(x); movl	$SYS_/**/x, %eax; SVC; jb LCL(cerror)
#define	SYSCALL(x)	ENTRY(x); movl	$SYS_/**/x, %eax; SVC; ret
#define	PSEUDO(x,y)	ENTRY(x); movl	$SYS_/**/y, %eax; SVC; ret

#endif // __STDC__

//#define	CALL(x,y)	calls $x, EXT(y)

//.data
//	.globl	LCL(cerror)
