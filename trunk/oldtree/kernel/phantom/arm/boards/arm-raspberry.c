/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Arm Raspberry PI hardware mappings.
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
#include <arm/memio.h>
#include <arm/private.h>

#include <dev/mem_disk.h>

#include "arm-raspberry.h"

#define DEBUG_MSG_PREFIX "board"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

char board_name[] = "Raspberry PI";

//static char * symtab_getname( void *addr );


//static int icp_irq_dispatch(struct trap_state *ts);
//static void sched_soft_int( void *a );

void board_init_early(void)
{

    // Relocate trap table to address 0
    extern unsigned int _start_of_kernel[];
    unsigned int *atzero = 0;

    unsigned int shift = (unsigned int)&_start_of_kernel;

    assert(0==(shift&3));

    // TODO wrong place - must be in arm arch code

    // Copy branch instructions, correcting (relative) target
    // address by distance we move them to. Branch address lacks
    // lower 2 bits (allways 0), so divide shift by 4
    int i;
    for( i = 0; i < 8; i++ )
    {
        atzero[i] = _start_of_kernel[i] + (shift/4);
    }

    // TODO wrong place - must be in arm arch code
    //phantom_symtab_getname = symtab_getname;
}

void board_init_cpu_management(void)
{
}



void board_init_kernel_timer(void)
{
    arm_raspberry_timer0_init(100);
}

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
    int len = *(int*)(addr-4);
    if( (len & 0xFF000000) != 0xFF000000 )
        return "?";

    return (char *)(addr - 4 - (len&0xFFFFFF));
}
*/

// -----------------------------------------------------------------------
// Interrupts processing
// -----------------------------------------------------------------------


void board_interrupt_enable(int irq)
{
    arm_bcm2835_interrupt_enable(irq);
}

void board_interrupt_disable(int irq)
{
    arm_bcm2835_interrupt_disable(irq);
}

void board_init_interrupts(void)
{
    arm_bcm2835_init_interrupts();
}

void board_interrupts_disable_all(void)
{
    arm_bcm2835_interrupts_disable_all();
}




void board_sched_cause_soft_irq(void)
{
    int ie = hal_save_cli();

    asm volatile("swi 0xFFF");

    //phantom_scheduler_soft_interrupt();

    if(ie) hal_sti();
}


// -----------------------------------------------------------------------
// Drivers
// -----------------------------------------------------------------------

// NB! No network drivers on stage 0!
static isa_probe_t icp_board_drivers[] =
{

//    { "UART0", 		driver_pl011_uart_probe, 	1, 0x16000000, 1 },
/*
    { "UART1", 		driver_pl011_uart_probe, 	1, 0x17000000, 2 },

    { "RTC", 		driver_pl031_rtc_probe, 	0, 0x15000000, 8 },


    { "MMC",		driver_pl181_mmc_probe,   	1, 0x1C000000, 23 }, // And 24 - how do we give 2 irqs?
    { "PL050.kb",      	driver_pl050_keyb_probe,   	1, 0x18000000, 3 },
    { "PL050.ms",      	driver_pl050_mouse_probe,   	1, 0x19000000, 4 },
*/
// icp has flash mapped here
//    { "memdisk",      	driver_icp_memdisk_probe,   	3, 0x20000000, -1 },

/*
    { "GPIO", 		driver_mem_icp_gpio_probe, 	0, 0xC9000000, 0 },
    { "LCD", 		driver_mem_icp_lcd_probe, 	0, 0xC0000000, 22 },

    { "touch",		driver_mem_icp_touch_probe,   	1, 0x1E000000, 28 },
    { "PL041.Audio",   	driver_mem_pl041_audio_probe,   2, 0x1D000000, 25 },



    { "LEDS", 		driver_mem_icp_leds_probe, 	0, 0x1A000000, 0 },

    { "LAN91C111", 	driver_mem_LAN91C111_net_probe, 2, 0xC8000000, 27 },
*/

    // End of list marker
    { 0, 0, 0, 0, 0 },
};

static int flash_mbytes = 0;


void board_make_driver_map(void)
{

    phantom_register_drivers(icp_board_drivers);
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

    int uptokernel = (int)&end;

    // Suppose we have 512Mb and give out 128Mb to video
    int len = (512-128)*1024*1024;
    assert( 0 == amap_modify( ram_map, uptokernel, len-uptokernel, MEM_MAP_HI_RAM) );
}





