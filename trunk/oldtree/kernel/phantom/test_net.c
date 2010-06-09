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
    syslog(LOG_DEBUG|LOG_KERN, "Test of UDP syslog: '%s'", test_parm );
#else
    (void) test_parm;
    SHOW_INFO0( 0, "Warning - no network in kernel, test SKIPPED");
#endif
    return 0;
}

