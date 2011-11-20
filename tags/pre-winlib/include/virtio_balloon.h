
#ifndef _VIRTIO_BALLOON_H
#define _VIRTIO_BALLOON_H
#include <virtio_config.h>
#include <sys/cdefs.h>

/** \ingroup Virtio
 *  @{
 */

/* The ID for virtio_balloon */
#define VIRTIO_ID_BALLOON	5

/* The feature bitmap for virtio balloon */
#define VIRTIO_BALLOON_F_MUST_TELL_HOST	0 /* Tell before reclaiming pages */

/* Size of a PFN in the balloon interface. */
#define VIRTIO_BALLOON_PFN_SHIFT 12

struct virtio_balloon_config
{
	/* Number of pages host wants Guest to give up. */
	__le32 num_pages;
	/* Number of pages we've actually got in balloon. */
	__le32 actual;
} __packed;
#endif /* _VIRTIO_BALLOON_H */

/** @} */
