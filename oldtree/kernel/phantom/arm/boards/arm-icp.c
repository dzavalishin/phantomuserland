/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Arm Integrator/CP hardware mappings.
 *
**/

#include <kernel/board.h>


void board_init_early(void)
{
    // On PC do nothing. Suppose we got quite a reasonable state
    // frtom multiboot.
}

void board_init_cpu_management(void)
{
}



void board_init_kernel_timer(void)
{
    icp_timer0_init(100);
}

void board_start_smp(void)
{
    // I'm single-CPU board, sorry.
}



void board_interrupt_enable(int irq)
{
#warning impl
}

void board_interrupt_disable(int irq)
{
#warning impl
}

void board_init_interrupts(void)
{
#warning impl
}

void board_interrupts_disable_all(int irq)
{
#warning impl
}


// -----------------------------------------------------------------------
// stubs
// -----------------------------------------------------------------------


void  paging_device_start_read_rq( void *pdev, void *pager_current_request, void *page_device_io_final_callback )
{
}

void  paging_device_start_write_rq( void *pdev, void *pager_current_request, void *page_device_io_final_callback )
{
}

void init_paging_device()
{
}

int phantom_dev_keyboard_getc(void)
{
    return debug_console_getc();
}

int phantom_scan_console_getc(void)
{
    return debug_console_getc();
}


int phantom_dev_keyboard_get_key()
{
#warning completely wrong!!!
    return debug_console_getc();
}


void driver_isa_vga_putc(int c )
{
    debug_console_putc(c);
}



void rtc_read_tm() {}

long long arch_get_rtc_delta() { return 0LL; }

