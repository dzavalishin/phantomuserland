/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM peripheral device identification
 *
**/

#define DEBUG_MSG_PREFIX "arm-id"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/arm/devid.h>
#include <arm/memio.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>


// ARM periph devices, usually, have 8 id bytes in upper part
// of dev io memory space.

#define ID_DEF_SHIFT 0xFE0



errno_t arm_id_shift( addr_t base, int shift, u_int32_t part_num, u_int32_t cellId, int verbose )
{
    int level = verbose ? 0 : 1;
    errno_t res = 0;

    u_int32_t id;

    id  =  R32(base+shift+0)        & 0xFF;
    id |= (R32(base+shift+1) <<  8) & 0xFF;
    id |= (R32(base+shift+2) << 16) & 0xFF;
    id |= (R32(base+shift+3) << 24) & 0xFF;

    SHOW_INFO( level, "@%x+%x: Part no %x, designer %x, rev. %d, conf %x", base, shift,
               (id & 0xFFF),
               (id>>12) & 0xFF,
               (id>>20) & 0x0F,
               (id>>24) & 0xFF
             );

    if( (id & 0xFFF) != part_num)
    {
        SHOW_ERROR( level, "Expected %x part number, got %x", part_num, (id & 0xFFF) );
        res = ENXIO;
    }

    id  =  R32(base+shift+4)        & 0xFF;
    id |= (R32(base+shift+5) <<  8) & 0xFF;
    id |= (R32(base+shift+6) << 16) & 0xFF;
    id |= (R32(base+shift+7) << 24) & 0xFF;

    if( id != cellId)
    {
        SHOW_ERROR( level, "Expect %x cell id, got %x", cellId, id );
        res = ENXIO;
    }


    return res;
}


errno_t arm_id( addr_t base, u_int32_t part_num, u_int32_t cellId, int verbose )
{
    return arm_id_shift( base, ID_DEF_SHIFT, part_num, cellId, verbose );
}








