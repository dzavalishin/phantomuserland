/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * QEMU mipssim machine (-M mipssim) hardware mappings.
 *
 * Unfinished! Wrong! Move parts to arch dir?
 *
**/

#include <kernel/board.h>
#include <kernel/trap.h>
#include <kernel/interrupts.h>
#include <kernel/driver.h>
#include <kernel/stats.h>
#include <kernel/vm.h>

#include <hal.h>
#include <assert.h>
#include <stdio.h>

#include <mips/cp0_regs.h>
#include <mips/interrupt.h>
#include <mips/arch/board-mips-mipssim-defs.h>

#define DEBUG_MSG_PREFIX "board"
#include <debug_ext.h>
#define debug_level_flow 7
#define debug_level_error 10
#define debug_level_info 10

char board_name[] = "mipssim";

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



int mips_irq_dispatch(struct trap_state *ts, u_int32_t pending)
{
    unsigned mask = mips_read_cp0_status();
    mask >>= 8;
    mask &= 0xFF;

    SHOW_FLOW( 8, "irq pending %x mask %x", pending, mask );

    pending &= mask;

    // Have software IRQ requests? Clear 'em BEFORE servicing,
    // or they'll fire again as soon as interrupts are open
    if( pending & 0x3 )
    {
        int ie = hal_save_cli();

        unsigned int cause = mips_read_cp0_cause();
        cause &= ~(0x3 << 8); // reset software irq 0 & 1
        mips_write_cp0_cause( cause );

        if(ie) hal_sti();
    }


    u_int32_t   irqs = pending;

    int nirq = 0;
    while( irqs )
    {
        if( irqs & 0x1 )
            process_irq(ts, nirq);

        irqs >>= 1;
        nirq++;
    }


    return 0; // We're ok
}





// -----------------------------------------------------------------------
// Drivers
// -----------------------------------------------------------------------

phantom_device_t * driver_isa_mipsnet_probe( int port, int irq, int stage );

// NB! No network drivers on stage 0!
static isa_probe_t board_drivers[] =
{
#if 1
//    { "COM1",		driver_isa_com_probe,   2, BOARD_ISA_IO|0x3F8u, 4 },

#if HAVE_NET
    { "MipsNet", 	driver_isa_mipsnet_probe,2, 0x4200, 2 },

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
#if 0
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
#endif


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

    assert( 0 == amap_modify( ram_map, kvtophys(BOARD_ISA_IO), BOARD_ISA_IO_LEN, MEM_MAP_DEV_MEM) );
}




