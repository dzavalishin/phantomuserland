/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ACPI reboot/poweroff
 *
 *
**/


#define DEBUG_MSG_PREFIX "acpi"
#include <debug_ext.h>
#define debug_level_flow 8
#define debug_level_error 10
#define debug_level_info 10

#include <acpi.h>
#include <kernel/acpi.h>

#include <threads.h>
#include <errno.h>
#include <time.h>


errno_t
acpi_powerdown(void)
{
    ACPI_STATUS status;

    t_migrate_to_boot_CPU();

    status = AcpiEnterSleepStatePrep(ACPI_POWER_STATE_OFF);
    if (status != AE_OK)
        return ENXIO;

    status = AcpiEnterSleepState(ACPI_POWER_STATE_OFF);
    if (status != AE_OK)
        return ENXIO;

    return 0;
}


errno_t
acpi_reboot(void)
{
    ACPI_STATUS status;

    status = AcpiReset();
    if (status == AE_NOT_EXIST)
        return ENXIO;

    if (status != AE_OK) {
        SHOW_ERROR( 0, "Reset failed, status = %d", status );
        return ENXIO;
    }

    phantom_spinwait(10000); // 10 sec
    SHOW_ERROR0( 0, "Reset failed, timeout" );
    return ENXIO;
}
