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

#include <amd64/asm.h>
#include "cpu_state.h"

#define SAVE_CR2 1
#warning CR2?

// called and returns with interrupts disabled
/* void phantom_switch_context(
                            phantom_thread_t *from,
                            phantom_thread_t *to,
                            int *unlock );

  Args are supposed to be in rdi (from), rsi (to), rdx (unlock)

*/
ENTRY(phantom_switch_context)

    movq	(%rsp),%rcx			// IP
    movq	%rcx, CSTATE_EIP(%rdi)
    movq	%rbp, CSTATE_EBP(%rdi)
#if FXSAVE
    fxsave   	      CSTATE_FXSTATE(%rdi)
#endif

    push	%rbx
    push	%r12
    push	%r13
    push	%r14
    push	%r15
#if SAVE_CR2
    movq        %cr2, %rbx
#endif
    pushq       %rbx

    movq	%rsp, CSTATE_ESP(%rdi)

    // saved ok, now load

    movq	CSTATE_ESP(%rsi), %rsp

    popq        %rbx
#if SAVE_CR2
    movq        %rbx, %cr2
#endif
    pop		%r15
    pop		%r14
    pop		%r13
    pop		%r12
    pop		%rbx

#if FXSAVE
    fxrstor     CSTATE_FXSTATE(%rsi)
#endif
    movq        CSTATE_EIP(%rsi), %rcx
    movq        %rcx, (%rsp)			// IP

    movq        CSTATE_EBP(%rsi), %rbp

    // Done, unlock the spinlock given

    movq        %rdx,%rdi			// Lock ptr
    call	EXT(hal_spin_unlock)

    ret


    /**
     * new thread starts here, no args
     * /
ENTRY(phantom_thread_trampoline)
    // c starter has no params now
    //pushq %rbx // tstruct
    //pushq %rdi // param
    //pushq %rsi // func addr
    call        EXT(phantom_thread_c_starter)
    hlt // not reached
*/
/*
ENTRY(phantom_thread_fp_init)

    // save ours
    movl	8(%rsp),%rax
    fxsave	0(%rax)

    // init
    fninit
    // save inited
    movl	16(%rsp),%rax
    fsave	0(%rax)

    // load ours
    movl	8(%rsp),%rax
    fxrstor  0(%rax)

    ret

    */

#if FXSAVE

/* void i386_fxsave(void *fpu_state); */
ENTRY(amd64_fxsave)
	fxsave	(%rdi)
	ret

/* void i386_fxrstor(void *fpu_state); */
ENTRY(amd64_fxrstor)
	fxrstor	(%rdi)
	ret

#endif

