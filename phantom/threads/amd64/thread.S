/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
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
*/
//.def CNAME(phantom_switch_context)
ENTRY(phantom_switch_context)

    movq	8(%rsp),%rax			// sw from (store to)

    movq	(%rsp),%rcx			// IP
    movq	%rcx, CSTATE_EIP(%rax)
    movq	%rbp, CSTATE_EBP(%rax)
#if FXSAVE
    fxsave   	      CSTATE_FXSTATE(%rax)
#endif
    // we saved rbp, can use it. 
    movq        %rsp, %rbp
    // params are on bp now

    pushq       %rbx
    pushq       %rdi
    pushq       %rsi
#if SAVE_CR2
    movq        %cr2, %rsi
#endif
    pushq       %rsi

    movq	%rsp, CSTATE_ESP(%rax)

    // saved ok, now load 

    movq	16(%rbp),%rax			// sw to (load from)

    movq	CSTATE_ESP(%rax), %rsp

    popq        %rsi
#if SAVE_CR2
    movq        %rsi, %cr2
#endif
    popq        %rsi
    popq        %rdi
    popq        %rbx

#if FXSAVE
    fxrstor     CSTATE_FXSTATE(%rax)
#endif
    movq        CSTATE_EIP(%rax), %rcx
    movq        %rcx, (%rsp)			// IP

    // now move original params ptr to rcx, as we will use and restore rbp
    movq        %rbp, %rcx

    movq        CSTATE_EBP(%rax), %rbp

    // Done, unlock the spinlock given

    movq        24(%rcx),%rcx			// Lock ptr
    pushq       %rcx
    call	EXT(hal_spin_unlock)
    popq        %rcx


    ret
//.endef


    /**
     * new thread starts here with
     *   rsi = func
     *   rdi = arg
     *   rbx = thread struct addr
     */
ENTRY(phantom_thread_trampoline)
    pushq %rbx // tstruct
    pushq %rdi // param
    pushq %rsi // func addr
    call        EXT(phantom_thread_c_starter)
    hlt // not reached

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
//#if 1

/* void i386_fsave(void *fpu_state); * /
FUNCTION(i386_fsave):
	movq	8(%rsp), %rax
	fsave	(%rax)
	ret
        */

/* void i386_fxsave(void *fpu_state); */
ENTRY(i386_fxsave)
	movq	8(%rsp), %rax
	fxsave	(%rax)
	ret

/* void i386_frstor(void *fpu_state); * /
FUNCTION(i386_frstor):
	movq	8(%rsp), %rax
	frstor	(%rax)
	ret
        */

/* void i386_fxrstor(void *fpu_state); */
ENTRY(i386_fxrstor)
	movq	8(%rsp), %rax
	fxrstor	(%rax)
	ret

#endif

