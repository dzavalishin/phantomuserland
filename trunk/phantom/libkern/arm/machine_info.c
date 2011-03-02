/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * ia32 cpuid support
 *
**/

#define DEBUG_MSG_PREFIX "cpuid"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


//#include <kernel/ia32/cpu.h>
#include <kernel/init.h>

#include <string.h>
#include <phantom_libc.h>


int detect_cpu(int curr_cpu)
{
	printf("CPU %d: features: %s\n", curr_cpu, cpu->arch.feature_string);
#warning implement
	return 0;
}





int boot_cpu_has_apic(void)
{
	SHOW_ERROR0( 0, "Request for APIC existance on arm" );
    return 0;
}

