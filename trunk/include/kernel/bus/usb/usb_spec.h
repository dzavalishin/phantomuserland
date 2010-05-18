/*
** Copyright 2003, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _USB_SPEC_H
#define _USB_SPEC_H

#include <shorttypes.h>
#include <sys/cdefs.h>

#define _PACKED __packed

typedef struct usb_request {
	uint8 type;
	uint8 request;
	uint16 value;
	uint16 index;
	uint16 len;
} _PACKED usb_request;

// USB Spec Rev 1.1, table 9-2, p 183
#define USB_REQTYPE_DEVICE_IN         0x80
#define USB_REQTYPE_DEVICE_OUT        0x00
#define USB_REQTYPE_INTERFACE_IN      0x81
#define USB_REQTYPE_INTERFACE_OUT     0x01
#define USB_REQTYPE_ENDPOINT_IN       0x82
#define USB_REQTYPE_ENDPOINT_OUT      0x02
#define USB_REQTYPE_OTHER_OUT         0x03
#define USB_REQTYPE_OTHER_IN          0x83

// USB Spec Rev 1.1, table 9-2, p 183
#define USB_REQTYPE_STANDARD          0x00
#define USB_REQTYPE_CLASS             0x20
#define USB_REQTYPE_VENDOR            0x40
#define USB_REQTYPE_RESERVED          0x60
#define USB_REQTYPE_MASK              0x9F

// USB Spec Rev 1.1, table 9-4, p 187
#define USB_REQUEST_GET_STATUS           0
#define USB_REQUEST_CLEAR_FEATURE        1
#define USB_REQUEST_SET_FEATURE          3
#define USB_REQUEST_SET_ADDRESS          5
#define USB_REQUEST_GET_DESCRIPTOR       6
#define USB_REQUEST_SET_DESCRIPTOR       7
#define USB_REQUEST_GET_CONFIGURATION    8
#define USB_REQUEST_SET_CONFIGURATION    9
#define USB_REQUEST_GET_INTERFACE       10
#define USB_REQUEST_SET_INTERFACE       11
#define USB_REQUEST_SYNCH_FRAME         12

// USB Spec Rev 1.1, table 9-5, p 187
#define USB_DESCRIPTOR_DEVICE            1
#define USB_DESCRIPTOR_CONFIGURATION     2
#define USB_DESCRIPTOR_STRING            3
#define USB_DESCRIPTOR_INTERFACE         4
#define USB_DESCRIPTOR_ENDPOINT          5

// USB Spec Rev 1.1, table 9-6, p 188
#define USB_FEATURE_DEVICE_REMOTE_WAKEUP 1
#define USB_FEATURE_ENDPOINT_HALT        0

// USB Spec Rev 1.1, table 9-7, p 197
typedef struct {
	uint8 length;
	uint8 descriptor_type;
	uint16 usb_version;
	uint8 device_class;
	uint8 device_subclass;
	uint8 device_protocol;
	uint8 max_packet_size_0;
	uint16 vendor_id;
	uint16 product_id;
	uint16 device_version;
	uint8 manufacturer;
	uint8 product;
	uint8 serial_number;
	uint8 num_configurations;
} _PACKED usb_device_descriptor;

// USB Spec Rev 1.1, table 9-8, p 199
typedef struct {
	uint8 length;
	uint8 descriptor_type;
	uint16 total_length;
	uint8 number_interfaces;
	uint8 configuration_value;
	uint8 configuration;
	uint8 attributes;
	uint8 max_power;
} _PACKED usb_configuration_descriptor;

// USB Spec Rev 1.1, table 9-9, p 202
typedef struct {
	uint8 length;
	uint8 descriptor_type;
	uint8 interface_number;
	uint8 alternate_setting;
	uint8 num_endpoints;
	uint8 interface_class;
	uint8 interface_subclass;
	uint8 interface_protocol;
	uint8 interface;
} _PACKED usb_interface_descriptor;

// USB Spec Rev 1.1, table 9-10, p 203
typedef struct {
	uint8 length;
	uint8 descriptor_type;
	uint8 endpoint_address;
	uint8 attributes;
	uint16 max_packet_size;
	uint8 interval;
} _PACKED usb_endpoint_descriptor;

#define USB_ENDPOINT_ATTR_CONTROL 0x0
#define USB_ENDPOINT_ATTR_ISO     0x1
#define USB_ENDPOINT_ATTR_BULK    0x2
#define USB_ENDPOINT_ATTR_INT     0x3

// USB Spec Rev 1.1, table 9-12, p 205
typedef struct {
	uint8 length;
	uint8 descriptor_type;
	unsigned char string[1];
} _PACKED usb_string_descriptor;

typedef struct {
	uint8 length;
	uint8 descriptor_type;
	uint8 data[1];
} _PACKED usb_generic_descriptor;

typedef union {
	usb_generic_descriptor generic;
	usb_device_descriptor device;
	usb_interface_descriptor interface;
	usb_endpoint_descriptor endpoint;
	usb_configuration_descriptor configuration;
	usb_string_descriptor string;
} usb_descriptor;

typedef struct {
	uint8 length;
	uint8 descriptor_type;
	uint8 num_ports;
	uint16 characteristics;
	uint8 power_delay;
	uint8 control_current;
	uint8 removable[8];
} _PACKED usb_hub_descriptor;

#define USB_HUB_REQUEST_GET_STATE	2

#define USB_HUB_PORTSTAT_CONNECTION    0x0001
#define USB_HUB_PORTSTAT_ENABLED       0x0002
#define USB_HUB_PORTSTAT_SUSPEND       0x0004
#define USB_HUB_PORTSTAT_OVER_CURRENT  0x0008
#define USB_HUB_PORTSTAT_RESET         0x0010
#define USB_HUB_PORTSTAT_POWER_ON      0x0100
#define USB_HUB_PORTSTAT_LOW_SPEED     0x0200

#define USB_HUB_CX_PORT_CONNECTION     0x0001
#define USB_HUB_CX_PORT_ENABLE         0x0002
#define USB_HUB_CX_PORT_SUSPEND        0x0004
#define USB_HUB_CX_PORT_OVER_CURRENT   0x0008
#define USB_HUB_CX_PORT_RESET          0x0010

#define USB_HUB_C_HUB_LOCAL_POWER		0
#define USB_HUB_C_HUB_OVER_CURRENTR		1

#define USB_HUB_PORT_CONNECTION			0
#define USB_HUB_PORT_ENABLE				1
#define USB_HUB_PORT_SUSPEND			2
#define USB_HUB_PORT_OVER_CURRENT		3
#define	USB_HUB_PORT_RESET				4
#define	USB_HUB_PORT_POWER				8
#define USB_HUB_PORT_LOW_SPEED			9

#define USB_HUB_C_PORT_CONNECTION		16
#define USB_HUB_C_PORT_ENABLE			17
#define USB_HUB_C_PORT_SUSPEND			18
#define USB_HUB_C_PORT_OVER_CURRENT		19
#define USB_HUB_C_PORT_RESET			20

#endif

