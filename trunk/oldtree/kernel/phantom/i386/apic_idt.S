/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * APIC IDT table and asm interrupt entry/exit code.
 *
**/


#include <mach/asm.h>
//#include <i386/isa/pic_regs.h>

#include <kernel/interrupts.h>


// TODO EBX is not really used

#define APIC_INTERRUPT(irq)			 \
    .data	2				;\
    .long	0f				;\
    .text					;\
    P2ALIGN(TEXT_ALIGN)				;\
0:						;\
    pushl	$(irq)	                /* err code */	;\
    pushl	$(irq)	  		/* trap num */	;\
    pusha                                       ;\
    movl	$(irq),%ebx		/* vector */      ;\
    jmp 	apic_int_start


	.data	2
DATA(apic_entry_table)
	.text
APIC_INTERRUPT(0x0)
APIC_INTERRUPT(0x1)
APIC_INTERRUPT(0x2)
APIC_INTERRUPT(0x3)
APIC_INTERRUPT(0x4)
APIC_INTERRUPT(0x5)
APIC_INTERRUPT(0x6)
APIC_INTERRUPT(0x7)
APIC_INTERRUPT(0x8)
APIC_INTERRUPT(0x9)
APIC_INTERRUPT(0xA)
APIC_INTERRUPT(0xB)
APIC_INTERRUPT(0xC)
APIC_INTERRUPT(0xD)
APIC_INTERRUPT(0xE)
APIC_INTERRUPT(0xF)
APIC_INTERRUPT(0x10)
APIC_INTERRUPT(0x11)
APIC_INTERRUPT(0x12)
APIC_INTERRUPT(0x13)
APIC_INTERRUPT(0x14)
APIC_INTERRUPT(0x15)
APIC_INTERRUPT(0x16)
APIC_INTERRUPT(0x17)
APIC_INTERRUPT(0x18)
APIC_INTERRUPT(0x19)
APIC_INTERRUPT(0x1A)
APIC_INTERRUPT(0x1B)
APIC_INTERRUPT(0x1C)
APIC_INTERRUPT(0x1D)
APIC_INTERRUPT(0x1E)
APIC_INTERRUPT(0x1F)




    P2ALIGN(TEXT_ALIGN)
apic_int_start:
    pushl	%ds
    pushl	%es
    pushl	%fs
    pushl	%gs

    // store stack ptr (struct trap_state *) to ESI to use as parameter
    movl 	%esp, %esi

    movw	%ss,%dx
    movw	%dx,%ds
    movw	%dx,%es

    cld
    /* Call the interrupt handler with the trap frame as a parameter */
    pushl       %ebx    // vector
    pushl	%esi    // trap state
    call        EXT(hal_APIC_interrupt_dispatcher)
    popl	%edx
    popl        %ebx    

    /* Return from the interrupt */
    popl	%gs
    popl	%fs
    popl	%es
    popl	%ds
    popa
    addl	$4*2,%esp	/* Pop trap number and error code */
    iret

