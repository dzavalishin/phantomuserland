#include <kernel/interrupts.h>
#include <threads.h>

void
phantom_scheduler_request_soft_irq()
{
    hal_request_softirq(SOFT_IRQ_THREADS);
    __asm __volatile("swi 0xFFF"); // TODO who catches that 0xFFF?
}

void
phantom_scheduler_schedule_soft_irq()
{
    hal_request_softirq(SOFT_IRQ_THREADS);
}
