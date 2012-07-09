/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ACPI to Phantom interface - Buttons
 *
 *
**/

#ifdef ARCH_ia32

#define DEBUG_MSG_PREFIX "acpi"
#include <debug_ext.h>
#define debug_level_flow 1
#define debug_level_error 10
#define debug_level_info 10

#include <assert.h>

/**
 * \file
 * \brief ACPI button event handlers
 */

/*
 * Copyright (c) 2009, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#include <stdio.h>
#include <kernel/dpc.h>
#include <sys/types.h>

//#include <barrelfish/barrelfish.h>
#include <acpi.h>

//#include "acpi_shared.h"
//#include "acpi_debug.h"

#include <acpi/accommon.h>
//#include "acpi/amlcode.h"
//#include "acpi/acparser.h"
#include <acpi/acdebug.h>

//#define ACPI_DEBUG(x...) printf(x)
#define ACPI_DEBUG(f,x...) SHOW_FLOW( 1, f, x )


#define OFF_STATE 5 // S5 state (really off!)


static dpc_request_t poweroff_dpc;


static void acpi_power_off(void *arg)
{
    ACPI_STATUS as;

    // TODO shutdown supposed here

    printf("Power button pressed, do switching off...\n");
    as = AcpiEnterSleepStatePrep(OFF_STATE);
    if (!ACPI_SUCCESS(as)) {
        printf("AcpiEnterSleepStatePrep failed\n");
        return;
    }

    as = AcpiEnterSleepState(OFF_STATE);
    printf("Well, that sure didn't work!\n");
    if (!ACPI_SUCCESS(as)) {
        printf("AcpiEnterSleepState failed\n");
    }
}


// This one is called in interrupt!
static u_int32_t power_button_handler(void *arg)
{
    printf("Power button pressed, switching off...\n");
    dpc_request_trigger( &poweroff_dpc, 0 );

    return 0;
}

static void power_button_notify_handler(ACPI_HANDLE handle, u_int32_t value,
                                        void *context)
{
    //printf("Power button notify 0x%"PRIx32"\n", value);
    printf("Power button notify 0x%d", value);
    if (value == 0x80) { // switch off!
        power_button_handler(context);
    }
}

static ACPI_STATUS power_button_probe(ACPI_HANDLE handle, u_int32_t nestlevel,
                                      void *context, void **retval)
{
    // install handler
    printf("Installing notify handler for power/sleep button\n");
    return AcpiInstallNotifyHandler(handle, ACPI_DEVICE_NOTIFY,
                                    power_button_notify_handler, NULL);
}

void acpi_buttons_init(void)
{
    ACPI_STATUS as;

    dpc_request_init( &poweroff_dpc, acpi_power_off );

    if ((AcpiGbl_FADT.Flags & ACPI_FADT_POWER_BUTTON) == 0) {
        SHOW_FLOW0( 1, "Installing fixed event handler for power button" );
        as = AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON,
                                          power_button_handler, NULL);
        assert(ACPI_SUCCESS(as));
    }

    // install handlers for notify events on other button objects
    AcpiGetDevices("PNP0C0C", power_button_probe, NULL, NULL); // power buttons
    AcpiGetDevices("PNP0C0E", power_button_probe, NULL, NULL); // sleep buttons
}

#endif // ARCH_ia32
