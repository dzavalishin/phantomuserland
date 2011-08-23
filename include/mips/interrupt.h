
#define MIPS_ONCPU_INTERRUPTS 8

void arch_interrupt_enable(int irq);
void arch_interrupt_disable(int irq);
void arch_interrupts_disable_all(void);
