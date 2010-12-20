#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#define         SOFT_IRQ_NOT_PENDING 	0x80000000
#define         SOFT_IRQ_DISABLED 	0x40000000

// Main PIC has 16 inputs
#define PIC_IRQ_COUNT 16

#define SOFT_IRQ_COUNT 32


#define USE_SOFTIRQ_DISABLE 1

#ifndef ASSEMBLER

// two top bits are 'no softint req' and 'softint disabled'
extern int      irq_nest; 


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

// used in apic
void call_irq_handler(struct trap_state *s, unsigned irq);


#endif




#endif // _INTERRUPTS_H



