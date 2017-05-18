#include <kernel/interrupts.h>
#include <threads.h>

void
phantom_scheduler_request_soft_irq()
{
    hal_request_softirq(SOFT_IRQ_THREADS);
    //__asm __volatile("int $15");
    panic("No phantom_scheduler_request_soft_irq impl");
}

void
phantom_scheduler_schedule_soft_irq()
{
    hal_request_softirq(SOFT_IRQ_THREADS);
}
