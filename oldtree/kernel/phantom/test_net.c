/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests - network
 *
 *
**/

#define DEBUG_MSG_PREFIX "test"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include "config.h"

#include <phantom_libc.h>
#include <sys/syslog.h>
#include <errno.h>

#include "net.h"
#include "udp.h"
#include "tcp.h"

#include "test.h"



int do_test_udp_send(const char *test_parm)
{
#if HAVE_NET

    int rc;

    void *prot_data;
    if( udp_open(&prot_data) )
    {
        SHOW_ERROR0(0, "UDP - can't prepare endpoint");
        return ENXIO;
    }

    char buf[] = "UDP request";

    sockaddr addr;
    addr.port = 69; // TFTP

    addr.addr.len = 4;
    addr.addr.type = ADDR_TYPE_IP;

    //NETADDR_TO_IPV4(addr.addr) = IPV4_DOTADDR_TO_ADDR(127, 0, 0, 1);
    NETADDR_TO_IPV4(addr.addr) = IPV4_DOTADDR_TO_ADDR(10, 0, 2, 2); // This is a 'host' in QEMU 'net'

    // It won't assign if address string is null or wrong
    parse_ipv4_addr( &(NETADDR_TO_IPV4(addr.addr)), test_parm );


    if( 0 != (rc = udp_sendto(prot_data, buf, sizeof(buf), &addr)) )
    {
        if(rc == ERR_NET_NO_ROUTE)
        {
            SHOW_ERROR( 0, "UDP - No route (%d)", rc);
            return EHOSTUNREACH;
        }
        else
        {
            SHOW_ERROR( 0, "UDP - can't send, net stack rc = %d", rc);
            return EIO;
        }
    }

    if( (rc = udp_close(prot_data)) )
    {
        SHOW_ERROR( 0, "UDP - close error %d", rc);
        return EIO;
    }


    SHOW_FLOW0( 0, "UDP - done sending");
    return 0;

#else
    (void) test_parm;
    SHOW_INFO0( 0, "Warning - no network in kernel, test SKIPPED");
    return 0;
#endif
}


int do_test_udp_syslog(const char *test_parm)
{
#if HAVE_NET
    if(test_parm == 0 || *test_parm == 0)
        test_parm = "Hello world";
    syslog(LOG_DEBUG|LOG_KERN, "Test of UDP syslog: '%s'", test_parm );
#else
    (void) test_parm;
    SHOW_INFO0( 0, "Warning - no network in kernel, test SKIPPED");
#endif
    return 0;
}


int do_test_tcp_connect(const char *test_parm)
{
#if HAVE_NET
    void *prot_data;

    if(test_parm == 0 || *test_parm == 0)
        test_parm = "87.250.250.3:80";

    int ip0, ip1, ip2, ip3, port;

    if( 5 != sscanf( test_parm, "%d.%d.%d.%d:%d", &ip0, &ip1, &ip2, &ip3, &port ) )
    {
        return 0;
    }

    sockaddr addr;
    addr.port = port;

    addr.addr.len = 4;
    addr.addr.type = ADDR_TYPE_IP;
    NETADDR_TO_IPV4(addr.addr) = IPV4_DOTADDR_TO_ADDR(ip0, ip2, ip2, ip3);


    SHOW_FLOW0( 0, "TCP - create socket");
    if( tcp_open(&prot_data) )
    {
        SHOW_ERROR0(0, "can't prepare endpoint");
    fail:
        return 0;
    }

    SHOW_FLOW0( 0, "TCP - will connect to Yandex");
    if( tcp_connect( prot_data, &addr) )
    {
        SHOW_ERROR(0, "can't connect to %s", test_parm);
        goto fail;
    }
    SHOW_FLOW0( 0, "TCP - connected to Yandex, read");

    char buf[1024];


    memset( buf, 0, sizeof(buf) );
    //strlcpy( buf, "GET /\r\n", sizeof(buf) );
    strlcpy( buf, "GET /\n", sizeof(buf) );
    int nwrite = tcp_sendto( prot_data, buf, 6, &addr);
    SHOW_FLOW( 0, "TCP - write = %d (%s)", nwrite, buf);


    memset( buf, 0, sizeof(buf) );
    int nread = tcp_recvfrom( prot_data, buf, sizeof(buf), &addr, SOCK_FLAG_TIMEOUT, 1000L*1000*10 );
    buf[sizeof(buf)-1] = 0;

    SHOW_FLOW( 0, "TCP - read = %d (%s)", nread, buf);

    tcp_close(prot_data);

#endif
    return 0;
}


