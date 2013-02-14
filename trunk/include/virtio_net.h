/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Virtio network devices.
 *
 *
**/


#ifndef _VIRTIO_NET_H
#define _VIRTIO_NET_H
/* This header is BSD licensed so anyone can use the definitions to implement
 * compatible drivers/servers. */
#include <phantom_types.h>
#include <virtio_config.h>
#include <sys/cdefs.h>

/** \ingroup Virtio
 *  @{
 */

/* The ID for virtio_net */
#define VIRTIO_ID_NET	1


#define VIRTIO_NET_Q_RECV 0
#define VIRTIO_NET_Q_XMIT 1
#define VIRTIO_NET_Q_CTRL 2

/* The feature bitmap for virtio net */
#define VIRTIO_NET_F_CSUM	0	/* Host handles pkts w/ partial csum */
#define VIRTIO_NET_F_GUEST_CSUM	1	/* Guest handles pkts w/ partial csum */
#define VIRTIO_NET_F_MAC	5	/* Host has given MAC address. */
#define VIRTIO_NET_F_GSO	6	/* Host handles pkts w/ any GSO type */
#define VIRTIO_NET_F_GUEST_TSO4	7	/* Guest can handle TSOv4 in. */
#define VIRTIO_NET_F_GUEST_TSO6	8	/* Guest can handle TSOv6 in. */
#define VIRTIO_NET_F_GUEST_ECN	9	/* Guest can handle TSO[6] w/ ECN in. */
#define VIRTIO_NET_F_GUEST_UFO	10	/* Guest can handle UFO in. */
#define VIRTIO_NET_F_HOST_TSO4	11	/* Host can handle TSOv4 in. */
#define VIRTIO_NET_F_HOST_TSO6	12	/* Host can handle TSOv6 in. */
#define VIRTIO_NET_F_HOST_ECN	13	/* Host can handle TSO[6] w/ ECN in. */
#define VIRTIO_NET_F_HOST_UFO	14	/* Host can handle UFO in. */
#define VIRTIO_NET_F_MRG_RXBUF	15	/* Host can merge receive buffers. */

struct virtio_net_config
{
	/* The config defining mac address (if VIRTIO_NET_F_MAC) */
	u_int8_t mac[6];
} __packed;
//} __attribute__((packed));

/* This is the first element of the scatter-gather list.  If you don't
 * specify GSO or CSUM features, you can simply ignore the header. */
struct virtio_net_hdr
{
#define VIRTIO_NET_HDR_F_NEEDS_CSUM	1	// Use csum_start, csum_offset
	u_int8_t flags;
#define VIRTIO_NET_HDR_GSO_NONE		0	// Not a GSO frame
#define VIRTIO_NET_HDR_GSO_TCPV4	1	// GSO frame, IPv4 TCP (TSO)
#define VIRTIO_NET_HDR_GSO_UDP		3	// GSO frame, IPv4 UDP (UFO)
#define VIRTIO_NET_HDR_GSO_TCPV6	4	// GSO frame, IPv6 TCP
#define VIRTIO_NET_HDR_GSO_ECN		0x80	// TCP has ECN set
	u_int8_t gso_type;
	u_int16_t hdr_len;		/* Ethernet + IP + tcp/udp hdrs */
	u_int16_t gso_size;		/* Bytes to append to hdr_len per frame */
	u_int16_t csum_start;	/* Position to start checksumming from */
	u_int16_t csum_offset;	/* Offset after that to place checksum */
} __packed;
//};

/* This is the version of the header to use when the MRG_RXBUF
 * feature has been negotiated. */
struct virtio_net_hdr_mrg_rxbuf {
	struct virtio_net_hdr hdr;
	u_int16_t num_buffers;	/* Number of merged rx buffers */
} __packed;
//};

#endif /* _VIRTIO_NET_H */

/** @} */
