/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Hosted TCP binding
 *
 *
**/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "../../include/kernel/net/tcp.h"


struct tcp_conn
{
	int fd;
};

int tcp_open(void **prot_data)
{
	struct tcp_conn *tc = calloc( 1, sizeof(struct tcp_conn) );
	if( tc == 0 ) return ENOMEM;

	*prot_data = tc;

	return 0;
}



int tcp_bind(void *prot_data, i4sockaddr *addr)
{
	struct sockaddr_in my_addr;

	struct tcp_conn *tc = prot_data;
	if( tc == 0 ) return EINVAL;

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons( addr->port );
	my_addr.sin_addr.s_addr = htonl( NETADDR_TO_IPV4(addr->addr) );

	int rc = bind( tc->fd, (struct sockaddr *) &my_addr, sizeof(my_addr) );

	return rc ? errno : 0;
}


int tcp_connect(void *prot_data, i4sockaddr *addr)
{
	struct sockaddr_in my_addr;

	struct tcp_conn *tc = prot_data;
	if( tc == 0 ) return EINVAL;

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons( addr->port );
	my_addr.sin_addr.s_addr = htonl( NETADDR_TO_IPV4(addr->addr) );

	int rc = connect( tc->fd, (struct sockaddr *) &my_addr, sizeof(my_addr) );

	return rc ? errno : 0;
}

int tcp_listen(void *prot_data)
{
	struct tcp_conn *tc = prot_data;
	return listen( tc->fd, 0 );
}

int tcp_accept(void *prot_data, i4sockaddr *addr, void **new_socket)
{
	struct tcp_conn *tc = prot_data;
	if( tc == 0 ) return EINVAL;

	struct tcp_conn *new_tc = calloc( 1, sizeof(struct tcp_conn) );
	if( new_tc == 0 ) return ENOMEM;

	struct sockaddr_in peer_addr;

    socklen_t peer_addr_size = sizeof(peer_addr);
	int newsock = accept( tc->fd, (struct sockaddr *)&peer_addr, &peer_addr_size );
	if( newsock < 0 ) return errno;

	*new_socket = new_tc;
	new_tc->fd = newsock;

	if( addr )
	{
		if( peer_addr.sin_family != AF_INET )
		{
			printf("tcp_accept: peer_addr.sin_family != AF_INET\n");
			addr->port = 0;
			NETADDR_TO_IPV4(addr->addr) = 0;
		}
		else
		{
			addr->port = ntohs( peer_addr.sin_port );
			NETADDR_TO_IPV4(addr->addr) = ntohl( peer_addr.sin_addr.s_addr );
		}
		
	}

	return 0;
}

int tcp_close(void *prot_data)
{
	struct tcp_conn *tc = prot_data;
	if( tc == 0 ) return 0;

	close( tc->fd );
	free( tc );

	return 0;
}


ssize_t tcp_recvfrom(void *prot_data, void *buf, ssize_t len, i4sockaddr *saddr, int flags, bigtime_t timeout)
{
	(void) saddr;
	struct tcp_conn *tc = prot_data;

	if( flags ) printf("ERROR: hosted tcp_recvfrom w flags %x\n", flags );

	ssize_t rc = read( tc->fd, buf, len );

	return rc;
}

ssize_t tcp_sendto(void *prot_data, const void *buf, ssize_t len, i4sockaddr *addr)
{
	(void) addr;
	struct tcp_conn *tc = prot_data;

	ssize_t rc = write( tc->fd, buf, len );

	return rc;
}

