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

#include <mach/asm.h>
#include "cpu_state.h"
            
// called and returns with interrupts disabled

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
 */

// TODO save/restore page fault addr? Or just ensure we're reading it soon enough with intrs disabled?
ENTRY(phantom_switch_context)

    push        {r4-r12}

    // Save R15/IP
    str         r15, [ r0, CSTATE_IP ]
    // Save R14/LR
    str         r14, [ r0, CSTATE_LR ]
    // Save R13/SP
    str         r14, [ r0, CSTATE_SP ]

// TODO machine status register!? check for ints enabled?!

    // Load R15/IP
    ldr         r15, [ r1, CSTATE_IP ]
    // Save R14/LR
    ldr         r14, [ r1, CSTATE_LR ]
    // Save R13/SP
    ldr         r14, [ r1, CSTATE_SP ]


    pop         {r4-r12}

    // Unlock (set to zero) given spinlock
    mov         r0, #0
    str         r0, [ r2 ]


    // Return to new thread's saved link register.
    mov         r15, r14


        
#if 0
    /**
     * new thread starts here with
     *   esi = func
     *   edi = arg
     *   ebx = thread struct addr
     */
ENTRY(phantom_thread_trampoline)
    pushl %ebx // tstruct
    pushl %edi // param
    pushl %esi // func addr
    call        EXT(phantom_thread_c_starter)
    hlt // not reached


#endif
