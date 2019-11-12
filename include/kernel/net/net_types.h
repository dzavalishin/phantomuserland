/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Types for network subsystem
 *
 *
**/

#ifndef _NET_TYPES_H
#define _NET_TYPES_H

#include <stdint.h>

// Becomes nonzero after TCPIP stack is initialized and some network
// card is, possibly, activated.
extern int phantom_tcpip_active;


/* contains common network stuff */

typedef struct netaddr {
	u_int8_t len;
	u_int8_t type;
	u_int8_t pad0;
	u_int8_t pad1;
	u_int8_t addr[12];
} netaddr;

enum {
	SOCK_PROTO_NULL = 0,
	SOCK_PROTO_UDP,
	SOCK_PROTO_TCP
};

enum {
	ADDR_TYPE_NULL = 0,
	ADDR_TYPE_ETHERNET,
	ADDR_TYPE_IP
};

#define SOCK_FLAG_TIMEOUT 1

typedef struct i4sockaddr {
	netaddr addr;
	int port;
} i4sockaddr;

enum {
	IP_PROT_ICMP = 1,
	IP_PROT_TCP = 6,
	IP_PROT_UDP = 17,
};

typedef u_int32_t ipv4_addr;
#define NETADDR_TO_IPV4(naddr) (*(ipv4_addr *)(&((&(naddr))->addr[0])))
#define IPV4_DOTADDR_TO_ADDR(a, b, c, d) \
	(((ipv4_addr)(a) << 24) | (((ipv4_addr)(b) & 0xff) << 16) | (((ipv4_addr)(c) & 0xff) << 8) | ((ipv4_addr)(d) & 0xff))


#endif // _NET_TYPES_H

