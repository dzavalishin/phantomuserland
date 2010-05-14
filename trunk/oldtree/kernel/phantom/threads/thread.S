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

#include <mach/asm.h>
#include "cpu_state.h"

#define KERNEL 0

// called and returns with interrupts disabled
/* void phantom_switch_context(
                            phantom_thread_t *from,
                            phantom_thread_t *to,
                            int *unlock );
*/
ENTRY(phantom_switch_context)

    movl	4(%esp),%eax			// sw from (store to)

    movl	(%esp),%ecx			// IP
    movl	%ecx, CSTATE_EIP(%eax)
    movl	%ebp, CSTATE_EBP(%eax)
#if FXSAVE
    fxsave   	      CSTATE_FXSTATE(%eax)
#endif
    // we saved ebp, can use it. 
    movl        %esp, %ebp
    // params are on bp now

    pushl       %ebx
    pushl       %edi
    pushl       %esi
#if KERNEL
    //pushl    	%cr2
    movl        %cr2, %esi
#endif
    pushl       %esi

    movl	%esp, CSTATE_ESP(%eax)

    // saved ok, now load 

    movl	8(%ebp),%eax			// sw to (load from)

    movl	CSTATE_ESP(%eax), %esp

    popl        %esi
#if KERNEL
    movl        %esi, %cr2
    //popl        %cr2
#endif
    popl        %esi
    popl        %edi
    popl        %ebx

#if FXSAVE
    fxrstor     CSTATE_FXSTATE(%eax)
#endif
    movl	CSTATE_EIP(%eax), %ecx
    movl	%ecx, (%esp)			// IP

    // now move original params ptr to ecx, as we will use and restore ebp
    movl        %ebp, %ecx

    movl	CSTATE_EBP(%eax), %ebp

    // Done, unlock the spinlock given

    movl	12(%ecx),%ecx			// Lock ptr
    xorl        %eax,%eax
    xchgl   	%eax,(%ecx)                     // Unlock it

    // now we have in eax (which is int ret val) old lock value

    ret
        

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

/*
ENTRY(phantom_thread_fp_init)

    // save ours
    movl	4(%esp),%eax
    fxsave	0(%eax)

    // init 
    fninit
    // save inited
    movl	8(%esp),%eax
    fsave	0(%eax)

    // load ours
    movl	4(%esp),%eax
    fxrstor  0(%eax)

    ret

    */
#if 0

/* void i386_fsave(void *fpu_state); * /
FUNCTION(i386_fsave):
	movl	4(%esp), %eax
	fsave	(%eax)
	ret
        */

/* void i386_fxsave(void *fpu_state); */
ENTRY(i386_fxsave)
	movl	4(%esp), %eax
	fxsave	(%eax)
	ret

/* void i386_frstor(void *fpu_state); * /
FUNCTION(i386_frstor):
	movl	4(%esp), %eax
	frstor	(%eax)
	ret
        */

/* void i386_fxrstor(void *fpu_state); */
ENTRY(i386_fxrstor)
	movl	4(%esp), %eax
	fxrstor	(%eax)
	ret

#endif

