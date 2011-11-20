/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ACPI related stuff.
 *
**/

#ifndef KERNEL_ACPI_H
#define KERNEL_ACPI_H

#include <errno.h>

/* Sleep states */

enum {
	ACPI_POWER_STATE_ON = 0,
	ACPI_POWER_STATE_SLEEP_S1,
	ACPI_POWER_STATE_SLEEP_S2,
	ACPI_POWER_STATE_SLEEP_S3,
	ACPI_POWER_STATE_HIBERNATE,
	ACPI_POWER_STATE_OFF
};

errno_t acpi_powerdown(void);
errno_t acpi_reboot(void);



#endif // KERNEL_ACPI_H

