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


// TODO move to $(ARCH)/smp.c

#include <kernel/smp.h>


int GET_CPU_ID(void)
{
    return 0;
/*
        have_apic ?
            ((apic_local_unit->unit_id.r >> 24) & 0xF)
        :
            0
        ;
*/
}

#if HAVE_SMP
# error we're surely not ready for SMP
#endif
