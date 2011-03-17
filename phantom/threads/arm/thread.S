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

#include <arm/asm.h>
#include "cpu_state.h"
            
/*
 * void phantom_switch_context(
 *                           phantom_thread_t *from,
 *                           phantom_thread_t *to,
 *                           int *unlock );
 *
 *
 * r0 - current thread
 * r1 - next thread
 * r2 - lock
 *
 * r3 is scratch reg
 *
 * called and returns with interrupts disabled
 */

// TODO save/restore page fault addr? Or just ensure we're reading it soon enough with intrs disabled?
ENTRY(phantom_switch_context)

    push        {r4-r12}

    // Save R11/FP
    str         r11, [ r0, #CSTATE_FP ]
    // Save R15/IP
    str         r15, [ r0, #CSTATE_IP ]
    // Save R14/LR
    str         r14, [ r0, #CSTATE_LR ]
    // Save R13/SP
    str         r13, [ r0, #CSTATE_SP ]

    // Save CPSR - need it?
    mrs    	r3, cpsr
    str         r3,  [ r0, #CSTATE_CPSR ]

    // Load CPSR - NB! - check for interrupts to be disabled!?
    ldr         r3,  [ r1, #CSTATE_CPSR ]
    msr         cpsr, r3

    // Load R15/IP
    ldr         r15, [ r1, #CSTATE_IP ]
    // Save R14/LR
    ldr         r14, [ r1, #CSTATE_LR ]
    // Save R13/SP
    ldr         r13, [ r1, #CSTATE_SP ]
    // Save R11/FP
    ldr         r11, [ r1, #CSTATE_FP ]


    pop         {r4-r12}

    // Unlock (set to zero) given spinlock
    mov         r0, #0
    str         r0, [ r2 ]


    // Return to new thread's saved link register.
    mov         r15, r14


        
#if 0
    /**
     * new thread starts here with
     *   r4 -> r0 = func
     *   r5 -> r1 = arg
     *   r6 -> r2 = thread struct addr
     */
ENTRY(phantom_thread_trampoline)
    //pushl %ebx // tstruct
    //pushl %edi // param
    //pushl %esi 
    mov         r0, r4 // func addr
    mov         r1, r5 // arg
    mov         r2, r6 // thread struct addr
    // Just jump there
    b           EXT(phantom_thread_c_starter)
    //hlt // not reached


#endif
