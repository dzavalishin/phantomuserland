/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS Acer Pica hardware mappings.
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


/*

http://www.netbsd.org/Documentation/Hardware/Machines/ARC/riscspec.pdf

http://www.sensi.org/~alec/mips/acer_pica.html

	addr            size
	--------------------------------------
	00000000			mem
	1fc00000 	7e000		BIOS mipsel_bios.bin
	fff00000 	7e000		BIOS mipsel_bios.bin
	11000000	1000000		ISA, mem
	90000000 	1000000		ISA, io

        40000000	? 4mb 1024*768	Video memory
        60000000        ?               Video control, IRQ19
        60200000        ?               Video control, extended

        80001000	1000		Ethernet ctrlr
	80002000	1000		SCSI ctrlr
	80003000	1000		Floppy ctrlr
	80004000	1000		RTC
	80005000	1000		Keyb
	80006000	1000		COM1
	80007000	1000		COM2
	80008000	1000		LPT
	80009000	1000		NVRAM
	8000d000 	1000		DMA memory
	8000f000	1000		LED

*/


char board_name[] = "Acer Pica";

void board_init_early(void)
{
    // install TLB entry for COM1 dev io
    tlb_entry_t e;

    addr_t v = 0xE0006000;
    addr_t p = 0x80006000;

    e.v = v & TLB_V_ADDR_MASK; // One bit less than pte
    e.p0 = (p & TLB_P_ADDR_MASK) >> TLB_P_ADDR_SHIFT;

    e.p0 |= TLB_P_CACHE_UNCACHED << TLB_P_CACHE_SHIFT;
    e.p0 |= TLB_P_DIRTY;
    e.p0 |= TLB_P_VALID;
    e.p0 |= TLB_P_GLOBAL;

    e.p1 = e.p0; // for simplicity, will be rearranged later

    mips_tlb_write_random( &e );

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
}



// -----------------------------------------------------------------------
// Drivers
// -----------------------------------------------------------------------


// NB! No network drivers on stage 0!
static isa_probe_t board_drivers[] =
{


/*
    { "ps2_keyb",       driver_isa_ps2_keyb_probe,      0, PICA_SYS_KBD, 23 },
    { "ps2_mouse",      driver_isa_mouse_keyb_probe,    0, PICA_SYS_KBD, 24 },

    { "com1",           driver_isa_com_probe, 	        2, PICA_SYS_COM1, 25 },
    { "com2",           driver_isa_com_probe, 	        2, PICA_SYS_COM2, 26 },

    { "lpt",            driver_isa_lpt_probe, 	        2, PICA_SYS_PAR1, 17 },

    { "led",            driver_isa_led_probe, 	        2, R4030_V_LOCAL_IO_BASE+0xf000, -1 },

    { "scsi",           driver_isa_scsi_probe, 	        2, PICA_SYS_SCSI, 21 }, // DMA0
    { "rtc",            driver_isa_rtc_probe, 	        2, PICA_SYS_CLOCK, 31 },

    { "sound",          driver_isa_jazz_sound_probe,    2, PICA_SYS_SOUND, 18 },
    { "net",            driver_isa_sonic_net_probe,     2, PICA_SYS_SONIC, 21 },

*/

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

    // Main IO mem
    hal_pages_control( R4030_P_LOCAL_IO_BASE, (void *)R4030_V_LOCAL_IO_BASE, R4030_S_LOCAL_IO_BASE/PAGE_SIZE, page_map_io, page_rw );

    // ISA IO -> mem
    hal_pages_control( PICA_P_ISA_IO, (void *)PICA_V_ISA_IO, PICA_S_ISA_IO/PAGE_SIZE, page_map_io, page_rw );

    // ISA MEM -> mem
    hal_pages_control( PICA_P_ISA_MEM, (void *)PICA_V_ISA_MEM, PICA_S_ISA_MEM/PAGE_SIZE, page_map_io, page_rw );
    
    //assert( 0 == amap_modify( ram_map, 0x80000000, 0x10000, MEM_MAP_DEV_MEM) );
}





