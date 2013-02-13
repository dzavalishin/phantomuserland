/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Default QEMU MIPS machine (-M mips) hardware mappings.
 *
**/

#include <kernel/board.h>
#include <kernel/trap.h>
#include <kernel/interrupts.h>
#include <kernel/driver.h>
#include <kernel/stats.h>

#include <hal.h>
#include <assert.h>
#include <stdio.h>

#include <mips/cp0_regs.h>
#include <mips/interrupt.h>

#define DEBUG_MSG_PREFIX "board"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

char board_name[] = "QEMU_M_mips";

void board_init_early(void)
{
}

void board_init_cpu_management(void)
{
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
    //assert_interrupts_disabled();
    int ie = hal_save_cli();

    if(irq < MIPS_ONCPU_INTERRUPTS)
        arch_interrupt_enable(irq);
    else
    {
#warning todo
        SHOW_ERROR( 0, "unimpl irq %d", irq );
    }

    if(ie) hal_sti();
}

void board_interrupt_disable(int irq)
{
    int ie = hal_save_cli();

    if(irq < MIPS_ONCPU_INTERRUPTS)
        arch_interrupt_disable(irq);
    else
    {
        SHOW_ERROR( 0, "unimpl irq %d", irq );
    }

    if(ie) hal_sti();
}


void board_init_interrupts(void)
{
    board_interrupts_disable_all();
}

void board_interrupts_disable_all(void)
{
    int ie = hal_save_cli();

    arch_interrupts_disable_all();

    if(ie) hal_sti();
}






// -----------------------------------------------------------------------
// Drivers
// -----------------------------------------------------------------------


// NB! No network drivers on stage 0!
static isa_probe_t board_drivers[] =
{
#if 0
    { "COM1",		driver_isa_com_probe,   2, BOARD_ISA_IO|0x3F8u, 4 }, // Interrupts are wrong
    { "COM2",		driver_isa_com_probe,   2, BOARD_ISA_IO|0x2F8u, 3 },

#if HAVE_NET
    { "NE2000", 	driver_isa_ne2000_probe,1, BOARD_ISA_IO|0x280, 9 },
    { "NE2000", 	driver_isa_ne2000_probe,1, BOARD_ISA_IO|0x300, 9 },
    { "NE2000", 	driver_isa_ne2000_probe,1, BOARD_ISA_IO|0x320, 9 },
#endif
#endif


    // End of list marker
    { 0, 0, 0, 0, 0 },
};



void board_make_driver_map(void)
{
    phantom_register_drivers(board_drivers);
}


// -----------------------------------------------------------------------
// stubs
// -----------------------------------------------------------------------

#warning stubs!
void  paging_device_start_read_rq( void *pdev, void *pager_current_request, void *page_device_io_final_callback )
{
    (void) pdev;
    (void) pager_current_request;
    (void) page_device_io_final_callback;
}

void  paging_device_start_write_rq( void *pdev, void *pager_current_request, void *page_device_io_final_callback )
{
    (void) pdev;
    (void) pager_current_request;
    (void) page_device_io_final_callback;
}

void init_paging_device(void)
{
}



int phantom_dev_keyboard_get_key()
{
//#warning completely wrong!!!
    //    return debug_console_getc();
    while(1)
        hal_sleep_msec(10000);
}


//int driver_isa_vga_putc(int c )
int board_boot_console_putc( int c )
{
    //debug_console_putc(c);
    return c;
}



void rtc_read_tm() {}

long long arch_get_rtc_delta() { return 0LL; }



void board_fill_memory_map( amap_t *ram_map )
{
    extern char end[];

    addr_t uptokernel = kvtophys(&end);

    int len = 256*1024*1024; // Hardcode 256M of RAM
    assert( 0 == amap_modify( ram_map, uptokernel, len-uptokernel, MEM_MAP_HI_RAM) );

    assert( 0 == amap_modify( ram_map, kvtophys(BOARD_ISA_IO),  BOARD_ISA_IO_LEN,  MEM_MAP_DEV_MEM) );
    assert( 0 == amap_modify( ram_map, kvtophys(BOARD_ISA_MEM), BOARD_ISA_MEM_LEN, MEM_MAP_DEV_MEM) );
}




