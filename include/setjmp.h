/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Setjmp buffer definition. Arch dep.
 *
 *
**/

// Sorry
//#ifdef ARCH_e2k
//// We do not have and use setjmp on Elbrus
//#else

#ifndef _MACH_SETJMP_H_PROCESSED_
#define _MACH_SETJMP_H_PROCESSED_ 1

#include <sys/cdefs.h>

#ifdef ARCH_ia32
/*
 * Setjmp/longjmp buffer for i386.
 */
#if 0
#define _JBLEN 6
#else
//#define _JBLEN 10
#define _JBLEN (13 * 4) // in user mode we use GCC code, here it's buf size
#endif
#endif


#ifdef ARCH_arm
// TODO float!
// In fact we store r4-r14 = 11 i + 12+1f
#define _JBLEN 96
#endif

#ifdef ARCH_amd64
#define	_JBLEN	12		// Size of the jmp_buf on AMD64. 
#endif

#ifdef ARCH_mips
#define	_JBLEN	64*8		// Size of the jmp_buf on MIPS - 64 regs 64 bits each
#endif

#ifdef ARCH_e2k
#define	_JBLEN	64*8+10		// Size of the jmp_buf on e2k - wrong, don't know yet FIXME BUG
#endif


#ifndef _JBLEN
# error setjmp arch
#endif

typedef int jmp_buf[_JBLEN];



// Wrapper - define below
//extern int setjmp (jmp_buf) __attribute__((returns_twice));
//extern void longjmp (jmp_buf, int) __dead2;


// Machine dependent implementation
extern int setjmp_machdep (jmp_buf) __attribute__((returns_twice));
extern void longjmp_machdep (jmp_buf, int) __dead2;


#ifdef KERNEL

#define setjmp(___j) \
({ \
    int tid = get_current_tid(); \
    int rv = setjmp_machdep(___j); \
\
    int new_tid = get_current_tid(); \
    if( tid != new_tid ) \
        panic("Cross-thread longjmp, saved state in tid %d, jump from tid %d", tid, new_tid ); \
\
    rv; \
})
#else
#define setjmp(___j) setjmp_machdep(___j)
#endif


#define longjmp(___j, ___v) longjmp_machdep(___j,___v)




#endif /* _MACH_SETJMP_H_PROCESSED_ */


// Sorry
//#endif // ARCH_e2k


/*
 * Original i386 copyright:
 *
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

