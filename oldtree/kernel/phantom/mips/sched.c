#include <kernel/interrupts.h>
#include <threads.h>

void
phantom_scheduler_request_soft_irq(void)
{
#warning implement
/*
    hal_request_softirq(SOFT_IRQ_THREADS);
    //__asm __volatile("swi 0xFFF"); // TODO who catches that 0xFFF?
    //board_sched_cause_soft_irq();

    // We call phantom_scheduler_soft_interrupt directly, and usually it is called in softirq,
    // so exepects softirqs to be enabled. Fulfill.
    //ENABLE_SOFT_IRQ();
    int ie = hal_save_cli();
    phantom_scheduler_soft_interrupt();
    ENABLE_SOFT_IRQ();
    if(ie) hal_sti();
*/
}

void
phantom_scheduler_schedule_soft_irq(void)
{
    hal_request_softirq(SOFT_IRQ_THREADS);
}

#if HAVE_SMP
#error no SMP support for arm target
#endif

#ifndef HAVE_SMP
#warning no HAVE_SMP definition
#endif

int GET_CPU_ID(void)
{
    return 0;
}
