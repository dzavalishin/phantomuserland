/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_NET_ARP_H
#define _NEWOS_KERNEL_NET_ARP_H

//#include <kernel/net/if.h>
//#include <kernel/net/ipv4.h>
#include <kernel/net.h>
#include <newos/cbuf.h>
#include <compat/newos.h>

enum {
	ARP_CALLBACK_CODE_OK = 0,
	ARP_CALLBACK_CODE_FAILED
};

int arp_input(cbuf *buf, ifnet *i);
//int arp_init(void);
int arp_insert(ipv4_addr ip_addr, netaddr *link_addr);
int arp_lookup(ifnet *i, ipv4_addr sender_ipaddr, ipv4_addr ip_addr, netaddr *link_addr, void (*arp_callback)(int, void *args, ifnet *, netaddr *), void *callback_args);

#endif

