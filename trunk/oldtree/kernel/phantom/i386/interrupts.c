/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * ia32 specific interrupts (PIC, 8259 - APIC is elsewhere)
 *
**/

#define DEBUG_MSG_PREFIX "intr-pic"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>
#include <phantom_assert.h>
#include <kernel/stats.h>
#include <hal.h>

#include <kernel/interrupts.h>
#include <kernel/trap.h>

#include <ia32/pio.h>
#include <kernel/bus/isa/pic.h>
#include <queue.h>

#include "../misc.h"





//void (*soft_irq_handler)(struct trap_state *) = def_soft_irq_handler;






// TODO use process_irq()
void
hal_PIC_interrupt_dispatcher(struct trap_state *ts, int mask)
{
    check_global_lock_entry_count();

    unsigned irq = ts->err;

    irq_nest++;
    call_irq_handler(ts,irq);
    irq_nest--;

    check_global_lock_entry_count();
    // now unmask PIC before running softint

    if( ts->err > 7 )
    {
        // Slave
        outb( 0xa1, mask );
    }
    else
    {
        // Master
        outb( 0x21, mask );
    }

    STAT_INC_CNT(STAT_CNT_INTERRUPT);

    if(irq_nest)
        return;

    // Now for soft IRQs
    irq_nest = SOFT_IRQ_DISABLED|SOFT_IRQ_NOT_PENDING;
    hal_softirq_dispatcher(ts);
    ENABLE_SOFT_IRQ();

}




