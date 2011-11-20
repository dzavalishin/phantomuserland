#if 0

	/*
** Copyright 2003, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _USB_SPEC_H
#define _USB_SPEC_H

#include <sys/cdefs.h>
#include <phantom_types.h>

typedef struct usb_request {
	u_int8_t type;
	u_int8_t request;
	u_int16_t value;
	u_int16_t index;
	u_int16_t len;
} __packed usb_request;

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
	u_int8_t length;
	u_int8_t descriptor_type;
	u_int16_t usb_version;
	u_int8_t device_class;
	u_int8_t device_subclass;
	u_int8_t device_protocol;
	u_int8_t max_packet_size_0;
	u_int16_t vendor_id;
	u_int16_t product_id;
	u_int16_t device_version;
	u_int8_t manufacturer;
	u_int8_t product;
	u_int8_t serial_number;
	u_int8_t num_configurations;
} __packed usb_device_descriptor;

// USB Spec Rev 1.1, table 9-8, p 199
typedef struct {
	u_int8_t length;
	u_int8_t descriptor_type;
	u_int16_t total_length;
	u_int8_t number_interfaces;
	u_int8_t configuration_value;
	u_int8_t configuration;
	u_int8_t attributes;
	u_int8_t max_power;
} __packed usb_configuration_descriptor;

// USB Spec Rev 1.1, table 9-9, p 202
typedef struct {
	u_int8_t length;
	u_int8_t descriptor_type;
	u_int8_t interface_number;
	u_int8_t alternate_setting;
	u_int8_t num_endpoints;
	u_int8_t interface_class;
	u_int8_t interface_subclass;
	u_int8_t interface_protocol;
	u_int8_t interface;
} __packed usb_interface_descriptor;

// USB Spec Rev 1.1, table 9-10, p 203
typedef struct {
	u_int8_t length;
	u_int8_t descriptor_type;
	u_int8_t endpoint_address;
	u_int8_t attributes;
	u_int16_t max_packet_size;
	u_int8_t interval;
} __packed usb_endpoint_descriptor;

#define USB_ENDPOINT_ATTR_CONTROL 0x0
#define USB_ENDPOINT_ATTR_ISO     0x1
#define USB_ENDPOINT_ATTR_BULK    0x2
#define USB_ENDPOINT_ATTR_INT     0x3

// USB Spec Rev 1.1, table 9-12, p 205
typedef struct {
	u_int8_t length;
	u_int8_t descriptor_type;
	unsigned char string[1];
} __packed usb_string_descriptor;

typedef struct {
	u_int8_t length;
	u_int8_t descriptor_type;
	u_int8_t data[1];
} __packed usb_generic_descriptor;

typedef union {
	usb_generic_descriptor generic;
	usb_device_descriptor device;
	usb_interface_descriptor interface;
	usb_endpoint_descriptor endpoint;
	usb_configuration_descriptor configuration;
	usb_string_descriptor string;
} usb_descriptor;

typedef struct {
	u_int8_t length;
	u_int8_t descriptor_type;
	u_int8_t num_ports;
	u_int16_t characteristics;
	u_int8_t power_delay;
	u_int8_t control_current;
	u_int8_t removable[8];
} __packed usb_hub_descriptor;

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

#endif
