/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Intel - usual pc - NB! - shared between 32 and 64 bit default PC definitions!
 *
**/

#include <kernel/board.h>
#include <kernel/init.h>
#include <kernel/drivers.h>

#include <i386/isa/pic.h>
#include <i386/isa/pic_regs.h>

// SHARED CODE!
//char board_name[] = "PC32";


void board_init_early(void)
{
    // On PC do nothing. Suppose we got quite a reasonable state
    // frtom multiboot.
}

void board_init_cpu_management(void)
{
    phantom_init_descriptors();
    phantom_fill_idt();
    phantom_load_idt();
}


void board_init_kernel_timer(void)
{
    phantom_timer_pit_init(100,0);
}


void board_start_smp(void)
{
    phantom_init_apic(); // Starts other CPUs
}





void board_init_interrupts(void)
{
    // Program the PIC
    phantom_pic_init(PIC_VECTBASE, PIC_VECTBASE+8);

    // Disable all the IRQs
    phantom_pic_disable_all();
}

void board_interrupts_disable_all(void)
{
    phantom_pic_disable_all();
}



void board_interrupt_enable(int irq)
{
    // TODO switch to apic?
    phantom_pic_enable_irq(irq);
}

void board_interrupt_disable(int irq)
{
    phantom_pic_disable_irq(irq);
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
#endif
    { "CGA", 		driver_isa_vga_probe, 	0, 0x3D4, -1 },
    { "MDA", 		driver_isa_vga_probe, 	0, 0x3B4, -1 },

    { "PS2 Keyboard", 	driver_isa_ps2k_probe, 	1, -1, 1 },
    { "PS2 Mouse", 	driver_isa_ps2m_probe, 	1, -1, 12 },

    { "Beep",           driver_isa_beep_probe,  0, 0x42, -1 },

#if HAVE_NET && 0
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
#endif
//    { "AdLib",         driver_isa_sdlib_probe, 3, 0x388, 8 },

    // End of list marker
    { 0, 0, 0, 0, 0 },
};



void board_make_driver_map(void)
{
    phantom_register_drivers(board_drivers);
}

