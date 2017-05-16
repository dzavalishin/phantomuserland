/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * Elbrus2K default board code
 *
**/

#include <kernel/board.h>
#include <kernel/init.h>
#include <kernel/drivers.h>

//#include <kernel/bus/isa/pic.h>
//#include <dev/isa/pic_regs.h>

char board_name[] = "e2k_default";


void board_init_early(void)
{
#warning write me
    // do nothing. Suppose we got quite a reasonable state
    // frtom boot code.
}

void board_init_cpu_management(void)
{
#warning write me
    //phantom_init_descriptors();
    //phantom_fill_idt();
    //phantom_load_idt();
}


void board_init_kernel_timer(void)
{
#warning write me
    //phantom_timer_pit_init(100,0);
}


void board_start_smp(void)
{
//#warning write me
    //phantom_init_apic(); // Starts other CPUs
	panic( "No SMP on E2K yet" );
}





void board_init_interrupts(void)
{
#warning write me
    // Program the PIC
    //phantom_pic_init(PIC_VECTBASE, PIC_VECTBASE+8);

    // Disable all the IRQs
    //phantom_pic_disable_all();
}

void board_interrupts_disable_all(void)
{
#warning write me
    //phantom_pic_disable_all();
}



void board_interrupt_enable(int irq)
{
#warning write me
    //phantom_pic_enable_irq(irq);
}

void board_interrupt_disable(int irq)
{
#warning write me
    //phantom_pic_disable_irq(irq);
}


// -----------------------------------------------------------------------
// Drivers
// -----------------------------------------------------------------------




// NB! No network drivers on stage 0!
static isa_probe_t board_drivers[] =
{
#if 0
    { "LPT1", 		driver_isa_lpt_probe, 	2, 0x378, 7 },
    { "LPT2", 		driver_isa_lpt_probe, 	2, 0x278, 5 },

    { "COM1",		driver_isa_com_probe,   2, 0x3F8, 4 },
    { "COM2",		driver_isa_com_probe,   2, 0x2F8, 3 },
    { "COM3",		driver_isa_com_probe,   2, 0x3E8, 4 },
    { "COM4",		driver_isa_com_probe,   2, 0x2E8, 3 },

    { "CGA", 		driver_isa_vga_probe, 	0, 0x3D4, -1 },
    { "MDA", 		driver_isa_vga_probe, 	0, 0x3B4, -1 },

    { "PS2 Keyboard", 	driver_isa_ps2k_probe, 	1, -1, 1 },
    { "PS2 Mouse", 	driver_isa_ps2m_probe, 	1, -1, 12 },
//#endif
//#if HAVE_NET && 0
    { "NE2000", 	driver_isa_ne2000_probe,1, 0x280, 11 },
    { "NE2000", 	driver_isa_ne2000_probe,1, 0x300, 11 },
    { "NE2000", 	driver_isa_ne2000_probe,1, 0x320, 11 },
    { "NE2000", 	driver_isa_ne2000_probe,1, 0x340, 11 },
    { "NE2000", 	driver_isa_ne2000_probe,1, 0x360, 11 },
#endif

#if 0
    { "SB16",          driver_isa_sb16_probe,  2, 0x210, 5 },
    { "SB16",          driver_isa_sb16_probe,  2, 0x220, 5 },
    { "SB16",          driver_isa_sb16_probe,  2, 0x230, 5 },
    { "SB16",          driver_isa_sb16_probe,  2, 0x240, 5 },
    { "SB16",          driver_isa_sb16_probe,  2, 0x250, 5 },
    { "SB16",          driver_isa_sb16_probe,  2, 0x260, 5 },

//    { "AdLib",         driver_isa_sdlib_probe, 3, 0x388, 8 },

    { "floppy",        driver_isa_floppy_probe,  3, 0, 0 }, // hardcoded in driver
    { "floppy",        driver_isa_floppy_probe,  3, 0, 0 }, // two of them!

    { "Beep",           driver_isa_beep_probe,  0, 0x42, -1 },
#endif
    // End of list marker
    { 0, 0, 0, 0, 0 },
};



void board_make_driver_map(void)
{
    phantom_register_drivers(board_drivers);
}

//! Stop interrupts, timer, seconary CPUs...
void board_panic_stop_world(void)
{
	board_interrupts_disable_all();
}

#include <console.h>


//! Wait for a key press on default console - don't use interrupts, assume interrupts disabled
void board_panic_wait_keypress(void)
{
    //phantom_scan_console_getc();
    board_boot_console_getc();
}

/*
int board_boot_console_getc(void)
{
    return debug_console_getc();
}
*/
