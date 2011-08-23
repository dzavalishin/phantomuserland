/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Low level thread switch code.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include <mips/asm.h>
#include <mips/cp0_regs.h>
#include <cpu_state.h>
            
/*
 * void phantom_switch_context(
 *                           phantom_thread_t *from,
 *                           phantom_thread_t *to,
 *                           int *unlock );
 *
 *
 * a0 - current thread
 * a1 - next thread
 * a2 - lock
 *
 * called and returns with interrupts disabled
 */

#define N_STACK_SLOTS 16

// TODO save/restore page fault addr? Or just ensure we're reading it soon enough with intrs disabled?
LEAF(phantom_switch_context)
    // save s0-s7 on stack, 8 regs * 4 bytes
    addi        sp, sp, -(N_STACK_SLOTS*4)

    sw          s0, 0(sp)
    sw          s1, 4(sp)
    sw          s2, 8(sp)
    sw          s3, 12(sp)
    sw          s4, 16(sp)
    sw          s5, 20(sp)
    sw          s6, 24(sp)
    sw          s7, 28(sp)

    mfhi t0
    sw          t0, 32(sp)
    mflo t0
    sw          t0, 36(sp)

    mfc0	t0, CP0_STATUS
    sw          t0, CSTATE_STATUS(a0)

    // Save SP
    sw          sp, CSTATE_SP(a0)
    // Save GP
    sw          gp, CSTATE_GP(a0)
    // Save FP
    sw          fp, CSTATE_FP(a0)
    // Save RA
    sw          ra, CSTATE_RA(a0)

    // ------------------------- Switch

    // Load SP
    lw          sp, CSTATE_SP(a1)
    // Load GP
    lw          gp, CSTATE_GP(a1)
    // Load FP
    lw          fp, CSTATE_FP(a1)
    // Load RA
    lw          ra, CSTATE_RA(a1)

    lw          t0, CSTATE_STATUS(a1)
    mtc0	t0, CP0_STATUS

    lw          t0, 32(sp)
    mthi t0
    lw          t0, 36(sp)
    mtlo t0

    lw          s0, 0(sp)
    lw          s1, 4(sp)
    lw          s2, 8(sp)
    lw          s3, 12(sp)
    lw          s4, 16(sp)
    lw          s5, 20(sp)
    lw          s6, 24(sp)
    lw          s7, 28(sp)

    addi        sp, sp, (N_STACK_SLOTS*4)

    // Unlock given spinlock

    addiu       a0, a2, 0
    j    	EXT(hal_spin_unlock)
    nop // delay slot
END(phantom_switch_context)


        

//* new thread starts here - no args
LEAF(phantom_thread_trampoline)
    j           _C_LABEL(phantom_thread_c_starter)
    nop // delay slot
    //hlt // not reached
END(phantom_thread_trampoline)



