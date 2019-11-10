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
#include <unistd.h>
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



int tcp_bind(void *prot_data, i4sockaddr *addr);
int tcp_connect(void *prot_data, i4sockaddr *addr);

int tcp_listen(void *prot_data)
{
	struct tcp_conn *tc = prot_data;
	return listen( tc->fd, 0 );
}

int tcp_accept(void *prot_data, i4sockaddr *addr, void **new_socket);

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

