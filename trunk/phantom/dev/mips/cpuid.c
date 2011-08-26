/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * mips cpuid support
 *
**/

#define DEBUG_MSG_PREFIX "cpuid"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <mips/cp0_regs.h>

#include <phantom_libc.h>
#include <string.h>
#include <stdlib.h>




struct brandid_map {
    u_int32_t key;
    const char *val;	/* These need to be less than 48 bytes */
} brandid_map[] = {
    {1,			"R2000 or R3000"},
    {2,			"IDT R305x"},
    {3,			"R6000"},
    {4,			"R4000 or R4400"},
    {5,			"LSI Logic"},
    {6,			"R6000A"},
    {7,			"IDT R3041"},

    {9,			"R10000"},
    {10,		"NEC Vr4200"},
    {11,		"NEC Vr4300"},
    {12,		"NEC Vr41xx"},

    {16,		"R8000"},

    {32,		"R4600"},
    {33,		"IDT R4700"},
    {34,		"Toshiba R3900"},
    {35,		"R5000"},

    {40,		"QED RM5230/5260"},

    {128,		"4Kc"},
    {129,		"5Kc"},
    {130,		"20Kc"},
    {131,		"4Kmp"},
    {132,		"4Kec"},
    {133,		"4KEmp"},
    {134,		"4KSc"},
    {135,		"M4K"},
    {136,		"25Kf"},
    {137,		"5Ke"},

    {144,		"4KEc (r2)"},
    {145,		"4KEmp (r2)"},
    {146,		"4KSd"},
    {147,		"24K"},

    {149,		"34K"},
    {150,		"24KE"},

    {0x00, NULL},
};

static const char *
get_brandid_string(u_int32_t brandid)
{
    struct brandid_map *p;

    /* Scan table for rest */
    for(p = brandid_map; p->val; p++)
        if(p->key == brandid)
            return p->val;

    return "?";
}


void
identify_cpu(void)
{
    u_int32_t id = mips_read_cp0_cpuid();

    int rev = id & 0xFF;
    int vendor_id = (id>16) & 0xFF;

    const char *cpu_vendor = "(unknown vendor)";

    switch( vendor_id )
    {
    case 0: cpu_vendor = "Prehistoric"; break;
    case 1: cpu_vendor = "MIPS?";       break;
    }

    const char *cpu_type = get_brandid_string( 0xFF & (id>>8) );

    printf("cpu0: %s (%d) %s rev. %d\n", cpu_vendor, vendor_id, cpu_type, rev );
}

void
identify_hypervisor(void)
{
}

