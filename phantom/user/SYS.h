/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Sycn: conds.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include <kernel/syscalls.h>

#ifdef ARCH_ia32

/* 
 * x86 version taken from Mach Operating System
 */


#include <mach/asm.h>


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

#endif



#ifdef ARCH_arm
#include <arm/asm.h>



//#define HASH #
//#define PASTE(a,b) PASTE1(a,b)
//#define PASTE1(a,b) a##b
//#define PASTE1(a,b) a/**/b

// not sure we need ret here as we don't "push le; mov lr, ip"
//#define	SYSCALL(x)	ENTRY(x); ldr r12, = PASTE( HASH , SYS_##x ); swi  0x0;
#define	SYSCALL(x)	ENTRY(x); ldr r12, = SYS_##x; swi  0x0;

//#undef HASH
//#undef PASTE
//#undef PASTE1

#endif


#ifdef ARCH_mips
#include <mips/asm.h>

// syscall instr has data field, but we don't use
// pass syscall number in k0
#define	SYSCALL(x)	LEAF(x); addiu k0, zero, SYS_##x; syscall; END(x);


#endif





#ifndef SYSCALL
#  error no syscall for this architecture?
#endif



