/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Virtio block devices.
 *
 *
**/

#ifndef _VIRTIO_BLK_H
#define _VIRTIO_BLK_H
/* This header is BSD licensed so anyone can use the definitions to implement
 * compatible drivers/servers. */
#include <phantom_types.h>
#include <virtio_config.h>
#include <sys/cdefs.h>

/** \ingroup Virtio
 *  @{
 */


/* The ID for virtio_block */
#define VIRTIO_ID_BLOCK	2

/* Feature bits */
#define VIRTIO_BLK_F_BARRIER	0	/* Does host support barriers? */
#define VIRTIO_BLK_F_SIZE_MAX	1	/* Indicates maximum segment size */
#define VIRTIO_BLK_F_SEG_MAX	2	/* Indicates maximum # of segments */
#define VIRTIO_BLK_F_GEOMETRY	4	/* Legacy geometry available  */
#define VIRTIO_BLK_F_RO		5	/* Disk is read-only */
#define VIRTIO_BLK_F_BLK_SIZE	6	/* Block size of disk is available*/

struct virtio_blk_config
{
	/* The capacity (in 512-byte sectors). */
	u_int64_t capacity;
	/* The maximum segment size (if VIRTIO_BLK_F_SIZE_MAX) */
	u_int32_t size_max;
	/* The maximum number of segments (if VIRTIO_BLK_F_SEG_MAX) */
	u_int32_t seg_max;
	/* geometry the device (if VIRTIO_BLK_F_GEOMETRY) */
	struct virtio_blk_geometry {
		u_int16_t cylinders;
		u_int8_t heads;
		u_int8_t sectors;
	} geometry;
	/* block size of device (if VIRTIO_BLK_F_BLK_SIZE) */
	u_int32_t blk_size;
} __packed;
//} __attribute__((packed));

/* These two define direction. */
#define VIRTIO_BLK_T_IN		0
#define VIRTIO_BLK_T_OUT	1

/* This bit says it's a scsi command, not an actual read or write. */
#define VIRTIO_BLK_T_SCSI_CMD	2

/* Barrier before this op. */
#define VIRTIO_BLK_T_BARRIER	0x80000000

/* This is the first element of the read scatter-gather list. */
struct virtio_blk_outhdr
{
	/* VIRTIO_BLK_T* */
	u_int32_t type;
	/* io priority. */
	u_int32_t ioprio;
	/* Sector (ie. 512 byte offset) */
	u_int64_t sector;
} __packed;

/* And this is the final byte of the write scatter-gather list. */
#define VIRTIO_BLK_S_OK		0
#define VIRTIO_BLK_S_IOERR	1
#define VIRTIO_BLK_S_UNSUPP	2


struct virtio_blk_inhdr
{
      unsigned char status;
} __packed;


#endif /* _VIRTIO_BLK_H */

/** @} */
