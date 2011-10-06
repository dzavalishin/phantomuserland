/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * ACPI code
 *
**/

#define DEBUG_MSG_PREFIX "acpi"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <string.h>
//#include <pc/bios.h>
#include <phantom_libc.h>
#include <kernel/vm.h>
#include <kernel/drivers.h>
#include <errno.h>


#include "acpi.h"


static void * acpi_identify();
static errno_t acpi_cksum(char *p);


static int seq_number = 0;
phantom_device_t * driver_etc_acpi_probe( const char *name, int stage )
{
    (void) stage;

    void * ep = acpi_identify();

    SHOW_FLOW( 0, "found @ 0x%p", ep);

    if( seq_number || ep == 0 ) return 0;

    if(acpi_cksum(ep))
    {
        SHOW_ERROR0( 0, "checksum failed");
        return 0;
    }

    struct rsdp_descriptor *rsdp = ep;

    SHOW_INFO( 0, "ACPI Version: %u.x OEM %.6s", rsdp->revision, rsdp->oem_id );

#if 0
    if(bcd2bin(ep->SMBIOS_BCD_Revision))
        SHOW_INFO( 0, "SMBios BCD Revision: %u.%u",
                   bcd2bin(ep->SMBIOS_BCD_Revision >> 4),
                   bcd2bin(ep->SMBIOS_BCD_Revision & 0x0f));
#endif

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = name;
    dev->seq_number = seq_number++;
    dev->drv_private = ep;

    //dev->dops.read = smbios_read;
    /*
    dev->dops.stop = beep_stop;

    dev->dops.write = beep_write;
    */

    return dev;
}



#define ACPI_SIG "RSD PTR "

static char *lookfor( int _s, int _e )
{
    char *start = (char *)_s;
    char *end =   (char *)_e;

    char *a;
    for( a = start; a < end; a += 16 )
    {
        if(!bcmp(a, ACPI_SIG, 8))
        {
            return a;
        }
    }
    return 0;
}


static void * acpi_identify()
{
    char *a = 0;

    //a = lookfor( 0, 0x400 );    		if( a ) return a;
    a = lookfor( 0xE0000, 0x100000 );    	if( a ) return a;

    return 0;
}


static errno_t acpi_cksum(char *p)
{
    int sz = sizeof(struct rsdp_descriptor);

    char sum = 0;

    while( sz-- > 0 )
        sum += *p++;

    return (sum==0) ? 0 : ENXIO;
}



