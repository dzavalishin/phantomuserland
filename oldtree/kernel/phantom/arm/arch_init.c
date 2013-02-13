/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Arm machdep cpu/system init
 *
**/

#define DEBUG_MSG_PREFIX "arch.init"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/init.h>
#include <kernel/board.h>
#include <kernel/debug.h>
#include <device.h>

#include <arm/private.h>

static u_int32_t		isa_read32(u_int32_t addr)                      { return *((u_int32_t*)(addr)); }
static void			isa_write32(u_int32_t addr, u_int32_t value)    { *((u_int32_t*)(addr)) = value; }

static u_int16_t		isa_read16(u_int32_t addr)                      { return *((u_int16_t*)(addr)); }
static void			isa_write16(u_int32_t addr, u_int16_t value)    { *((u_int16_t*)(addr)) = value; }

static u_int8_t			isa_read8(u_int32_t addr)               	{ return *((u_int8_t*)(addr)); }
static void			isa_write8(u_int32_t addr, u_int8_t value)      { *((u_int8_t*)(addr)) = value; }



void arch_init_early(void)
{
    isa_bus.read32 	= isa_read32;
    isa_bus.write32 	= isa_write32;

    isa_bus.read16      = isa_read16;
    isa_bus.write16     = isa_write16;

    isa_bus.read8   	= isa_read8;
    isa_bus.write8      = isa_write8;

    arm_init_swi();
}


void arch_threads_init()
{
}


//! Stop interrupts, timer, seconary CPUs...
void board_panic_stop_world(void)
{
    board_interrupts_disable_all();
}



//! Wait for a key press on default console - don't use interrupts, assume interrupts disabled
void board_panic_wait_keypress(void)
{
    debug_console_getc();
}


// http://www.raspberrypi.org/phpBB3/viewtopic.php?f=72&t=13959

void arm_mem_barrier(void)
{
    //__asm__ __volatile__("": : :"memory")
    arm11_mem_barrier();
}

void arm_mem_write_barrier(void)
{
    //__asm__ __volatile__("": : :"memory");
    arm11_drain_writebuf();
}


void arm_mem_read_barrier(void)
{
    arm11_mem_barrier();
}





