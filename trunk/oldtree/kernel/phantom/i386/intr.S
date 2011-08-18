/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * 8259 IDT table and asm interrupt entry/exit code.
 *
**/


#include <ia32/asm.h>
#include <i386/isa/pic_regs.h>

#include <kernel/interrupts.h>

// TODO EBX is not really used

#define INTERRUPT(irq,line, label)				\
    .data	2				;\
    .long	0f				;\
    .text					;\
    P2ALIGN(TEXT_ALIGN)				;\
0:						;\
    pushl	$(irq)	                  /* err code */	;\
    pushl	$PIC_VECTBASE+(irq)	  /* trap num */	;\
    pusha                                       ;\
    movl	$(irq),%ebx		  /* vector */      ;\
    movb	$1 << line,%dl		  /* mask */      ;\
    jmp	label

    //jmp	EXT(label)

	.data	2
DATA(int_entry_table)
	.text
INTERRUPT(0,0,imaster)
INTERRUPT(1,1,imaster)
INTERRUPT(2,2,imaster)
INTERRUPT(3,3,imaster)
INTERRUPT(4,4,imaster)
INTERRUPT(5,5,imaster)
INTERRUPT(6,6,imaster)
INTERRUPT(7,7,imaster)
INTERRUPT(8,0,islave)
INTERRUPT(9,1,islave)
INTERRUPT(10,2,islave)
INTERRUPT(11,3,islave)
INTERRUPT(12,4,islave)
INTERRUPT(13,5,islave)
INTERRUPT(14,6,islave)
INTERRUPT(15,7,islave)

/* XXX handle NMI - at least print a warning like Linux does.  */



#if 1


// On enter EBX has IRQ num (0-15), DL has mask (8 bits, 1 in our line's bit)
    P2ALIGN(TEXT_ALIGN)
imaster:

    // Get and store the current master PIC mask (bl)
    inb		$0x21,%al
    movb	%al,%bl

    /* Mask the interrupting IRQ */
    orb		%dl,%al
    outb	%al,$0x21

    /* Acknowledge the interrupt */
    movb	$0x20,%al
    outb	%al,$0x20

    // Save the rest of the standard trap frame
    pushl	%ds
    pushl	%es
    pushl	%fs
    pushl	%gs

    // store stack ptr (struct trap_state *) to ESI to use as parameter
    movl %esp, %esi

    //call call_handler

    // ...to unmask the PIC
    //movb	%bl,%al
    //outb	%al,$0x21

    //jmp finish

    jmp call_handler


// On enter EBX has IRQ num (0-15), DL has mask (8 bits, 1 in our line's bit)
    P2ALIGN(TEXT_ALIGN)
islave:

    /* Save the current slave PIC mask (bl) */
    inb		$0xa1,%al
    movb	%al,%bl

    /* Mask the interrupting IRQ */
    orb		%dl,%al
    outb	%al,$0xa1

    // Ack master & slave
    movb	$0x20,%al
    outb	%al,$0x20
    outb	%al,$0xa0

    // Save the rest of the standard trap frame
    pushl	%ds
    pushl	%es
    pushl	%fs
    pushl	%gs

    // store stack ptr (struct trap_state *) to ESI to use as parameter
    movl %esp, %esi

    //call call_handler

    // moved to C
    // ...to unmask the PIC
    //movb	%bl,%al
    //outb	%al,$0xa1

    //jmp finish

    jmp call_handler


call_handler:

    // Load the kernel's segment registers
    movw	%ss,%dx
    movw	%dx,%ds
    movw	%dx,%es

    cld
    /* Call the interrupt handler with the trap frame as a parameter */
    pushl       %ebx    // mask
    pushl	%esi    // trap state
    call        EXT(hal_PIC_interrupt_dispatcher)
    popl	%edx
    popl        %ebx    

//    ret




//finish:

    /* Return from the interrupt */
    popl	%gs
    popl	%fs
    popl	%es
    popl	%ds
    popa
    addl	$4*2,%esp	/* Pop trap number and error code */
    iret

#endif





























// to be rewritten
#if 0
/*
 * All interrupts enter here.
 * old %eax on stack; interrupt number in %eax.
 */
ENTRY(all_intrs)
	pushl	%ecx			/* save registers */
	pushl	%edx
	cld				/* clear direction flag */

	cmpl	%ss:EXT(int_stack_high),%esp /* on an interrupt stack? */
	jb	int_from_intstack	/* if not: */

	pushl	%ds			/* save segment registers */
	pushl	%es
	mov	%ss,%dx			/* switch to kernel segments */
	mov	%dx,%ds
	mov	%dx,%es

	CPU_NUMBER(%edx)

	movl	CX(EXT(int_stack_top),%edx),%ecx
	xchgl	%ecx,%esp		/* switch to interrupt stack */

#if	STAT_TIME
	pushl	%ecx			/* save pointer to old stack */
#else
	pushl	%ebx			/* save %ebx - out of the way */
					/* so stack looks the same */
	pushl	%ecx			/* save pointer to old stack */
	TIME_INT_ENTRY			/* do timing */
#endif

	call	EXT(interrupt)		/* call generic interrupt routine */

	.globl	EXT(return_to_iret)
LEXT(return_to_iret)			/* ( label for kdb_kintr and hardclock) */

	CPU_NUMBER(%edx)
#if	STAT_TIME
#else
	TIME_INT_EXIT			/* do timing */
	movl	4(%esp),%ebx		/* restore the extra reg we saved */
#endif

	popl	%esp			/* switch back to old stack */

	testl	$(EFL_VM),I_EFL(%esp)	/* if in V86 */
	jnz	0f			/* or */
	testb	$3,I_CS(%esp)		/* user mode, */
	jz	1f			/* check for ASTs */
0:
	cmpl	$0,CX(EXT(need_ast),%edx)
	jnz	ast_from_interrupt	/* take it if so */
1:
	pop	%es			/* restore segment regs */
	pop	%ds
	pop	%edx
	pop	%ecx
	pop	%eax
	iret				/* return to caller */

int_from_intstack:
	call	EXT(interrupt)		/* call interrupt routine */
_return_to_iret_i:			/* ( label for kdb_kintr) */
	pop	%edx			/* must have been on kernel segs */
	pop	%ecx
	pop	%eax			/* no ASTs */
	iret

/*
 *	Take an AST from an interrupt.
 *	On PCB stack.
 * sp->	es	-> edx
 *	ds	-> ecx
 *	edx	-> eax
 *	ecx	-> trapno
 *	eax	-> code
 *	eip
 *	cs
 *	efl
 *	esp
 *	ss
 */
ast_from_interrupt:
	pop	%es			/* restore all registers ... */
	pop	%ds
	popl	%edx
	popl	%ecx
	popl	%eax
	pushl	$0			/* zero code */
	pushl	$0			/* zero trap number */
	pusha				/* save general registers */
	push	%ds			/* save segment registers */
	push	%es
	push	%fs
	push	%gs
	mov	%ss,%dx			/* switch to kernel segments */
	mov	%dx,%ds
	mov	%dx,%es

	CPU_NUMBER(%edx)
	TIME_TRAP_UENTRY

	movl	CX(EXT(kernel_stack),%edx),%esp
					/* switch to kernel stack */
	call	EXT(i386_astintr)	/* take the AST */
	popl	%esp			/* back to PCB stack */
	jmp	_return_from_trap	/* return */

#endif


















#if 0



    P2ALIGN(TEXT_ALIGN)
imaster:

    // Get and store the current master PIC mask (bl)
    inb		$0x21,%al
    movb	%al,%bl

    /* Mask the interrupting IRQ */
    orb		%dl,%al
    outb	%al,$0x21

    /* Acknowledge the interrupt */
    movb	$0x20,%al
    outb	%al,$0x20

    // Save the rest of the standard trap frame
    pushl	%ds
    pushl	%es
    pushl	%fs
    pushl	%gs

    // store stack ptr (struct trap_state *) to ESI to use as parameter
    movl %esp, %esi

    pushl       %ebx    // GCC saves it, but who knows. :)
    call call_handler
    popl        %ebx    // We keep mask there

    // ...to unmask the PIC
    movb	%bl,%al
    outb	%al,$0x21

    jmp finish


    P2ALIGN(TEXT_ALIGN)
islave:

    /* Save the current slave PIC mask (bl) */
    inb		$0xa1,%al
    movb	%al,%bl

    /* Mask the interrupting IRQ */
    orb	%dl,%al
    outb	%al,$0xa1

    // Ack master & slave
    movb	$0x20,%al
    outb	%al,$0x20
    outb	%al,$0xa0

    // Save the rest of the standard trap frame
    pushl	%ds
    pushl	%es
    pushl	%fs
    pushl	%gs

    // store stack ptr (struct trap_state *) to ESI to use as parameter
    movl %esp, %esi

    pushl       %ebx    // GCC saves it, but who knows. :)
    call call_handler
    popl        %ebx    // We keep mask there

// move to C
    // ...to unmask the PIC
    movb	%bl,%al
    outb	%al,$0xa1

    jmp finish


call_handler:

    // Load the kernel's segment registers
    movw	%ss,%dx
    movw	%dx,%ds
    movw	%dx,%es

// move to C
    // Increment the hardware interrupt nesting counter
    incl	EXT(irq_nest)
    // Load the handler pointer
    movl	EXT(pic_irq_handlers)(,%ebx,4),%edi

    cld
    /* Call the interrupt handler with the trap frame as a parameter */
    pushl	%esi
    call	*%edi
    popl	%edx

    ret




finish:
// move to C

    /* Decrement the nesting counter and check for software interrupts */
    decl	EXT(irq_nest)
    jnz	1f
	
    movl	$SOFT_IRQ_NOT_PENDING|SOFT_IRQ_DISABLED,EXT(irq_nest)
    sti
    pushl	%esi
    call    	EXT(hal_softirq_handler)
    popl	%eax
    cli
    andl	$SOFT_IRQ_NOT_PENDING,EXT(irq_nest)
1:

    /* Return from the interrupt */
    popl	%gs
    popl	%fs
    popl	%es
    popl	%ds
    popa
    addl	$4*2,%esp	/* Pop trap number and error code */
    iret

#endif




#if 0
/*
 * The "inb" instructions are not needed, but seem to change the timings
 * a bit - without them it seems that the harddisk driver won't work on
 * all hardware. Arghh.
 */
#define ACK_FIRST(mask) \
	"inb $0x21,%al\n\t" \
	"jmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:\torb $" #mask ","SYMBOL_NAME_STR(cache_21)"\n\t" \
	"movb "SYMBOL_NAME_STR(cache_21)",%al\n\t" \
	"outb %al,$0x21\n\t" \
	"jmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:\tmovb $0x20,%al\n\t" \
	"outb %al,$0x20\n\t"

#define ACK_SECOND(mask) \
	"inb $0xA1,%al\n\t" \
	"jmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:\torb $" #mask ","SYMBOL_NAME_STR(cache_A1)"\n\t" \
	"movb "SYMBOL_NAME_STR(cache_A1)",%al\n\t" \
	"outb %al,$0xA1\n\t" \
	"jmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:\tmovb $0x20,%al\n\t" \
	"outb %al,$0xA0\n\t" \
	"jmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:\toutb %al,$0x20\n\t"

#define UNBLK_FIRST(mask) \
	"inb $0x21,%al\n\t" \
	"jmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:\tandb $~(" #mask "),"SYMBOL_NAME_STR(cache_21)"\n\t" \
	"movb "SYMBOL_NAME_STR(cache_21)",%al\n\t" \
	"outb %al,$0x21\n\t"

#define UNBLK_SECOND(mask) \
	"inb $0xA1,%al\n\t" \
	"jmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:\tandb $~(" #mask "),"SYMBOL_NAME_STR(cache_A1)"\n\t" \
	"movb "SYMBOL_NAME_STR(cache_A1)",%al\n\t" \
	"outb %al,$0xA1\n\t"

#endif


