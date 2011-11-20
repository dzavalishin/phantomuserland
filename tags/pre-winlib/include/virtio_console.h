/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Virtio console devices.
 *
 *
**/

#ifndef _VIRTIO_CONSOLE_H
#define _VIRTIO_CONSOLE_H
#include <phantom_types.h>
#include <virtio_config.h>
#include <sys/cdefs.h>
/* This header, excluding the #ifdef __KERNEL__ part, is BSD licensed so
 * anyone can use the definitions to implement compatible drivers/servers. */

/** \ingroup Virtio
 *  @{
 */


/* The ID for virtio console */
#define VIRTIO_ID_CONSOLE	3

/* Feature bits */
#define VIRTIO_CONSOLE_F_SIZE	0	/* Does host provide console size? */

struct virtio_console_config {
	/* colums of the screens */
	__u16 cols;
	/* rows of the screens */
	__u16 rows;
} __packed;
//} __attribute__((packed));


#endif /* _VIRTIO_CONSOLE_H */

/** @} */
