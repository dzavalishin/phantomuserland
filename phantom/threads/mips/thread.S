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
#include "cpu_state.h"
            
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

// TODO save/restore page fault addr? Or just ensure we're reading it soon enough with intrs disabled?
LEAF(phantom_switch_context)
    // save s0-s7 on stack, 8 regs * 4 bytes
    addi        sp, sp, -(8*4)

    sw          s0, 0(sp)
    sw          s1, 4(sp)
    sw          s2, 8(sp)
    sw          s3, 12(sp)
    sw          s4, 16(sp)
    sw          s5, 20(sp)
    sw          s6, 24(sp)
    sw          s7, 28(sp)

    // Save SP
    sw          sp, CSTATE_SP(a0)
    // Save GP
    sw          gp, CSTATE_GP(a0)
    // Save FP
    sw          fp, CSTATE_FP(a0)
    // Save RA
    sw          ra, CSTATE_RA(a0)

    // Load SP
    lw          sp, CSTATE_SP(a0)
    // Load GP
    lw          gp, CSTATE_GP(a0)
    // Load FP
    lw          fp, CSTATE_FP(a0)
    // Load RA
    lw          ra, CSTATE_RA(a0)

    lw          s0, 0(sp)
    lw          s1, 4(sp)
    lw          s2, 8(sp)
    lw          s3, 12(sp)
    lw          s4, 16(sp)
    lw          s5, 20(sp)
    lw          s6, 24(sp)
    lw          s7, 28(sp)

    addi        sp, sp, (8*4)

    // Unlock given spinlock

    addiu       a0, a2, 0
    j    	EXT(hal_spin_unlock)

END(phantom_switch_context)


        

//* new thread starts here - no args
LEAF(phantom_thread_trampoline)
    j           _C_LABEL(phantom_thread_c_starter)
    nop // delay slot
    //hlt // not reached
END(phantom_thread_trampoline)



