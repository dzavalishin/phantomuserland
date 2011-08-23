/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS machdep cpu/system init
 *
**/

#define DEBUG_MSG_PREFIX "arch.init"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/init.h>
#include <mips/cp0_regs.h>


static void timer_interrupt( void *a );
static int timer_compare_delta = 1000; // Default value of 2000 instructions
static int timer_compare_value = 0;

static int usec_per_tick = 100000; // 100 Hz = HZ*1000

void arch_init_early(void)
{


}

void board_init_kernel_timer(void)
{
    // dies
#if 1
    int timer_irq = 7;

    // On-chip timer init
    assert(!hal_irq_alloc( timer_irq, timer_interrupt, 0, HAL_IRQ_SHAREABLE ));

    mips_write_cp0_compare( timer_compare_value );
    mips_write_cp0_count( 0 );
#endif
}


static void timer_interrupt( void *a )
{
    (void) a;

    timer_compare_value += timer_compare_delta;
    mips_write_cp0_compare( timer_compare_value );

    hal_time_tick(usec_per_tick);
}




