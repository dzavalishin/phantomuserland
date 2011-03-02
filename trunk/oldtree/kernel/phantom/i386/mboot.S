/*-
 * Copyright (c) 1995, Jack F. Vogel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Jack F. Vogel
 * 4. The name of the developer may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * mpboot.s:	FreeBSD machine support for the Intel MP Spec
 *		multiprocessor systems.
 *
 * $FreeBSD: src/sys/i386/i386/mpboot.s,v 1.24.22.1.2.1 2009/10/25 01:10:29 kensmith Exp $
 */

//#include "opt_pmap.h"

#include <mach/asm.h>		/* miscellaneous asm macros */
#include <kernel/page.h>

//#include <x86/phantom_page.h>
#include <i386/proc_reg.h>
#include <i386/seg.h>

//#include <machine/apicreg.h>
//#include <machine/specialreg.h>

//#include "assym.s"

//#define	R(x)	((x)-KERNBASE)

/*
 * this code MUST be enabled here and in mp_machdep.c
 * it follows the very early stages of AP boot by placing values in CMOS ram.
 * it NORMALLY will never be needed and thus the primitive method for enabling.
 *
#define CHECK_POINTS
 */

#if defined(CHECK_POINTS) && !defined(PC98)

#define CMOS_REG	(0x70)
#define CMOS_DATA	(0x71)

#define CHECKPOINT(A,D)		\
	movb	$(A),%al ;	\
	outb	%al,$CMOS_REG ;	\
	movb	$(D),%al ;	\
	outb	%al,$CMOS_DATA

#else

#define CHECKPOINT(A,D)

#endif /* CHECK_POINTS */


/*
 * the APs enter here from their trampoline code (bootMP, below)
 */
	.p2align 4

NON_GPROF_ENTRY(MPentry)
	CHECKPOINT(0x36, 3)
	/*
	 * Enable features on this processor.  We don't support SMP on
	 * CPUs older than a Pentium, so we know that we can use the cpuid
	 * instruction.
	 */
	movl	$1,%eax
	cpuid					/* Retrieve features */
	movl	%cr4,%eax
//hlt
/*
#ifndef DISABLE_PSE
	testl	$CPUID_PSE,%edx
	jz 1f
	orl	$CR4_PSE,%eax			// Enable PSE
1:
#endif
#ifndef DISABLE_PG_G
	testl	$CPUID_PGE,%edx
	jz 1f
	orl	$CR4_PGE,%eax			// Enable PGE
1:	
#endif
	testl	$CPUID_VME,%edx
	jz 1f
	orl	$CR4_VME,%eax			// Enable VME
*/
1:
	movl	%eax,%cr4

    // phantom kernel lives in p=v, so we don't have to turn paging here
    // we'll do that in C code
#if 0 
	/* Now enable paging mode */
#ifdef PAE
	movl	R(IdlePDPT), %eax
	movl	%eax, %cr3
	movl	%cr4, %eax
	orl	$CR4_PAE, %eax
	movl	%eax, %cr4
#else
	movl	R(IdlePTD), %eax
	movl	%eax,%cr3	
#endif

	movl	%cr0,%eax
	orl	$CR0_PE|CR0_PG,%eax		/* enable paging */
	movl	%eax,%cr0			/* let the games begin! */
	movl	bootSTK,%esp			/* boot stack end loc. */
#endif
//hlt
	pushl	$mp_begin			/* jump to high mem */
	ret

	/*
	 * Wait for the booting CPU to signal startup
	 */
mp_begin:	/* now running relocated at KERNBASE */
    CHECKPOINT(0x37, 4)
// hlt
    call	EXT(smp_ap_start)			/* load i386 tables */

	.data
	P2ALIGN(PAGE_SHIFT)
        .space	0x1000	/* space for bootSTK - 2nd temporary stack */
bootSTK:

/*
 * This is the embedded trampoline or bootstrap that is
 * copied into 'real-mode' low memory, it is where the
 * secondary processor "wakes up". When it is executed
 * the processor will eventually jump into the routine
 * MPentry, which resides in normal kernel text above
 * 1Meg.		-jackv
 */

	.data
//	ALIGN_DATA				/* just to be sure */
	P2ALIGN(PAGE_SHIFT)

BOOTMP1:

NON_GPROF_ENTRY(bootMP)
bootMP:
	.code16		
	cli
	CHECKPOINT(0x34, 1)
	/* First guarantee a 'clean slate' */
	xorl	%eax, %eax
	movl	%eax, %ebx
	movl	%eax, %ecx
 	movl	%eax, %edx
	movl	%eax, %esi
	movl	%eax, %edi

	/* set up data segments */
	mov	%cs, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	mov	%ax, %ss
	mov	$(boot_stk-bootMP), %esp

//hlt // ok

	/* Now load the global descriptor table */
	lgdt	(MP_GDTptr-bootMP)

	/* Enable protected mode */
	movl	%cr0, %eax
	orl	$CR0_PE, %eax
	movl	%eax, %cr0 
//hlt // ok
	/*
	 * make intrasegment jump to flush the processor pipeline and
	 * reload CS register
	 */
#if 1
        pushl	$0x18
	//pushl	$KERNEL_DS
	//pushl	$KERNEL_CS
	pushl	$(protmode-bootMP)
	lretl
#else
        pushf
        push	$0x18
	pushl	$(protmode-bootMP)
        iretl;
#endif

       .code32		
protmode:
	CHECKPOINT(0x35, 2)
//hlt
	/*
	 * we are NOW running for the first time with %eip
	 * having the full physical address, BUT we still
	 * are using a segment descriptor with the origin
	 * not matching the booting kernel.
	 *
 	 * SO NOW... for the BIG Jump into kernel's segment
	 * and physical text above 1 Meg.
	 */
	mov	$0x10, %ebx
	//mov	$KERNEL_DS, %ebx
	movw	%bx, %ds
	movw	%bx, %es
	movw	%bx, %fs
	movw	%bx, %gs
	movw	%bx, %ss
//hlt
	.globl	bigJump
ENTRY(bigJump)
	/* this will be modified by mpInstallTramp() */
	ljmp	$0x08, $0			/* far jmp to MPentry() */
	
dead:	hlt /* We should never get here */
	jmp	dead

/*
 * MP boot strap Global Descriptor Table
 */
	.p2align 4
	.globl	MP_GDT
	.globl	bootCodeSeg
	.globl	bootDataSeg
ENTRY(MP_GDT)

nulldesc:		/* offset = 0x0 */

	.word	0x0	
	.word	0x0	
	.byte	0x0	
	.byte	0x0	
	.byte	0x0	
	.byte	0x0	

kernelcode:		/* offset = 0x08 */

	.word	0xffff	/* segment limit 0..15 */
	.word	0x0000	/* segment base 0..15 */
	.byte	0x0	/* segment base 16..23; set for 0K */
	.byte	0x9f	/* flags; Type	*/
	.byte	0xcf	/* flags; Limit	*/
	.byte	0x0	/* segment base 24..32 */

kerneldata:		/* offset = 0x10 */

	.word	0xffff	/* segment limit 0..15 */
	.word	0x0000	/* segment base 0..15 */
	.byte	0x0	/* segment base 16..23; set for 0k */
	.byte	0x93	/* flags; Type  */
	.byte	0xcf	/* flags; Limit */
	.byte	0x0	/* segment base 24..32 */

bootcode:		/* offset = 0x18 */

        .word	0xffff	/* segment limit 0..15 */
// NB! Not ENTRY()!
            //ENTRY(bootCodeSeg)     	/* this will be modified by mpInstallTramp() */
.globl EXT(bootCodeSeg); LEXT(bootCodeSeg)
	.word	0x0000	/* segment base 0..15 */
	.byte	0x00	/* segment base 16...23; set for 0x000xx000 */
	.byte	0x9e	/* flags; Type  */
	.byte	0xcf	/* flags; Limit */
	.byte	0x0	/*segment base 24..32 */

bootdata:		/* offset = 0x20 */

	.word	0xffff	
// NB! Not ENTRY()!
    //ENTRY(bootDataSeg)     	/* this will be modified by mpInstallTramp() */
.globl EXT(bootDataSeg); LEXT(bootDataSeg)
	.word	0x0000	/* segment base 0..15 */
	.byte	0x00	/* segment base 16...23; set for 0x000xx000 */
	.byte	0x92	
	.byte	0xcf	
	.byte	0x0		

/*
 * GDT pointer for the lgdt call
 */
	.globl	mp_gdtbase

MP_GDTptr:	
mp_gdtlimit:
	.word	0x0028		
// NB! Not ENTRY()!
//ENTRY(mp_gdtbase)      	/* this will be modified by mpInstallTramp() */
.globl EXT(mp_gdtbase); LEXT(mp_gdtbase)
	.long	0

	.space	0x100	/* space for boot_stk - 1st temporary stack */
boot_stk:

/*
    .code16
Puts16:
		pusha				; save registers
putsLoop1:
		lodsb				; load next byte from string from SI to AL
		or	al, al			; Does AL=0?
		jz	Puts16Done		; Yep, null terminator found-bail out
		mov	ah, 0xe			; Nope-Print the character
		int	0x10			; invoke BIOS
		jmp	putsLoop1			; Repeat until null terminator found
Puts16Done:
		popa				; restore registers
		ret				; we are done, so return
*/



BOOTMP2:
	.globl	bootMP_size
ENTRY(bootMP_size)
	.long	BOOTMP2 - BOOTMP1

