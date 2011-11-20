/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Interrupt vector for vectored IRQ controller (PL192)
 *
**/


#include <arm/asm.h>



// 4 instructions per src
#define INTERRUPT(irq)				\
    push        {r0-r12}        ;\
    ldr         r1, = irq       ;\
    b           allintrs        ;\
    nop


	.data	2
DATA(int_entry_table)
	.text
INTERRUPT(0)
INTERRUPT(1)
INTERRUPT(2)
INTERRUPT(3)
INTERRUPT(4)
INTERRUPT(5)
INTERRUPT(6)
INTERRUPT(7)
INTERRUPT(8)
INTERRUPT(9)
INTERRUPT(10)
INTERRUPT(11)
INTERRUPT(12)
INTERRUPT(13)
INTERRUPT(14)
INTERRUPT(15)

INTERRUPT(16+0)
INTERRUPT(16+1)
INTERRUPT(16+2)
INTERRUPT(16+3)
INTERRUPT(16+4)
INTERRUPT(16+5)
INTERRUPT(16+6)
INTERRUPT(16+7)
INTERRUPT(16+8)
INTERRUPT(16+9)
INTERRUPT(16+10)
INTERRUPT(16+11)
INTERRUPT(16+12)
INTERRUPT(16+13)
INTERRUPT(16+14)
INTERRUPT(16+15)

allintrs:

    call EXT(panic);

