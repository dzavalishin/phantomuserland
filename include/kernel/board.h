/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Board (specific hardware configuration) mappings.
 *
 *
**/

#ifndef _K_BOARD_H
#define _K_BOARD_H

#include <kernel/amap.h>

/**
 * \ingroup Arch
 * @{
**/

struct phantom_device;

typedef struct
{
    const char *	name;
    struct phantom_device * (*probe_f)( int port, int irq, int stage );        // Driver probe func
    int 		minstage; // How early we can be called
    int 		port; // can be zero, driver supposed to find itself
    int 		irq; // can be zero, driver supposed to find itself
} isa_probe_t;




//! The first function called by init code at all.
//! At this point all we have is stack. Do what is
//! needed earliest to prepare for later startup
//! code. Such as enable hardware components, correct
//! memory access, relocate kernel, etc etc.
void board_init_early(void);


//! Init what is needed to management of cpu state, such
//! as descriptor tables, etc.
void board_init_cpu_management(void);


//! For drivers which have no standard way of finding its
//! device ports/memory/irq, produce list of possible mappings
//! to probe for devices at known addresses. Or, if board has
//! some meanings of automatic device detection, use 'em.
//! This function calls back to phantom_register_drivers().
void board_make_driver_map(void);

//! Called back by board code to pass drivers list.
void phantom_register_drivers(isa_probe_t *drivers );


//! This must bring alive interrupts from main OS timer
//! providing about 100HZ calls to hal_time_tick().
//! Drives timed calls, sleep_msec, scheduling and date/time.

void board_init_kernel_timer(void);



//! Init default interupts hardware
void board_init_interrupts(void);

//! Call board-specific interrupt line enable function
void board_interrupt_enable(int irq);

//! Call board-specific interrupt line disable function
void board_interrupt_disable(int irq);

//! Disable all the possible interrupt lines. Not cli!
void board_interrupts_disable_all(void);


//! Start other SMP processors and corresponding infrastructure
void board_start_smp(void);




//! Produce map of system's memory using MEM_MAP_ attributes.
//! Most notably MEM_MAP_HI_RAM and MEM_MAP_KERNEL must be
//! filled in. MEM_MAP_KERNEL will get identical mapping and
//! _RAM will be added to phys ram pool.
void board_fill_memory_map( amap_t *ram_map );

#define MEM_MAP_UNKNOWN 1
#define MEM_MAP_LOW_RAM 2
#define MEM_MAP_HI_RAM  3
#define MEM_MAP_DEV_MEM 4
#define MEM_MAP_BIOS_DA 5
#define MEM_MAP_KERNEL  6
#define MEM_MAP_MODULE  7
#define MEM_MAP_MOD_NAME  8
#define MEM_MAP_MODTAB  9
#define MEM_MAP_ARGS    10
#define MEM_MAP_ELF_HDR 11
#define MEM_MAP_ELF_SEC 12

#define MEM_MAP_NOTHING 0


//! Get character from debug console
//int debug_console_getc(void);

//! Print character on boot console
int board_boot_console_putc(int c);

//! Get character from boot console
int board_boot_console_getc( void );


//! Stop interrupts, timer, seconary CPUs...
void board_panic_stop_world(void);


//! Wait for a key press on default console - don't use interrupts, assume interrupts disabled
void board_panic_wait_keypress(void);


#endif // _K_BOARD_H

