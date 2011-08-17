/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM startup code. 
 *
**/

#include <arm/psr.h>
#include <arm/asm.h>

// todo: save contents of registers r1 and r2 in r7 and r8 to save off architecture ID and atags pointer passed in by bootloader 

    .text
    .code 32

    .globl	_start_of_kernel
_start_of_kernel:

    .global _start
    .func   _start
_start:
    // Vect table
    B           _trap_e_reset              // reset
    B           _trap_e_undef              // undef instr
    B           _trap_e_syscall            // SWI - soft int
    B           _trap_e_prefetch_abort     // prefetch abort
    B           _trap_e_data_abort         // data abort
    B           _trap_e_addrexc            // 
    B           _trap_e_irq                // irq
    B           _trap_e_fiq                // fiq

    // Multiboot header for any case - need it here?
    .long	0x1BADB002		/* magic */
    .long	0x00000000		/* feature flags */
    .long	0-0x1BADB002-0x00000000	/* checksum */


    .string "Phantom OS\r\n"
    .string "Copyright (C) Dmitry Zavalishin, 2011.\r\n"

    .align 4


_trap_e_reset:

    // Clear BSS

    LDR         r1, = __bss_start__
    LDR         r2, = __bss_end__
    MOV         r3, #0

1:
    CMP         r1, r2
    STMLTIA     r1!, {r3}
    BLT         1b

    // Set stacks

    MSR         CPSR_c, #(PSR_FIQ32_MODE | I_BIT | F_BIT)
    LDR         sp, = __fiq_stack_top__

    MSR         CPSR_c, #(PSR_IRQ32_MODE | I_BIT | F_BIT)
    LDR         sp, = __irq_stack_top__

    MSR         CPSR_c, #(PSR_SVC32_MODE | I_BIT | F_BIT)
    LDR         sp, = __svc_stack_top__

    MSR         CPSR_c, #(PSR_ABT32_MODE | I_BIT | F_BIT)
    LDR         sp, = __abt_stack_top__

    MSR         CPSR_c, #(PSR_UND32_MODE | I_BIT | F_BIT)
    LDR         sp, = __und_stack_top__

    MSR         CPSR_c, #(PSR_SYS32_MODE | I_BIT | F_BIT)
    LDR         sp, = __cmain_stack_top__



    // Set the Domain Access register. TODO move to paging start code
    // 0101b repeated = 5
    ldr     	r0, = 0x55555555
    mcr		p15, 0, r0, c3, c0, 0

    // Call main

    //LDR         r12, = EXT(phantom_multiboot_main)
.global phantom_multiboot_main
    LDR         r12, =phantom_multiboot_main
    MOV         lr, pc
    BX          r12

    // No return supposed, die somehow
    // Cause exception - TODO call some kind of hard reboot
    //SWI         0xFFFFFF
    adr		r0, .Lmainreturned
    mov         lr, pc // attempt to return will bring here
    b		_C_LABEL(panic)
    /* NOTEACHED */


    .data
    .align 12 // TODO use page size define

.Lmainreturned:
	.asciz	"main() returned"

    .bss
    .align 12 // TODO use page size define

//#include "stack.h"
#define PHANTOM_START_STACK_SIZE (16*1024)
    .space	PHANTOM_START_STACK_SIZE
__irq_stack_top__:

    .space	PHANTOM_START_STACK_SIZE
__svc_stack_top__:

    .space	PHANTOM_START_STACK_SIZE
__fiq_stack_top__:

    .space	PHANTOM_START_STACK_SIZE
__abt_stack_top__:

    .space	PHANTOM_START_STACK_SIZE
__und_stack_top__:

    .space	PHANTOM_START_STACK_SIZE
__cmain_stack_top__:


    .endfunc



/**
 *
 * General idea is:
 *
 * - save R0-R6 to whatever stack we got
 *
 * - put specific data to R3 (swi code, for example)
 * - put trap number (index in trap array) to R4
 * - put LR we got to R2, correcting it as needed
 *
 *
 * ! jump to alltraps
 *
 * - put SPSR (old PSR) to R0
 * - put SP we have to R1
 * - switch to SYS32 mode (and to it's stack, sure) - KEEP interrupts closed!
 *
 * - leave R5 as scratch for later code
 *
 * - find if we're from user mode, if so - switch to curr thread's kernel stack, saving old stack in R6
 *
 * - Get stuff off the stack we used before (R1 keeps its SP now) and build return record and trap_state on actual stack
 *
 * ? Do smthng with FP (R11)
 *
 * - Still with ints closed call C handler, passing trap_state address in R0
 *
 * - Cleanup junk and leave trap
 *
**/

.text
//.code 32



_trap_e_undef:
    push        {r0-r6}
    mov         r4, #1
    mov         r2, lr
    b           alltraps

_trap_e_syscall:
    push        {r0-r6}

    // read SWI instruction to R3
    ldr         r3, [ lr, #-4 ]

    mov         r4, #2
    mov         r2, lr
    b           alltraps

_trap_e_prefetch_abort:
    push        {r0-r6}
    mov         r4, #3
    sub         r2, lr, #4 // ret one instr back
    b           alltraps

_trap_e_data_abort:
    push        {r0-r6}
    mov         r4, #4
    sub         r2, lr, #8 // ret 2 instr back
    b           alltraps

_trap_e_addrexc:
    push        {r0-r6}
    mov         r4, #5
    mov         r2, lr // TODO fix lr?
    b           alltraps

_trap_e_irq:
    push        {r0-r6}
    mov         r4, #6
    sub         r2, lr, #4 // ret one instr back
    b           alltraps

_trap_e_fiq:
    push        {r0-r6}
    mov         r4, #7
    sub         r2, lr, #4 // ret one instr back
    b           alltraps




alltraps:

    mrs		r0, spsr_all		// Save spsr
    mov         r1, sp

    // switch to SYS mode

    mrs     	r5, cpsr_all
    bic     	r5, r5, #(PSR_MODE)
    orr     	r5, r5, #(PSR_SYS32_MODE|I_BIT|F_BIT)
    msr     	cpsr_all, r5	   

    mov         r6, sp

    and         r5, r0, #(PSR_MODE)     // Extract mode bits
    subs        r5, r5, #(PSR_USR32_MODE)
    bne         not_user

    // TODO here we have to load cur thread's kernel stack top to SP

    // TODO Set it up in thread switch code. Not so easy on SMP :(
.global EXT(curr_thread_k0_stack_top)
    LDR         r5, = EXT(curr_thread_k0_stack_top)
    ldr         SP, [r5]

not_user:

    // We're on a steady stack now.

    // Lets build exception return frame first - ? No.

    // Build trap_state

    push        {r0}          // SPSR
    push        {r2}          // LR = his PC

    push        {r3}          // Specific info - fro swi it's swi instruction itself
    push        {r4}          // trapno

    push        {lr}  	    // user mode lr
    push        {r6}  	    // user mode sp we saved above when switched to kernel stack
    //push        {r1}  	    // user mode sp we saved above

    push        {r7-r12}    // Now save the rest of user's registers, at last

    // Now we're free to use R7-R10 (11 is FP, 12 is not saved by callee)

    mov         r7, r1      // This is the stack pointer we used to store original R0-R6

    ldmfd       r7, {r0-r6} // Get 'em back, don't bother to update R7
    push        {r0-r6}     // Now re-store to correct stack

    // Do smth with FP? Clear, at least?
    //mov         r11, #0

    //mov         r10, sp     // r10 is callee saved, store trap_state there for us
    mov         r0, sp      // here is the arg to C function - address of struct trap_state we just built
    bl          _C_LABEL(phantom_kernel_trap)

    // Here we have R10 pointing to struct trap_state, and most of the other registers are free to be used

    // Turn off IRQ/FIQ
    mrs     	r5, cpsr_all
    orr     	r5, r5, #(I_BIT|F_BIT)
    msr     	cpsr_all, r5	   

    // Will return through SVC mode

    LDR         r9, = __svc_stack_top__

    ldmia	sp!, {r0-r6} // Get from our trap_state
    stmdb       r9!, {r0-r6}  // push to SVC stack

    ldmia	sp!, {r7-r12} // This is final, don't touch 'em anymore

    // Now we can use just R0-6

    ldmia	sp!, {r1-r6} // Get special stuff: R6=spsr, R5=lr, R4=intno (ignore), R3=trapno (ignore), R2=usr_lr, R1=usr_sp

    // That's all , now old SP is unused and will be overwritten below

    mov         lr, r2
    mov         sp, r1

    MSR         CPSR_c, #(PSR_SVC32_MODE | I_BIT | F_BIT)

    msr     	spsr, r6
    mov         lr, r5

    pop         {r0-r6}

    // move lr to pc and spsr to cpsr
    movs        pc, lr

    // Finita






ENTRY(call_arm_angel)
    swi 	#0x123456
    mov         pc, lr
    












#if 0


    .end







#endif

#if 0

/*	$NetBSD: locore.S,v 1.14 2003/04/20 16:21:40 thorpej Exp $	*/


#include "assym.s"
#include <sys/syscall.h>
#include <machine/asm.h>
#include <machine/armreg.h>
#include <machine/pte.h>
__FBSDID("$FreeBSD: src/sys/arm/arm/locore.S,v 1.17.2.1.2.1 2008/11/25 02:59:29 kensmith Exp $");

/* What size should this really be ? It is only used by initarm() */
#define INIT_ARM_STACK_SIZE	2048

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */


#define	CPWAIT_BRANCH							 \
	sub	pc, pc, #4

#define	CPWAIT(tmp)							 \
	mrc	p15, 0, tmp, c2, c0, 0	/* arbitrary read of CP15 */	;\
	mov	tmp, tmp		/* wait for it to complete */	;\
	CPWAIT_BRANCH			/* branch to next insn */

	.text
	.align	0
.globl kernbase
.set kernbase,KERNBASE
.globl physaddr
.set physaddr,PHYSADDR

ENTRY_NP(btext)

ASENTRY_NP(_start)
#if defined (FLASHADDR) && defined(LOADERRAMADDR)
	/* Check if we're running from flash. */
	ldr	r7, =FLASHADDR
	/* 
	 * If we're running with MMU disabled, test against the
	 * physical address instead.
	 */
	mrc     p15, 0, r2, c1, c0, 0
	ands	r2, r2, #CPU_CONTROL_MMU_ENABLE
	ldreq	r8, =PHYSADDR
	ldrne	r8, =LOADERRAMADDR
	cmp	r7, r8
	bls 	flash_lower
	cmp	r7, pc
	bhi	from_ram
	b	do_copy
	
flash_lower:
	cmp	r8, pc
	bls	from_ram
do_copy:
	ldr	r9, =KERNBASE
	adr	r1, _start 
	ldr	r0, Lreal_start
	ldr	r2, Lend
	sub	r2, r2, r0
	sub	r0, r0, r9
	add	r0, r0, r8
	mov	r4, r0
	bl	memcpy
	ldr	r0, Lram_offset
	add	pc, r4, r0
Lram_offset:	.word from_ram-_C_LABEL(_start)
from_ram:
	nop
#endif
	adr	r7, Lunmapped
	bic     r7, r7, #0xff000000
	orr     r7, r7, #PHYSADDR
			

disable_mmu:
	/* Disable MMU for a while */
	mrc     p15, 0, r2, c1, c0, 0
	bic	r2, r2, #(CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_DC_ENABLE |\
	    CPU_CONTROL_WBUF_ENABLE)
	bic	r2, r2, #(CPU_CONTROL_IC_ENABLE)
	bic	r2, r2, #(CPU_CONTROL_BPRD_ENABLE)
	mcr     p15, 0, r2, c1, c0, 0

	nop
	nop
	nop
	mov	pc, r7
Lunmapped:
#ifdef STARTUP_PAGETABLE_ADDR
	/* build page table from scratch */
	ldr	r0, Lstartup_pagetable
	adr	r4, mmu_init_table
	b	3f

2:
	str	r3, [r0, r2]
	add	r2, r2, #4
	add	r3, r3, #(L1_S_SIZE)
	adds	r1, r1, #-1
	bhi	2b
3:
	ldmia	r4!, {r1,r2,r3}   /* # of sections, VA, PA|attr */
	cmp	r1, #0
	adrne	r5, 2b
	bicne	r5, r5, #0xff000000
	orrne	r5, r5, #PHYSADDR
	movne	pc, r5

	mcr	p15, 0, r0, c2, c0, 0	/* Set TTB */
	mcr	p15, 0, r0, c8, c7, 0	/* Flush TLB */

	/* Set the Domain Access register.  Very important! */
	mov     r0, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r0, c3, c0, 0
	/* Enable MMU */
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r0, c1, c0, 0
	nop
	nop
	nop
	CPWAIT(r0)

#endif
mmu_done:
	nop
	adr	r1, .Lstart
	ldmia	r1, {r1, r2, sp}	/* Set initial stack and */
	sub	r2, r2, r1		/* get zero init data */
	mov	r3, #0
.L1:
	str	r3, [r1], #0x0004	/* get zero init data */
	subs	r2, r2, #4
	bgt	.L1
	ldr	pc, .Lvirt_done

virt_done:
	mov	fp, #0		/* trace back starts here */
	bl	_C_LABEL(initarm)	/* Off we go */

	/* init arm will return the new stack pointer. */
	mov	sp, r0

	bl	_C_LABEL(mi_startup)		/* call mi_startup()! */

	adr	r0, .Lmainreturned
	b	_C_LABEL(panic)
	/* NOTEACHED */
#ifdef STARTUP_PAGETABLE_ADDR
#define MMU_INIT(va,pa,n_sec,attr) \
	.word	n_sec					    ; \
	.word	4*((va)>>L1_S_SHIFT)			    ; \
	.word	(pa)|(attr)				    ;

Lvirtaddr:
	.word	KERNVIRTADDR
Lphysaddr:
	.word	KERNPHYSADDR
Lreal_start:
	.word	_start
Lend:	
	.word	_edata
Lstartup_pagetable:
	.word	STARTUP_PAGETABLE_ADDR
mmu_init_table:
	/* fill all table VA==PA */
	/* map SDRAM VA==PA, WT cacheable */
	MMU_INIT(PHYSADDR, PHYSADDR , 64, L1_TYPE_S|L1_S_C|L1_S_AP(AP_KRW))
	/* map VA 0xc0000000..0xc3ffffff to PA */
	MMU_INIT(KERNBASE, PHYSADDR, 64, L1_TYPE_S|L1_S_C|L1_S_AP(AP_KRW))

	.word 0	/* end of table */
#endif
.Lstart:
	.word	_edata
	.word	_end
	.word	svcstk + INIT_ARM_STACK_SIZE

#if defined(FLASHADDR) && defined(LOADERRAMADDR)
.L_arm_memcpy:
        .word   _C_LABEL(_arm_memcpy)
#endif

.Lvirt_done:
	.word	virt_done
.Lmainreturned:
	.asciz	"main() returned"
	.align	0

	.bss
svcstk:
	.space	INIT_ARM_STACK_SIZE

	.text
	.align	0

#ifndef OFW
	/* OFW based systems will used OF_boot() */

.Lcpufuncs:
	.word	_C_LABEL(cpufuncs)

ENTRY_NP(cpu_halt)
	mrs     r2, cpsr
	bic	r2, r2, #(PSR_MODE)
	orr     r2, r2, #(PSR_SVC32_MODE)
	orr	r2, r2, #(I32_bit | F32_bit)
	msr     cpsr_all, r2

	ldr	r4, .Lcpu_reset_address
	ldr	r4, [r4]

	ldr	r0, .Lcpufuncs
	mov	lr, pc
	ldr	pc, [r0, #CF_IDCACHE_WBINV_ALL]

	/*
	 * Load the cpu_reset_needs_v4_MMU_disable flag to determine if it's
	 * necessary.
	 */

	ldr	r1, .Lcpu_reset_needs_v4_MMU_disable
	ldr	r1, [r1]
	cmp	r1, #0
	mov	r2, #0

	/*
 	 * MMU & IDC off, 32 bit program & data space
	 * Hurl ourselves into the ROM
	 */
	mov	r0, #(CPU_CONTROL_32BP_ENABLE | CPU_CONTROL_32BD_ENABLE)
	mcr     15, 0, r0, c1, c0, 0
	mcrne   15, 0, r2, c8, c7, 0 	/* nail I+D TLB on ARMv4 and greater */
	mov     pc, r4

	/*
	 * _cpu_reset_address contains the address to branch to, to complete
	 * the cpu reset after turning the MMU off
	 * This variable is provided by the hardware specific code
	 */
.Lcpu_reset_address:
	.word	_C_LABEL(cpu_reset_address)

	/*
	 * cpu_reset_needs_v4_MMU_disable contains a flag that signals if the
	 * v4 MMU disable instruction needs executing... it is an illegal instruction
	 * on f.e. ARM6/7 that locks up the computer in an endless illegal
	 * instruction / data-abort / reset loop.
	 */
.Lcpu_reset_needs_v4_MMU_disable:
	.word	_C_LABEL(cpu_reset_needs_v4_MMU_disable)

#endif	/* OFW */


	.data
	.global _C_LABEL(esym)
_C_LABEL(esym):	.word	_C_LABEL(end)

ENTRY_NP(abort)
	b	_C_LABEL(abort)

ENTRY_NP(sigcode)
	mov	r0, sp
	swi	SYS_sigreturn

	/* Well if that failed we better exit quick ! */

	swi	SYS_exit
	b	. - 8

	.align	0
	.global _C_LABEL(esigcode)
		_C_LABEL(esigcode):

	.data
	.global szsigcode
szsigcode:
	.long esigcode-sigcode
/* End of locore.S */


#endif
