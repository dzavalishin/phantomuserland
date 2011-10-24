/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM low level tests. 
 *
**/

#include <arm/psr.h>
#include <arm/asm.h>



ENTRY(arm_test_interrupts_integrity)

    push        {r0-r12,r14}

    LDR         r1, = .L_arm_test_interrupts_integrity__sp
    str         r13, [r1]
    
    LDR         r1, = .L_arm_test_interrupts_integrity__lr    
    str         r14, [r1]

    LDR         r1, = .L_arm_test_interrupts_integrity__count
    mov         r0, #0
    str         r0, [r1]

    mov         r0, #0
    mov         r1, #0
    mov         r2, #0
    mov         r3, #0
    mov         r4, #0
    mov         r5, #0
    mov         r6, #0
    mov         r7, #0
    mov         r8, #0
    mov         r9, #0
    mov         r10, #0
    mov         r11, #0
    mov         r12, #0

Lagain:

    cmp         r2, #0
    bne         fail

    cmp         r3, #0
    bne         fail

    cmp         r4, #0
    bne         fail

    cmp         r5, #0
    bne         fail

    cmp         r6, #0
    bne         fail

    cmp         r7, #0
    bne         fail

    cmp         r8, #0
    bne         fail

    cmp         r9, #0
    bne         fail

    cmp         r10, #0
    bne         fail

    cmp         r11, #0
    bne         fail

    cmp         r12, #0
    bne         fail

    // check 'em last to increase chance of being overwritten - we reload em with zero in loop code below
    cmp         r0, #0
    bne         fail

    cmp         r1, #0
    bne         fail



    ldr         r0, = .L_arm_test_interrupts_integrity__sp
    ldr         r0, [r0]
    cmp         r13, r0
    bne         fail

    ldr         r0, = .L_arm_test_interrupts_integrity__lr
    ldr         r0, [r0]
    cmp         r14, r0
    bne         fail


    // loop
    ldr         r0, = .L_arm_test_interrupts_integrity__count
    ldr         r0, [r0]
    sub         r0, r0, #1
    cmp         r0, #0
    beq         finita
    ldr         r1, = .L_arm_test_interrupts_integrity__count
    str         r0, [r1]

    mov         r0, #0
    mov         r1, #0
    b           Lagain
    
finita:
    pop        {r0-r12,r14}

    RET


fail:

    adr		r0, .LregOverwritten
    mov         lr, pc // attempt to return will bring here
    b		_C_LABEL(panic)
    /* NOTEACHED */





    .data
    .align 12 // TODO use page size define

.LregOverwritten:
	.asciz	"register overwrite?!"


.L_arm_test_interrupts_integrity__sp:
	.word	0

.L_arm_test_interrupts_integrity__lr:
	.word	0

.L_arm_test_interrupts_integrity__count:
	.word	0
