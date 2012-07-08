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
#include <kernel/net.h>
#include <newos/cbuf.h>

int udp_input(cbuf *buf, ifnet *i, ipv4_addr source_address, ipv4_addr target_address);
int udp_open(void **prot_data);
int udp_bind(void *prot_data, i4sockaddr *addr);
errno_t udp_connect(void *prot_data, i4sockaddr *addr);
errno_t udp_listen(void *prot_data);
errno_t udp_accept(void *prot_data, i4sockaddr *addr, void **new_socket);
errno_t udp_close(void *prot_data);

ssize_t udp_recvfrom(void *prot_data, void *buf, ssize_t len, i4sockaddr *saddr, int flags, bigtime_t timeout);
ssize_t udp_sendto(void *prot_data, const void *buf, ssize_t len, i4sockaddr *addr);

#endif

