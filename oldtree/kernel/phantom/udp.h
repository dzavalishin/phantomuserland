/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_NET_UDP_H
#define _NEWOS_KERNEL_NET_UDP_H

/*
#include <kernel/net/if.h>
#include <kernel/net/ipv4.h>
#include <kernel/net/socket.h>
*/
#include "net.h"
#include "cbuf.h"

int udp_input(cbuf *buf, ifnet *i, ipv4_addr source_address, ipv4_addr target_address);
int udp_open(void **prot_data);
int udp_bind(void *prot_data, sockaddr *addr);
int udp_connect(void *prot_data, sockaddr *addr);
int udp_listen(void *prot_data);
int udp_accept(void *prot_data, sockaddr *addr, void **new_socket);
int udp_close(void *prot_data);
ssize_t udp_recvfrom(void *prot_data, void *buf, ssize_t len, sockaddr *saddr, int flags, bigtime_t timeout);
ssize_t udp_sendto(void *prot_data, const void *buf, ssize_t len, sockaddr *addr);
//int udp_init(void);

#endif

