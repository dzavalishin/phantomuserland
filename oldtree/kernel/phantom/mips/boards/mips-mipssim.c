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

#define DEBUG_MSG_PREFIX "board"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

char board_name[] = "mipssim";

//static char * symtab_getname( void *addr ) { (void) addr; return "?"; }


//static int icp_irq_dispatch(struct trap_state *ts);
//static void sched_soft_int( void *a );

void board_init_early(void)
{

    // TODO wrong place - must be in arch code
    //phantom_symtab_getname = symtab_getname;
}

void board_init_cpu_management(void)
{
}


/* in arch
void board_init_kernel_timer(void)
{
    // mc146818rtc irq 8??
} */

void board_start_smp(void)
{
    // I'm single-CPU board, sorry.
}

// -----------------------------------------------------------------------
// Arm -mpoke-function-name
// -----------------------------------------------------------------------

/*
static char * symtab_getname( void *addr )
{
}
*/

// -----------------------------------------------------------------------
// Interrupts processing
// -----------------------------------------------------------------------

#define MIPS_ONCPU_INTERRUPTS 8


void board_interrupt_enable(int irq)
{
    //assert_interrupts_disabled();
    int ie = hal_save_cli();

    if(irq < MIPS_ONCPU_INTERRUPTS)
    {
        // irq mask in 15:8
        unsigned mask = mips_read_cp0_status();
        mask |= 1 << (irq+8);
        mips_write_cp0_status( mask );
    }
    else
    {
#warning todo
        SHOW_ERROR( 0, "unimpl irq %d", irq );
    }

    if(ie) hal_sti();
}

void board_interrupt_disable(int irq)
{
    //assert_interrupts_disabled();
    int ie = hal_save_cli();

    if(irq < MIPS_ONCPU_INTERRUPTS)
    {
        // irq mask in 15:8
        unsigned mask = mips_read_cp0_status();
        mask &= ~(1 << (irq+8));
        mips_write_cp0_status( mask );
    }
    else
    {
        SHOW_ERROR( 0, "unimpl irq %d", irq );
    }

    if(ie) hal_sti();
}

void board_init_interrupts(void)
{
    board_interrupts_disable_all();
    //phantom_trap_handlers[T_IRQ] = icp_irq_dispatch;
#warning todo
}

void board_interrupts_disable_all(void)
{
    int ie = hal_save_cli();

    unsigned mask = mips_read_cp0_status();
    mask &= ~(0xFF << 8);
    mips_write_cp0_status( mask );

    if(ie) hal_sti();
}

// TODO this seems to be pretty arch indep?
static void process_irq(struct trap_state *ts, int irq)
{
    (void) ts;
    (void) irq;

    ts->intno = irq;

    board_interrupt_disable(irq);

    irq_nest++;
    call_irq_handler( ts, irq );
    irq_nest--;

    board_interrupt_enable(irq); // TODO Wrong! Int handler might disable itself! Keep local mask.

    STAT_INC_CNT(STAT_CNT_INTERRUPT);

    if(irq_nest)
        return;

    // Now for soft IRQs
    irq_nest = SOFT_IRQ_DISABLED|SOFT_IRQ_NOT_PENDING;
    hal_softirq_dispatcher(ts);
    ENABLE_SOFT_IRQ();

}


int mips_irq_dispatch(struct trap_state *ts, u_int32_t pending)
{
    unsigned mask = mips_read_cp0_status();
    mask >>= 8;
    mask &= 0xFF;

    pending &= mask;

    u_int32_t   irqs;

    while( (irqs = pending) != 0 )
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


void board_sched_cause_soft_irq(void)
{
    //phantom_scheduler_soft_interrupt();

#warning set soft int bit 0
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

/*
    { "IDE", 		??, 	2, 0xB4000170, 15 }, // 0x376 io2
    { "IDE", 		??, 	2, 0xB40001F0, 14 }, // 0x3F6 io2
    { "LCD", 		driver_mem_icp_lcd_probe, 	0, 0xC0000000, 22 },

    { "touch",		driver_mem_icp_touch_probe,   	1, 0x1E000000, 28 },
    { "PL041.Audio",   	driver_mem_pl041_audio_probe,   2, 0x1D000000, 25 },



    { "LEDS", 		driver_mem_icp_leds_probe, 	0, 0x1A000000, 0 },

    { "LAN91C111", 	driver_mem_LAN91C111_net_probe, 2, 0xC8000000, 27 },
*/

    // End of list marker
    { 0, 0, 0, 0, 0 },
};



void board_make_driver_map(void)
{
    //int id = R32(ICP_IDFIELD);

    //if( (id >> 24) != 0x41 )        SHOW_ERROR( 0, "Board manufacturer is %d, not %d", (id >> 24), 0x41 );

    // don't need - this window is directly accessible for kernel
    //hal_pages_control( 0x14000000u, (void *)0xB4000000u, 0xE0000, page_map_io, page_rw );
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
//#warning completely wrong!!!
    //    return debug_console_getc();
    while(1)
        hal_sleep_msec(10000);
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

    addr_t uptokernel = kvtophys(&end);

    int len = 256*1024*1024; // Hardcode 256M of RAM
    assert( 0 == amap_modify( ram_map, uptokernel, len-uptokernel, MEM_MAP_HI_RAM) );

    assert( 0 == amap_modify( ram_map, kvtophys(BOARD_ISA_IO), BOARD_ISA_IO_LEN, MEM_MAP_DEV_MEM) );
}




