/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS startup code. 
 *
**/

#include <mips/cp0_regs.h>
#include <mips/asm.h>

LEAF(_start)
	.set	noreorder
	.set	mips32

	j _reset
    nop // delay slot

    .string "Phantom OS\r\n"
    .string "Copyright (C) Dmitry Zavalishin, 2011.\r\n"

    .align 8


_reset:
    /* Disable interrupts */
    mtc0	zero, CP0_STATUS

    /* Disable watch exception. */
    mtc0    zero, CP0_WATCHLO
    mtc0    zero, CP0_WATCHHI

    /* disable kernel mode cache */
    mfc0	t0, CP0_CONFIG
    and	t0, ~0x7
    ori	t0, 0x2
    mtc0	t0, CP0_CONFIG

    /* set up stack */
    la	sp, __startup_stack_top__

 	lui	v0,0xB400
 	ori	v0,v0,0x3f8
 	li	v1,104
 	sb	v1,0(v0)
 	li	v1,101
 	sb	v1,0(v0)
 	li	v1,108
 	sb	v1,0(v0)
 	sb	v1,0(v0)
 	li	v1,111
 	sb	v1,0(v0)
 	li	v1,33
 	sb	v1,0(v0)
 	li	v1,10
 	sb	v1,0(v0)
 	li	v1,13
 	sb	v1,0(v0)

	
    /* jump to main */
    jal	main
    nop

loop:
    j	loop
    nop
END(_start)

#define PHANTOM_START_STACK_SIZE (16*1024)
    .space	PHANTOM_START_STACK_SIZE
__startup_stack_top__:


// pagemask (cp0 r5) = 0 -> 4Kb page
// wired (cp0 r6) = 24 (half of TLB is wired)?

// get pr id = (cp0 r15)

// TODO cp0 r16 config



