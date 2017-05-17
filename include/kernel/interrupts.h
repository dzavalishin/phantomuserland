/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * General interrupt engine.
 *
 *
**/

#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#define         SOFT_IRQ_NOT_PENDING    0x80000000
#define         SOFT_IRQ_DISABLED       0x40000000

// TODO this must go to board specific include file for it depends on hw more than on CPU

// How many different interrupts kernel can allocate

#ifdef ARCH_ia32
// Main PIC has 16 inputs, but on APIC 24 interrupts are usually available
#define         MAX_IRQ_COUNT 32
#endif

#ifdef ARCH_amd64
// Main PIC has 16 inputs, but on APIC 24 interrupts are usually available
#define         MAX_IRQ_COUNT 32
#endif

#ifdef ARCH_arm
// Raspberry has 64+32
#define         MAX_IRQ_COUNT 128
#endif

#ifdef ARCH_mips
#define         MAX_IRQ_COUNT 64
#endif

#ifdef ARCH_e2k
#define         MAX_IRQ_COUNT 64
#endif


#ifndef MAX_IRQ_COUNT
#error Unknown architecture
#endif

#define SOFT_IRQ_COUNT 32


#define USE_SOFTIRQ_DISABLE 1

#ifndef ASSEMBLER

// two top bits are 'no softint req' and 'softint disabled'
extern int      irq_nest; 

#define         IN_INTERRUPT()          ((irq_nest & ~(SOFT_IRQ_NOT_PENDING|SOFT_IRQ_DISABLED)))

#define         REQUEST_SOFT_IRQ()      (irq_nest &= ~SOFT_IRQ_NOT_PENDING )
#define         CLEAR_SOFT_IRQ()        (irq_nest |= SOFT_IRQ_NOT_PENDING )

#if USE_SOFTIRQ_DISABLE
#define         ENABLE_SOFT_IRQ()       (irq_nest &= ~SOFT_IRQ_DISABLED)
#define         DISABLE_SOFT_IRQ()      (irq_nest |= SOFT_IRQ_DISABLED)
#define         IS_SOFT_IRQ_DISABLED()  (irq_nest & SOFT_IRQ_DISABLED)
#else
#warning do not select me
#define         ENABLE_SOFT_IRQ()
#define         DISABLE_SOFT_IRQ()
#endif


struct trap_state;
void (*soft_irq_handler)(struct trap_state *);

void hal_softirq_dispatcher(struct trap_state *ts);

// TODO move softint part here
//! Arch independent part - calls IRQ handler
void call_irq_handler(struct trap_state *s, unsigned irq);

//! Arch independent part - general irq process.
//! Calls call_irq_handler, called by board specific IRQ
//! handling code. Does irq_nest, statistics, calls
//! hal_softirq_dispatcher(). 
void process_irq(struct trap_state *ts, int irq);



#endif




#endif // _INTERRUPTS_H



