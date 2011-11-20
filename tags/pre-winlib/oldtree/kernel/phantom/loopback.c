#include <kernel/config.h>

#if HAVE_NET

/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <phantom_libc.h>

#include <kernel/net.h>

#include <kernel/net/ethernet.h>
#include <kernel/net/arp.h>

#include "misc.h"



static int _loopback_input(cbuf *buf, ifnet *i, int protocol_type)
{
    int err;

    switch(protocol_type) {
    case PROT_TYPE_IPV4:
        err = ipv4_input(buf, i);
        break;
    case PROT_TYPE_ARP:
        err = arp_input(buf, i);
        break;
    default:
        err = -1;
    }

    return err;
}

int loopback_input(cbuf *buf, ifnet *i)
{
    (void) i;
    // What? you can't call this directly
    cbuf_free_chain(buf);
    return NO_ERROR;
}

int loopback_output(cbuf *buf, ifnet *i, netaddr *target, int protocol_type)
{
    (void) target;

    _loopback_input(buf, i, protocol_type);

    return NO_ERROR;
}

int loopback_init(void)
{
    ifnet *i;
    ifaddr *address;
    int err;

    // set up an initial device
    //err = if_register_interface("loopback", &i);
    err = if_register_interface(IF_TYPE_LOOPBACK, &i,0);
    if(err < 0)
        return err;

    address = kmalloc(sizeof(ifaddr));
    address->addr.type = ADDR_TYPE_NULL;
    address->broadcast.type = ADDR_TYPE_NULL;
    address->netmask.type = ADDR_TYPE_NULL;
    if_bind_link_address(i, address);

    // set the ip address for this net interface
    address = kmalloc(sizeof(ifaddr));
    address->addr.len = 4;
    address->addr.type = ADDR_TYPE_IP;
    NETADDR_TO_IPV4(address->addr) = 0x7f000001; // 127.0.0.1
    address->netmask.len = 4;
    address->netmask.type = ADDR_TYPE_IP;
    NETADDR_TO_IPV4(address->netmask) = 0xff000000; // 255.255.255.0
    address->broadcast.len = 4;
    address->broadcast.type = ADDR_TYPE_IP;
    NETADDR_TO_IPV4(address->broadcast) = 0x7fffffff; // 127.255.255.255
    if_bind_address(i, address);

    // set up an initial routing table
    ipv4_route_add(0x7f000000, 0xff000000, 0x7f000001, i->id);

    return 0;
}

#endif // HAVE_NET

