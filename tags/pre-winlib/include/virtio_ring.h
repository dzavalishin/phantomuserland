/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Virtio ring structure definition.
 *
 *
**/


#ifndef _VIRTIO_RING_H
#define _VIRTIO_RING_H
/* An interface for efficient virtio implementation, currently for use by KVM
 * and lguest, but hopefully others soon.  Do NOT change this since it will
 * break existing servers and clients.
 *
 * This header is BSD licensed so anyone can use the definitions to implement
 * compatible drivers/servers.
 *
 * Copyright Rusty Russell IBM Corporation 2007. */
#include <phantom_types.h>
#include <sys/cdefs.h>

/** \ingroup Virtio
 *  @{
 */


/* This marks a buffer as continuing via the next field. */
#define VRING_DESC_F_NEXT	1
/* This marks a buffer as write-only (otherwise read-only). */
#define VRING_DESC_F_WRITE	2

/* The Host uses this in used->flags to advise the Guest: don't kick me when
 * you add a buffer.  It's unreliable, so it's simply an optimization.  Guest
 * will still kick if it's out of buffers. */
#define VRING_USED_F_NO_NOTIFY	1
/* The Guest uses this in avail->flags to advise the Host: don't interrupt me
 * when you consume a buffer.  It's unreliable, so it's simply an
 * optimization.  */
#define VRING_AVAIL_F_NO_INTERRUPT	1

/* Virtio ring descriptors: 16 bytes.  These can chain together via "next". */
struct vring_desc
{
	/* Address (guest-physical). */
	u_int64_t addr;
	/* Length. */
	u_int32_t len;
	/* The flags as indicated above. */
	u_int16_t flags;
	/* We chain unused descriptors via this, too */
	u_int16_t next;
} __packed;

struct vring_avail
{
	u_int16_t flags;
	u_int16_t idx;
	u_int16_t ring[];
} __packed;

/* u32 is used here for ids for padding reasons. */
struct vring_used_elem
{
	/* Index of start of used descriptor chain. */
	u_int32_t id;
	/* Total length of the descriptor chain which was used (written to) */
	u_int32_t len;
} __packed;

struct vring_used
{
	u_int16_t flags;
	u_int16_t idx;
	struct vring_used_elem ring[];
} __packed;

struct vring {
	unsigned int num;

	struct vring_desc *desc;

	struct vring_avail *avail;

	struct vring_used *used;
} __packed;

/* The standard layout for the ring is a continuous chunk of memory which looks
 * like this.  We assume num is a power of 2.
 *
 * struct vring
 * {
 *	// The actual descriptors (16 bytes each)
 *	struct vring_desc desc[num];
 *
 *	// A ring of available descriptor heads with free-running index.
 *	__u16 avail_flags;
 *	__u16 avail_idx;
 *	__u16 available[num];
 *
 *	// Padding to the next align boundary.
 *	char pad[];
 *
 *	// A ring of used descriptor heads with free-running index.
 *	__u16 used_flags;
 *	__u16 used_idx;
 *	struct vring_used_elem used[num];
 * };
 */
static inline void vring_init(struct vring *vr, unsigned int num, void *p,
			      unsigned long align)
{
	vr->num = num;
	vr->desc = p;
	vr->avail = p + num*sizeof(struct vring_desc);
	vr->used = (void *)(((unsigned long)&vr->avail->ring[num] + align-1)
			    & ~(align - 1));
}

static inline unsigned vring_size(unsigned int num, unsigned long align)
{
	return ((sizeof(struct vring_desc) * num + sizeof(u_int16_t) * (2 + num)
		 + align - 1) & ~(align - 1))
		+ sizeof(u_int16_t) * 2 + sizeof(struct vring_used_elem) * num;
}

#ifdef __KERNEL__
#include <linux/irqreturn.h>
struct virtio_device;
struct virtqueue;

struct virtqueue *vring_new_virtqueue(unsigned int num,
				      unsigned int vring_align,
				      struct virtio_device *vdev,
				      void *pages,
				      void (*notify)(struct virtqueue *vq),
				      void (*callback)(struct virtqueue *vq));
void vring_del_virtqueue(struct virtqueue *vq);
/* Filter out transport-specific feature bits. */
void vring_transport_features(struct virtio_device *vdev);

irqreturn_t vring_interrupt(int irq, void *_vq);
#endif /* __KERNEL__ */
#endif /* _VIRTIO_RING_H */

/** @} */
