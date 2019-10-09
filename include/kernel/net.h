/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_NET_H
#define _KERNEL_NET_H

#include <kernel/config.h>


#include <errno.h>
#include <hal.h>

#include <newos/nqueue.h>

#include <compat/newos.h>
#include <newos/cbuf.h>

#include <device.h>

// Becomes nonzero after TCPIP stack is initialized and some network
// card is, possibly, activated.
extern int phantom_tcpip_active;


/* contains common network stuff */

typedef struct netaddr {
	uint8 len;
	uint8 type;
	uint8 pad0;
	uint8 pad1;
	uint8 addr[12];
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

typedef uint32 ipv4_addr;
#define NETADDR_TO_IPV4(naddr) (*(ipv4_addr *)(&((&(naddr))->addr[0])))
#define IPV4_DOTADDR_TO_ADDR(a, b, c, d) \
	(((ipv4_addr)(a) << 24) | (((ipv4_addr)(b) & 0xff) << 16) | (((ipv4_addr)(c) & 0xff) << 8) | ((ipv4_addr)(d) & 0xff))


errno_t parse_ipv4_addr( ipv4_addr *out, const char *str );


#if 0
/* network control ioctls */
enum {
	IOCTL_NET_CONTROL_IF_CREATE = IOCTL_DEVFS_NETWORK_OPS_BASE,
	IOCTL_NET_CONTROL_IF_DELETE,
	IOCTL_NET_CONTROL_IF_ADDADDR,
	IOCTL_NET_CONTROL_IF_RMADDR,
	IOCTL_NET_CONTROL_IF_LIST,
	IOCTL_NET_CONTROL_ROUTE_ADD,
	IOCTL_NET_CONTROL_ROUTE_DELETE,
	IOCTL_NET_CONTROL_ROUTE_LIST,
	IOCTL_NET_IF_GET_ADDR,
	IOCTL_NET_IF_GET_TYPE,
};

/* used in all of the IF control messages */
struct _ioctl_net_if_control_struct {
	char if_name[SYS_MAX_PATH_LEN];
	netaddr if_addr;
	netaddr mask_addr;
	netaddr broadcast_addr;
};

/* used in all of the route control messages */
struct _ioctl_net_route_struct {
	netaddr net_addr;
	netaddr mask_addr;
	netaddr if_addr;
	char if_name[SYS_MAX_PATH_LEN];
};


#define NET_CONTROL_DEV "/dev/net/ctrl"
#endif







typedef struct ifaddr {
	struct ifaddr *next;
	struct ifnet *if_owner;
	netaddr addr;
	netaddr netmask;
	netaddr broadcast;
} ifaddr;

enum {
	IF_TYPE_NULL = 0,
	IF_TYPE_LOOPBACK,
	IF_TYPE_ETHERNET
};

typedef int if_id;

typedef struct ifnet {
	struct ifnet *next;
	if_id id;
	//char path[SYS_MAX_PATH_LEN];
	int type;
        //int fd;

        phantom_device_t *dev;

	thread_id rx_thread;
	thread_id tx_thread;
	ifaddr *addr_list;
        ifaddr *link_addr;
	size_t mtu;
	int (*link_input)(cbuf *buf, struct ifnet *i);
	int (*link_output)(cbuf *buf, struct ifnet *i, netaddr *target, int protocol_type);
	sem_id tx_queue_sem;
    hal_mutex_t tx_queue_lock;

        fixed_queue tx_queue;

        uint8 tx_buf[2048];
	uint8 rx_buf[2048];
} ifnet;



int cmp_netaddr(netaddr *addr1, netaddr *addr2);



ifnet *if_id_to_ifnet(if_id id);
ifnet *if_path_to_ifnet(const char *path);
//int if_register_interface(const char *path, ifnet **i);
int if_boot_interface(ifnet *i);
int if_output(cbuf *b, ifnet *i);

void if_bind_address(ifnet *i, ifaddr *addr);
void if_bind_link_address(ifnet *i, ifaddr *addr);
//! Clear all existing interface addresses and set new one
void if_replace_address(ifnet *i, ifaddr *addr);


int if_register_interface(int type, ifnet **_i, phantom_device_t *dev);


void if_simple_setup( ifnet *interface, int addr, int netmask, int bcast, int net, int router, int def_router );


// for SNMP
int if_get_count(void);




int ipv4_route_add(ipv4_addr network_addr, ipv4_addr netmask, ipv4_addr if_addr, if_id interface_num);
int ipv4_route_add_gateway(ipv4_addr network_addr, ipv4_addr netmask, ipv4_addr if_addr, if_id interface_num, ipv4_addr gw_addr);

// dz - is it correct?
int ipv4_route_add_default(ipv4_addr if_addr, if_id interface_num, ipv4_addr gw_addr);

//! Remove all routes for given interface
errno_t ipv4_route_remove_iface(if_id interface_num);
void ipv4_route_dump(void);


int ipv4_lookup_srcaddr_for_dest(ipv4_addr dest_addr, ipv4_addr *src_addr);
int ipv4_get_mss_for_dest(ipv4_addr dest_addr, uint32 *mss);

int ipv4_input(cbuf *buf, ifnet *i);
int ipv4_output(cbuf *buf, ipv4_addr target_addr, int protocol);


void dump_ipv4_addr(ipv4_addr addr);


int icmp_input(cbuf *buf, ifnet *i, ipv4_addr source_ipaddr);





int loopback_input(cbuf *buf, ifnet *i);
int loopback_output(cbuf *buf, ifnet *i, netaddr *target, int protocol_type);





int loopback_init(void);


int cbuf_init(void);
int ethernet_init(void);
int arp_init(void);
errno_t udp_init(void);
int tcp_init(void);

int ipv4_init(void);
int if_init(void);




int net_timer_init(void);


errno_t bootp(ifnet *iface);


void udp_syslog_send(const char *prefix, const char *message);
void start_tcp_echo_server(void);

errno_t net_curl( const char *url, char *obuf, size_t obufsize, const char *headers );
const char * http_skip_header( const char *buf );

#endif // _KERNEL_NET_H

