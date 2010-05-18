/*
** Copyright 2003, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _USB_BUS_H
#define _USB_BUS_H

#include <kernel/bus/usb/usb_spec.h>

#define USB_BUS_MODULE_NAME "bus_managers/usb/v1"

struct usb_module_hooks {
	int (*null)(void); // XXX add stuff here
};

#endif
