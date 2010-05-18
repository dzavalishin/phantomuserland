/*
** Copyright 2003, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _USB_HC_BUS_H
#define _USB_HC_BUS_H

/* Contains the usb declarations needed to deal with USB Host Controller
 * modules. Used internally by the USB stack
 */
#define USB_HC_MODULE_NAME_PREFIX "busses/usb"

typedef void hc_cookie;
typedef void hc_endpoint;

typedef struct usb_hc_transfer {
	// setup portion of the transfer
	void *setup_buf;
	size_t setup_len;

	// data portion of the transfer
	void *data_buf;
	size_t data_len;

	// return code
	int status;

	// callback or completion sem (make sure the sem is <0 if you dont want to release it)
	void (*callback)(struct usb_hc_transfer *transfer, void *cookie);
	void *cookie;
	sem_id completion_sem;
} usb_hc_transfer;

struct usb_hc_module_hooks {
	// initialize the host controller.
	// When a host controller is found, the callback needs to be called, passing back
	// callback_cookie as the first arg, and a hc defined cookie as the second.
	int (*init_hc)(int (*hc_init_callback)(void *callback_cookie, void *cookie), hc_cookie *callback_cookie);

	// deinitialize the host controller
	// assumes all transactions are cancelled, all endpoints are destroyed
	int (*uninit_hc)(hc_cookie *cookie);

	// create an endpoint
	int (*create_endpoint)(hc_cookie *cookie, hc_endpoint **endpoint,
		usb_endpoint_descriptor *usb_endpoint, int address, bool lowspeed);

	// delete an endpoint
	int (*destroy_endpoint)(hc_cookie *cookie, hc_endpoint *endpoint);

	// queue a transfer
	int (*enqueue_transfer)(hc_cookie *cookie, hc_endpoint *endpoint, usb_hc_transfer *transfer);
};

#endif
