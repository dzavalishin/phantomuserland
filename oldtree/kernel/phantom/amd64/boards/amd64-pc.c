/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Amd 64 - usual PC mappings.
 *
**/


#include <kernel/board.h>

// TODO simply inlude ia32 code here? we copy everything...

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






void board_interrupt_enable(int irq)
{
    // TODO switch to apic?
    phantom_pic_enable_irq(irq);
}

void board_interrupt_disable(int irq)
{
    phantom_pic_disable_irq(irq);
}

void board_init_interrupts(void)
{
    // Program the PIC
    phantom_pic_init(PIC_VECTBASE, PIC_VECTBASE+8);

    // Disable all the IRQs
    phantom_pic_disable_all();
}

void board_interrupts_disable_all(int irq)
{
    phantom_pic_disable_all();
}



