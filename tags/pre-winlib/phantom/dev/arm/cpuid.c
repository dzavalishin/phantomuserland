/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * arm cpuid support
 *
**/

#define DEBUG_MSG_PREFIX "cpuid"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

//#include <cpuid.h>

#include <phantom_libc.h>
#include <string.h>
#include <stdlib.h>



#if 0
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

    {128,		"MIPS 4Kc"},
    {129,		"MIPS 5Kc"},
    {130,		"MIPS 20Kc"},
    {131,		"MIPS 4Kmp"},
    {132,		"MIPS 4Kec"},
    {133,		"MIPS 4KEmp"},
    {134,		"MIPS 4KSc"},
    {135,		"MIPS M4K"},
    {136,		"MIPS 25Kf"},
    {137,		"MIPS 5Ke"},

    {144,		"MIPS 4KEc (r2)"},
    {145,		"MIPS 4KEmp (r2)"},
    {146,		"MIPS 4KSd"},
    {147,		"MIPS 24K"},

    {149,		"MIPS 34K"},
    {150,		"MIPS 24KE"},

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

#endif

void
identify_cpu(void)
{
    //u_int32_t id = mips_c0_get_id();

    //const char *cpu_type = get_brandid_string( 0xFF & (id>>8) );

    printf("cpu0: arm? (no cpuid yet)\n" );
}

void
identify_hypervisor(void)
{
}

