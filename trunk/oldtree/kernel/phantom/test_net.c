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
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>

#include <phantom_libc.h>
#include <sys/syslog.h>
#include <errno.h>

#include <netinet/resolv.h>

#include <kernel/net.h>
#include <kernel/net/udp.h>
#include <kernel/net/tcp.h>

#include "test.h"


int do_test_cbuf(const char *test_parm)
{
    cbuf *buf, *buf2;
    char temp[1024];
    unsigned int i;

    (void) test_parm;

    SHOW_FLOW0( 0, "starting cbuffer test");

    buf = cbuf_get_chain(32);
    if(!buf)
        test_fail_msg( EINVAL, "cbuf_test: failed allocation of 32");

    buf2 = cbuf_get_chain(3*1024*1024);
    if(!buf2)
        test_fail_msg( EINVAL, "cbuf_test: failed allocation of 3mb");

    buf = cbuf_merge_chains(buf2, buf);

    test_check_eq( 3*1024*1024 + 32, cbuf_get_len(buf) );

    cbuf_free_chain(buf);

    SHOW_FLOW0( 0, "allocating too much...");

    buf = cbuf_get_chain(128*1024*1024);
    if(buf)
        test_fail_msg( EINVAL, "cbuf_test: should have failed to allocate 128mb");

    SHOW_FLOW0( 0, "touching memory allocated by cbufn");

    buf = cbuf_get_chain(7*1024*1024);
    if(!buf)
        test_fail_msg( EINVAL, "cbuf_test: failed allocation of 7mb");

    for(i=0; i < sizeof(temp); i++)
        temp[i] = i;
    for(i=0; i<7*1024*1024 / sizeof(temp); i++) {
        //if(i % 128 == 0) dprintf("%Lud\n", (long long)(i*sizeof(temp)));
        cbuf_memcpy_to_chain(buf, i*sizeof(temp), temp, sizeof(temp));
    }
    cbuf_free_chain(buf);

    SHOW_FLOW0( 0, "finished cbuffer test");

    return 0;
}



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




int do_test_resolver(const char *test_parm)
{
    (void) test_parm;
#if HAVE_NET
    in_addr_t out;

    // Force resolver to do it from DNS, not from cache
    test_check_eq( 0, name2ip( &out, "ya.ru", RESOLVER_FLAG_NORCACHE ) );

    // Now do from cache
    test_check_eq( 0, name2ip( &out, "ya.ru", RESOLVER_FLAG_NOWAIT ) );

    // TODO compare results
#endif
    return 0;
}










#include <netinet/tftp.h>



#if HAVE_NET
static int run_tftp_test(void *prot_data)
{
    int rc;
    char buf[1024];


    sockaddr addr;
    addr.port = 1069; // local port

    addr.addr.len = 4;
    addr.addr.type = ADDR_TYPE_IP;
    NETADDR_TO_IPV4(addr.addr) = IPV4_DOTADDR_TO_ADDR(10, 0, 2, 2);

    if( 0 != (rc = udp_bind(prot_data, &addr)) )
        return rc;

    addr.port = 69; // TFTP


#define RRQ "__tftp/menu.lst\0octet"

    memcpy( buf, RRQ, sizeof(RRQ) );


#if 0
    printf("TFTP invalid RRQ...");
    if( 0 != (rc = udp_sendto(prot_data, buf, sizeof(RRQ), &addr)) )
        return rc;

    hal_sleep_msec(500);
#endif

    ((struct tftp_t*)buf)->opcode = htons(1); // RRQ
    SHOW_FLOW0( 1, "TFTP RRQ...");

    if( 0 != (rc = udp_sendto(prot_data, buf, sizeof(RRQ), &addr)) )
    {
        if(rc == ERR_NET_NO_ROUTE)
        {
            SHOW_ERROR( 0, "UDP tftp - No route\n", rc);
        }
        else
            SHOW_ERROR( 0, "UDP tftp - can't send, rc = %d\n", rc);

        return rc;
    }


    while(1)
    {

        SHOW_FLOW0( 1, "TFTP recv...");
        if( 0 >= (rc = udp_recvfrom(prot_data, buf, sizeof(buf), &addr, 0, 0)) )
            return rc;
        SHOW_FLOW( 1, "TFTP GOT PKT \"%s\"", ((struct tftp_t*)buf)->u.data.download );

        if( rc < 4 )
        {
            SHOW_ERROR0( 0, "TFTP too short pkt received\n");
            return -1;
        }

        int opcode, nblock;
        opcode = ntohs( ((struct tftp_t*)buf)->opcode );
        nblock = ntohs( ((struct tftp_t*)buf)->u.ack.block );


        if( opcode != 3 )
        {
            if( opcode == 5 )
            {
                SHOW_ERROR( 0, "TFTP got error: %d (%s)...", nblock, buf+4);
            }
            else
                SHOW_ERROR( 0, "TFTP error: got opcode %d...", opcode);
            return -1;
        }

        SHOW_FLOW( 1, "TFTP blk %d...", nblock);

        if( rc < 4+512 )
        {
            printf("TFTP finish...");
            return 0;
        }


        ((struct tftp_t*)buf)->opcode = htons(4); // ACK
        ((struct tftp_t*)buf)->u.ack.block = htons(nblock); //orig block

       if( 0 != (rc = udp_sendto(prot_data, buf, 4, &addr)) )
           return rc;
    }

}


int do_test_tftp(const char *test_parm)
{
    (void) test_parm;

    void *prot_data;

    printf("\ntftp start... ");
    if( udp_open(&prot_data) )
    {
        SHOW_ERROR0( 0, "UDP - can't prepare endpoint");
        return EAGAIN;
    }

    int rc;
    if( (rc = run_tftp_test(prot_data)) )
        SHOW_ERROR( 0 , "\ntftp failed, rc = %d\n", rc);

    udp_close(prot_data);

    return rc;
}
#else
int do_test_tftp(const char *test_parm)
{
    (void) test_parm;
    return 0;
}
#endif // HAVE_NET
























int do_test_tcp_connect(const char *test_parm)
{
#if HAVE_NET
    void *prot_data;

    if(test_parm == 0 || *test_parm == 0)
        //test_parm = "87.250.250.3:80";
        //test_parm = "93.158.134.3:80";
        //test_parm = "93.158.134.203:80";
        //test_parm = "173.194.32.16:80"; // google.com
        test_parm = "89.108.110.118:80"; // misc.dz.ru

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


    SHOW_FLOW( 0, "TCP - create socket to %d.%d.%d.%d port %d", ip0, ip2, ip2, ip3, port);
    if( tcp_open(&prot_data) )
    {
        SHOW_ERROR0(0, "can't prepare endpoint");
    fail:
        return EIO;
    }

    SHOW_FLOW0( 0, "TCP - will connect");
    if( tcp_connect( prot_data, &addr) )
    {
        SHOW_ERROR(0, "can't connect to %s", test_parm);
        goto fail;
    }
    SHOW_FLOW0( 0, "TCP - connected, read");

    char buf[1024];


    memset( buf, 0, sizeof(buf) );
    strlcpy( buf, "GET /\r\n\r\n", sizeof(buf) );
    //snprintf( buf, sizeof(buf), "GET / HTTP/1.1\r\nHost: ya.ru\r\nUser-Agent: PhantomOSNetTest/0.1 (PhanomOS i686; ru)\r\nAccept: text/html\r\nConnection: close\r\n\r\n" );
    int nwrite = tcp_sendto( prot_data, buf, strlen(buf), &addr);
    SHOW_FLOW( 0, "TCP - write = %d (%s)", nwrite, buf);

    memset( buf, 0, sizeof(buf) );
    int nread = tcp_recvfrom( prot_data, buf, sizeof(buf), &addr, SOCK_FLAG_TIMEOUT, 1000L*1000*50 );
    buf[sizeof(buf)-1] = 0;

    SHOW_FLOW( 0, "TCP - read = %d (%s)", nread, buf);

    tcp_close(prot_data);

    return nread > 0 ? 0 : -nread;
#else
    (void) test_parm;
    return 0;
#endif
}


