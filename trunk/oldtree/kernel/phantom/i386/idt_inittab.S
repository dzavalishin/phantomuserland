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











#include <mach/asm.h>

#include <i386/seg.h>

#define FULL_STACK_TRACE

/* We'll be using macros to fill in a table in data hunk 2
   while writing trap entrypoint routines at the same time.
   Here's the header that comes before everything else.  */
	.data	2
ENTRY(idt_inittab)
	.text

/*
 * Interrupt descriptor table and code vectors for it.
 */
#define	IDT_ENTRY(n,entry,type) \
	.data	2		;\
	.long	entry		;\
	.word	n		;\
	.word	type		;\
	.text

/*
 * No error code.  Clear error code and push trap number.
 */
#define	EXCEPTION(n,name) \
	IDT_ENTRY(n,EXT(name),ACC_PL_K|ACC_TRAP_GATE);\
ENTRY(name)				;\
	pushl	$(0)			;\
	pushl	$(n)			;\
	jmp	EXT(phantom_alltraps)

/*
 * User-accessible exception.  Otherwise, same as above.
 */
#define	EXCEP_USR(n,name) \
	IDT_ENTRY(n,EXT(name),ACC_PL_U|ACC_TRAP_GATE);\
ENTRY(name)				;\
	pushl	$(0)			;\
	pushl	$(n)			;\
	jmp	EXT(phantom_alltraps)

/*
 * Error code has been pushed.  Just push trap number.
 */
#define	EXCEP_ERR(n,name) \
	IDT_ENTRY(n,EXT(name),ACC_PL_K|ACC_INTR_GATE);\
ENTRY(name)				;\
	pushl	$(n)			;\
	jmp	EXT(phantom_alltraps)

/*
 * Special interrupt code: dispatches to a unique entrypoint,
 * not defined automatically here.
 */
#define	EXCEP_SPC(n,name)  \
	IDT_ENTRY(n,EXT(name),ACC_PL_K|ACC_TRAP_GATE)


EXCEPTION(0x00,t_zero_div)
    //EXCEP_SPC(0x01,t_debug)
EXCEPTION(0x01,t_debug)

EXCEPTION(0x02,t_nmi)

/* skip NMI interrupt - let more specific code figure that out.  */
EXCEP_USR(0x03,t_int3)
EXCEP_USR(0x04,t_into)
EXCEP_USR(0x05,t_bounds)
EXCEPTION(0x06,t_invop)
EXCEPTION(0x07,t_nofpu)
EXCEPTION(0x08,a_dbl_fault)
EXCEPTION(0x09,a_fpu_over)
EXCEPTION(0x0a,a_inv_tss)

//EXCEP_SPC(0x0b,t_segnp)
EXCEP_ERR(0x0b,t_segnp)

EXCEP_ERR(0x0c,t_stack_fault)

//EXCEP_SPC(0x0d,t_gen_prot)
//EXCEP_SPC(0x0e,t_page_fault)

EXCEP_ERR(0x0d,t_gen_prot)
EXCEP_ERR(0x0e,t_page_fault)


EXCEPTION(0x0f,t_trap_0f)
EXCEPTION(0x10,t_fpu_err)
EXCEPTION(0x11,t_trap_11)
EXCEPTION(0x12,t_trap_12)
EXCEPTION(0x13,t_trap_13)
EXCEPTION(0x14,t_trap_14)
EXCEPTION(0x15,t_trap_15)
EXCEPTION(0x16,t_trap_16)
EXCEPTION(0x17,t_trap_17)
EXCEPTION(0x18,t_trap_18)
EXCEPTION(0x19,t_trap_19)
EXCEPTION(0x1a,t_trap_1a)
EXCEPTION(0x1b,t_trap_1b)
EXCEPTION(0x1c,t_trap_1c)
EXCEPTION(0x1d,t_trap_1d)
EXCEPTION(0x1e,t_trap_1e)
EXCEPTION(0x1f,t_trap_1f)

/* Terminator */
	.data	2
	.long	0


/*
 * All 'exceptions' enter here with:
 *	esp->   trap number
 *		error code
 *		old eip
 *		old cs
 *		old eflags
 *		old esp		if trapped from user
 *		old ss		if trapped from user
 */
ENTRY(phantom_alltraps)
    .cfi_startproc
    .cfi_def_cfa esp, 20
    .cfi_offset eip, -12
    pusha				/* save the general registers */
    .cfi_adjust_cfa_offset 32
    .cfi_offset ebp, -44 
trap_push_segs:
    pushl	%ds			/* and the segment registers */
    pushl	%es
    pushl	%fs
    pushl	%gs

    .cfi_adjust_cfa_offset 16

    /* Note that we have to load the segment registers
     even if this is a trap from the kernel,
     because the kernel uses user segment registers for copyin/copyout.
     (XXX Would it be smarter just to use fs or gs for that?)  */
    mov	%ss,%ax			/* switch to kernel data segment */
    mov	%ax,%ds			/* (same as kernel stack segment) */
    mov	%ax,%es


    cld				/* clear direction flag */
#if 0
    // TODO special C func to go on V86 trap
    testl	$(EFL_VM),R_EFLAGS(%esp) /* in V86 mode? */
    jnz	trap_from_user		/* user mode trap if so */
    testb	$3,R_CS(%esp)		/* user mode trap? */
    jz	trap_from_kernel	/* kernel trap if not */
trap_from_user:
#endif

    //pushl	%esp			/* pass parameter */
    //call	EXT(phantom_kernel_trap)	/* to kernel trap routine */
    //addl	$4,%esp			/* pop parameter */


#ifdef FULL_STACK_TRACE
#define TS_EIP(r) 0x38(r)
	/*
	 * Fake a stack frame for back traces
	 */
	movl	%esp,%edx
	pushl	TS_EIP(%esp)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edx
    .cfi_adjust_cfa_offset 12
	call    EXT(phantom_kernel_trap)	/* to kernel trap routine */
	movl	%ebp,%esp
	popl	%ebp
	popl	%edx
#else
	pushl	%esp
    .cfi_adjust_cfa_offset 4
	call    EXT(phantom_kernel_trap)	/* to kernel trap routine */
	popl	%edx
#endif



    // Now return
    popl	%gs			/* restore segment registers */
    popl	%fs
    popl	%es
    popl	%ds
    popa				/* restore general registers */
    addl	$8,%esp			/* discard trap number and error code */
    iret				/* return from interrupt */

    .cfi_endproc







#if 0 // Example of EXCEP_SPC(0x0d,t_debug) handler code

/*
 * Debug trap.  Check for single-stepping across system call into
 * kernel.  If this is the case, taking the debug trap has turned
 * off single-stepping - save the flags register with the trace
 * bit set.
 */
ENTRY(t_debug)
	testl	$(EFL_VM),8(%esp)	/* is trap from V86 mode? */
	jnz	0f			/* isn`t kernel trap if so */
	testl	$3,4(%esp)		/* is trap from kernel mode? */
	jnz	0f			/* if so: */
	cmpl	$syscall_entry,(%esp)	/* system call entry? */
	jne	0f			/* if so: */
					/* flags are sitting where syscall */
					/* wants them */
	addl	$8,%esp			/* remove eip/cs */
	jmp	syscall_entry_2		/* continue system call entry */

0:	pushl	$0			/* otherwise: */
	pushl	$(T_DEBUG)		/* handle as normal */
	jmp	EXT(alltraps)		/* debug fault */

/*
 * Page fault traps save cr2.
 */
ENTRY(t_page_fault)
	pushl	$(T_PAGE_FAULT)		/* mark a page fault trap */
	pusha				/* save the general registers */
	movl	%cr2,%eax		/* get the faulting address */
	movl	%eax,12(%esp)		/* save in esp save slot */
	jmp	trap_push_segs		/* continue fault */

#endif



#if 0

#ifdef FULL_STACK_TRACE
	/*
	 * Fake a stack frame for back traces
	 */
	movl	%esp,%edx
	pushl	TS_EIP(%esp)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edx
	call	*%eax
	movl	%ebp,%esp
	popl	%ebp
	popl	%edx
#else
	pushl	%esp
	call	*%eax
	popl	%edx
#endif

#endif
