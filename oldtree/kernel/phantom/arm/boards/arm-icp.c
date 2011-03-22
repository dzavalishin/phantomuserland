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
#include <kernel/trap.h>
#include <kernel/interrupts.h>

#include <assert.h>
#include <stdio.h>
#include <arm/memio.h>
#include "arm-icp.h"

#define DEBUG_MSG_PREFIX "board"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10




static int icp_irq_dispatch(struct trap_state *ts);


void board_init_early(void)
{

    // Relocate trap table to address 0
    extern unsigned int _start_of_kernel[];
    unsigned int *atzero = 0;

    unsigned int shift = &_start_of_kernel;

    // Copy branch instructions, correcting (relative) target
    // address by distance we move them to.
    int i;
    for( i = 0; i < 8; i++ )
    {
        atzero[i] = _start_of_kernel[i] + shift;
    }


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

// -----------------------------------------------------------------------
// Interrupts processing
// -----------------------------------------------------------------------


void board_interrupt_enable(int irq)
{
    W32(ICP_PRI_INTERRUPT_IRQ_SET,1<<irq);
}

void board_interrupt_disable(int irq)
{
    W32(ICP_PRI_INTERRUPT_IRQ_CLEAR,1<<irq);
}

void board_init_interrupts(void)
{
    board_interrupts_disable_all();
    phantom_trap_handlers[T_IRQ] = icp_irq_dispatch;

}

void board_interrupts_disable_all(void)
{
    W32(ICP_PRI_INTERRUPT_IRQ_CLEAR,0xFFFFFFFF); // Disable all primary controller interrupts
    W32(ICP_PRI_INTERRUPT_FIQ_CLEAR,0xFFFFFFFF); // Disable all primary controller fast interrupts
    W32(ICP_PRI_INTERRUPT_SOFT_CLEAR,0xFFFFFFFF); // Disable all primary controller soft interrupts

    W32(SIC_INT_ENABLECLR, 0xFFFFFFFF); // Secondary too
    W32(SIC_INT_SOFTCLR, 0xFFFFFFFF);
}

static void process_irq(struct trap_state *ts, int irq)
{
    ts->intno = irq;

    board_interrupt_disable(irq);

    call_irq_handler( ts, irq );

    board_interrupt_enable(irq); // TODO Wrong! Int handler might disable itself! Keep local mask.
}


static int icp_irq_dispatch(struct trap_state *ts)
{
    u_int32_t   irqs;

    while( (irqs = R32(ICP_PRI_INTERRUPT_IRQ_STATUS)) != 0 )
    {
        int nirq = 0;
        while( irqs )
        {
            if( irqs & 0x1 )
                process_irq(ts, nirq);

            irqs >>= 1;
            nirq++;
        }
    }

    return 0; // We're ok
}



// -----------------------------------------------------------------------
// Drivers
// -----------------------------------------------------------------------

// Interrupts on Primary are 0-31, Secondary 32-63, Debug 64-...

// Timers at 0x13000000, but we init 'em separately
// Some ICP stuff at 0x1B000000 - find
// Core control 0x10000000-0x1000003F - todo in early init
// Core IRQ controller @0x10000040-0x1000007F
// Serial Presence Detect memory 0x10000100-0x100001FF
// CP control registers 0xCB000000-0xCBFFFFFF

// Primary IRQ controller 0x14000000 - done
// Sec int controller 0xCA000000 - just disabled


// NB! No network drivers on stage 0!
static isa_probe_t board_drivers[] =
{
/*
    { "GPIO", 		driver_mem_icp_gpio_probe, 	0, 0xC9000000, 0 },
    { "LCD", 		driver_mem_icp_lcd_probe, 	0, 0xC0000000, 22 },

    { "touch",		driver_mem_icp_touch_probe,   	1, 0x1E000000, 28 },
    { "PL041.Audio",   	driver_mem_pl041_audio_probe,   2, 0x1D000000, 25 },
    { "MMC",		driver_mem_icp_mmc_probe,   	2, 0x1C000000, 23 }, // And 24 - how do we give 2 irqs?
    { "PL050.kb",      	driver_mem_pl050_kb_probe,   	1, 0x18000000, 3 },
    { "PL050.ms",      	driver_mem_pl050_ms_probe,   	1, 0x19000000, 4 },

    { "UART0", 		driver_mem_pl011_uart_probe, 	2, 0x16000000, 1 },
    { "UART1", 		driver_mem_pl011_uart_probe, 	2, 0x17000000, 2 },

    { "RTC", 		driver_mem_icp_rtc_probe, 	0, 0x15000000, 8 },

    { "LEDS", 		driver_mem_icp_leds_probe, 	0, 0x1A000000, 0 },

    { "LAN91C111", 	driver_mem_LAN91C111_net_probe, 2, 0xC8000000, 27 },
*/

    // End of list marker
    { 0, 0, 0, 0, 0 },
};



void board_make_driver_map(void)
{
    int id = R32(ICP_IDFIELD);

    if( (id >> 24) != 0x41 )
        SHOW_ERROR( 0, "Board manufacturer is %d, not %d", (id >> 24), 0x41 );

    phantom_register_drivers(board_drivers);
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


int driver_isa_vga_putc(int c )
{
    //debug_console_putc(c);
    return c;
}



void rtc_read_tm() {}

long long arch_get_rtc_delta() { return 0LL; }



void board_fill_memory_map( amap_t *ram_map )
{

    extern char end[];

    int uptokernel = (int)&end;

//    int len = 256*1024*1024;
    int len = 128*1024*1024;
    assert( 0 == amap_modify( ram_map, uptokernel, len-uptokernel, MEM_MAP_HI_RAM) );

	//int start = 0x10000000;
	//len =       0xFFFFFFFF-start;
    //assert( 0 == amap_modify( ram_map, start, len, MEM_MAP_DEV_MEM) );
}





