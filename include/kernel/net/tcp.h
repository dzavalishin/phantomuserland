/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * TCP in kernel interface.
 *
**/

/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_NET_TCP_H
#define _NEWOS_KERNEL_NET_TCP_H

/*
#include <kernel/net/if.h>
#include <kernel/net/ipv4.h>
#include <kernel/net/socket.h>
#include <kernel/cbuf.h>
*/

//#include <kernel/net.h> // brings too much for hosted code
#include <stdint.h>
#include <kernel/net/net_types.h>
#include <bits/bigtime_t.h>
#include <newos/cbuf.h>

int tcp_open(void **prot_data);
int tcp_bind(void *prot_data, i4sockaddr *addr);
int tcp_connect(void *prot_data, i4sockaddr *addr);
int tcp_listen(void *prot_data);
int tcp_accept(void *prot_data, i4sockaddr *addr, void **new_socket);
int tcp_close(void *prot_data);
ssize_t tcp_recvfrom(void *prot_data, void *buf, ssize_t len, i4sockaddr *saddr, int flags, bigtime_t timeout);
ssize_t tcp_sendto(void *prot_data, const void *buf, ssize_t len, i4sockaddr *addr);

int tcp_getpeername(void *prot_data, i4sockaddr *addr);

#endif

