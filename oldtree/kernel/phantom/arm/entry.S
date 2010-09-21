/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM startup code. Not compiled even once. Not sure that it is right at all.
 *
**/



.text
    .code 32

    .global _start
    .func   _start
_start:
    // Vect table
    B           _reset  // reset
    B           _reset  // undef instr
    B           _reset  // soft int
    B           _reset  // prefetch abort
    B           _reset  // data abort
    B           _reset  // reserved
    B           _reset  // irq
    B           _reset  // fiq

    // Multiboot header for any case - need it here?
    .long	0x1BADB002		/* magic */
    .long	0x00000000		/* feature flags */
    .long	0-0x1BADB002-0x00000000	/* checksum */


    .string "Phantom OS\r\n"
    .string "Copyright (C) Dmitry Zavalishin, 2010.\r\n"

    .align 4

_reset:

    // Clear BSS

    LDR         r1, = __bss_start__
    LDR         r2, = __bss_end__
    LDR         r3, #0

1:
    CMP         r1, r2
    STMLTIA     r1!, {r3}
    BLT         1b

    // Set stacks

    MSR         CPSR_c, #(IRQ_MODE | I_BIT | F_BIT)
    LDR         sp, = __irq_stack_top__

    MSR         CPSR_c, #(SVC_MODE | I_BIT | F_BIT)
    LDR         sp, = __svc_stack_top__

    MSR         CPSR_c, #(FIQ_MODE | I_BIT | F_BIT)
    LDR         sp, = __fiq_stack_top__

    MSR         CPSR_c, #(ABT_MODE | I_BIT | F_BIT)
    LDR         sp, = __abt_stack_top__

    MSR         CPSR_c, #(UND_MODE | I_BIT | F_BIT)
    LDR         sp, = __und_stack_top__

    MSR         CPSR_c, #(SYS_MODE | I_BIT | F_BIT)
    LDR         sp, = __cmain_stack_top__


    // Call main

    LDR         r12, = EXT(phantom_multiboot_main)
    MOV         lr, pc
    BX          r12

    // Cause exception - TODO call some kind of hard reboot
    SWI         0xFFFFFF


    .data
    .align 4096 // TODO use page size define

    .bss
    .align 4096 // TODO use page size define

#include "stack.h"

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
    .space	PHANTOM_START_STACK_SIZE


    .endfunc
    .end

