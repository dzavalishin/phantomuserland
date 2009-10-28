/* 
 * Mach Operating System
 * Copyright (c) 1993 Carnegie Mellon University
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
 * Setjmp/longjmp buffer for i386.
 */
#ifndef _MACH_SETJMP_H_PROCESSED_
#define _MACH_SETJMP_H_PROCESSED_ 1

/* XXX prefix these with mach_ so they don't interfere with higher layers?
   This stuff is included by cthreads.h.  */

/* XXX The original definition of jmp_buf[] causes problems using
 * libthreads when linked against NetBSD and FreeBSD's libc because
 * it's too small.  When cthreads calls _setjmp, it gets the libc
 * version which saves more state than it's expecting and overwrites
 * important cthread data. =( This definition is big enough for all
 * known systems so far (Linux's is 6, FreeBSD's is 9 and NetBSD's is
 * 10).  This file really shouldn't even be here, since we should be
 * using libc's setjmp.h.  
 */
#if 0
#define _JBLEN 6
#else
#define _JBLEN 10
#endif

typedef int jmp_buf[_JBLEN];		/* ebx, esi, edi, ebp, esp, eip */


extern int setjmp (jmp_buf);
extern void longjmp (jmp_buf, int);
extern int _setjmp (jmp_buf);
extern void _longjmp (jmp_buf, int);

#endif /* _MACH_SETJMP_H_PROCESSED_ */
