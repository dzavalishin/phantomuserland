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


}


