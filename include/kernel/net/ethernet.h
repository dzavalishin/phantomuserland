/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_NET_ETHERNET_H
#define _NEWOS_KERNEL_NET_ETHERNET_H

#include <newos/cbuf.h>
//#include "newos.h"
#include <compat/newos.h>

/*
#define PROT_TYPE_IPV4 0x0800
#define PROT_TYPE_ARP  0x0806

#define ETHERNET_HEADER_SIZE (6+6+2)
#define ETHERNET_MAX_SIZE (ETHERNET_HEADER_SIZE+1500)
#define ETHERNET_MIN_SIZE (ETHERNET_HEADER_SIZE+46)
*/

#include <kernel/ethernet_defs.h>


typedef uint8 ethernet_addr[6];

// not to be called directly, use the ifnet.link_output and link_input
int ethernet_input(cbuf *buf, ifnet *i);
int ethernet_output(cbuf *buf, ifnet *i, netaddr *target, int protocol_type);

//int ethernet_init(void);

void dump_ethernet_addr(ethernet_addr addr);

#endif

