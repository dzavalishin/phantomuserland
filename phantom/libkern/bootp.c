#if HAVE_NET
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Bootp/DHCP client.
 *
**/


#define DEBUG_MSG_PREFIX "bootp"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_info 0
#define debug_level_error 10


#define DUMP_ROUTES 1


#include <kernel/debug.h>

#include <kernel/net/udp.h>
#include <kernel/libkern.h>
#include <time.h>
#include <errno.h>

// extern struct utsname phantom_uname;
#include <kernel/boot.h>
#include <netinet/resolv.h>


/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>

#define SUPPORT_DHCP

#include <kernel/net.h>
#include <kernel/net/bootp.h>



struct bootp_state
{
    // Results
    unsigned int 	smask;

    struct in_addr 	servip;
    struct in_addr 	gateip;
    struct in_addr 	rootip;
    struct in_addr 	myip;

    struct in_addr 	dns;

    char 		rootpath[256];
    char 		hostname[256];
    char 		bootfile[256];

    // Workplace

    time_t		bot;
    char		mac_addr[6];

    // DHCP
    char 		expected_dhcpmsgtype;
    char 		dhcp_ok;
    struct in_addr 	dhcp_serverip;


};








static	char vm_rfc1048[4] = VM_RFC1048;

/* Local forwards */
static ssize_t bootpsend(struct bootp_state *bstate, void *udp_sock, struct bootp *bp, size_t, ipv4_addr send_addr);
static errno_t bootprecv(struct bootp_state *bstate, void *udp_sock, struct bootp *bp, size_t len, size_t *retlen);
static	int vend_rfc1048(struct bootp_state *bstate, u_char *, u_int);

static int xid = 0;


static void attach_dhcp_option( unsigned char**bufpp, u_int32_t *bufs, char tag, int optlen, void *opt_data )
{
    size_t total = optlen + 2;
    if( total > *bufs )
    {
        LOG_ERROR( 0, "out of space, tag %d", tag );
        return;
    }

    if( optlen >= 255 )
    {
        LOG_ERROR( 0, "optlen %d", optlen );
        return;
    }

    unsigned char *bufp = *bufpp;

    bufp[0] = tag;
    bufp[1] = (unsigned char) optlen;
    
    if( optlen > 0 ) memcpy( bufp+2, opt_data, optlen );

    (*bufpp) += total;
}

/* Fetch required bootp infomation */
static errno_t do_bootp(struct bootp_state *bstate, void *udp_sock, ipv4_addr send_addr)
{
    struct bootp *bp;

    struct {
        //u_char header[HEADER_SIZE];
        struct bootp wbootp;
    } wbuf;

    struct {
        //u_char header[HEADER_SIZE];
        struct bootp rbootp;
    } rbuf;

    LOG_FLOW0( 3, "bootp start");

    if (!bstate->bot)
        bstate->bot = time(0);



    bp = &wbuf.wbootp;
    bzero(bp, sizeof(*bp));

    bp->bp_op = BOOTREQUEST;
    bp->bp_htype = 1;		/* 10Mb Ethernet (48 bits) */
    bp->bp_hlen = 6;
    bp->bp_xid = htonl(xid);

    // [dz] attempt to fight 'ipv4 packet for someone else: 192.168.255.255'
    bp->bp_flags = htons(0x8000); // ask him to broadcast reply
    bp->bp_yiaddr.s_addr = 0xFFFFFFFF; // set my address to full broadcast

    //MACPY(d->myea, bp->bp_chaddr);
    memcpy( bp->bp_chaddr, bstate->mac_addr, 6 );

    //strncpy(bp->bp_file, bootfile, sizeof(bp->bp_file));
    bcopy(vm_rfc1048, bp->bp_vend, sizeof(vm_rfc1048));

    bp->bp_vend[4] = TAG_DHCP_MSGTYPE;
    bp->bp_vend[5] = 1;
    bp->bp_vend[6] = DHCPDISCOVER;

    unsigned char *bufp = bp->bp_vend + 7;
    u_int32_t bufs = sizeof(bp->bp_vend) - ((void *)bufp - (void *)bp->bp_vend);

    bufs--; // leave place for final TAG_END

    //char req[] = { TAG_SUBNET_MASK, TAG_TIME_SERVER, TAG_DOMAIN_SERVER, TAG_LOG_SERVER, TAG_HOSTNAME, TAG_DOMAINNAME };
    char req[] = { TAG_SUBNET_MASK, TAG_GATEWAY, TAG_NTP_SERVER, TAG_DOMAIN_SERVER, TAG_LOG_SERVER, TAG_HOSTNAME, TAG_DOMAINNAME };

    attach_dhcp_option( &bufp, &bufs, TAG_PARAM_REQ, sizeof(req), req );

    *bufp++ = TAG_END;

    size_t filled = ((void *)bufp - (void *)bp);

# if 0
    /*
     * If we are booting from PXE, we want to send the string
     * 'PXEClient' to the DHCP server so you have the option of
     * only responding to PXE aware dhcp requests.
     */
    if (flag & BOOTP_PXE) {
        bp->bp_vend[7] = TAG_CLASSID;
        bp->bp_vend[8] = 9;
        bcopy("PXEClient", &bp->bp_vend[9], 9);
        bp->bp_vend[18] = TAG_END;
    } else
        bp->bp_vend[7] = TAG_END;
//#else
//    bp->bp_vend[4] = TAG_END;
//#endif
#endif


    //d->myip.s_addr = INADDR_ANY;
    //d->myport = htons(IPPORT_BOOTPC);
    //d->destip.s_addr = INADDR_BROADCAST;
    //d->destport = htons(IPPORT_BOOTPS);

    bstate->expected_dhcpmsgtype = DHCPOFFER;
    bstate->dhcp_ok = 0;

    i4sockaddr src_addr;
    src_addr.port = IPPORT_BOOTPC; // local port

    src_addr.addr.len = 4;
    src_addr.addr.type = ADDR_TYPE_IP;
    // INADDR_ANY
    NETADDR_TO_IPV4(src_addr.addr) = IPV4_DOTADDR_TO_ADDR(0, 0, 0, 0);

    if( 0 != udp_bind(udp_sock, &src_addr) )
    {
        LOG_ERROR0( 0, "can't bind UDP address");
        return ENOTCONN;
    }

    errno_t rc;

    //if( 0 != bootpsend(bstate, udp_sock, bp, sizeof(*bp), send_addr) )
    if( 0 != bootpsend(bstate, udp_sock, bp, filled, send_addr) )
    {
        LOG_ERROR0( 0, "can't send UDP packet");
        return ECONNREFUSED;
    }

    rc = bootprecv(bstate, udp_sock, &rbuf.rbootp, sizeof(rbuf.rbootp), 0 );
    if(rc)
    {
        LOG_ERROR( 0, "no reply, rc = %d", rc);
        return rc;
    }


    if(bstate->dhcp_ok)
    {
        LOG_FLOW( 1, "DHCP ok, now will send request. xid = 0x%X", rbuf.rbootp.bp_xid );

        u_int32_t leasetime;
        bp->bp_vend[6] = DHCPREQUEST;
        bp->bp_vend[7] = TAG_REQ_ADDR;
        bp->bp_vend[8] = 4;
        bcopy(&rbuf.rbootp.bp_yiaddr, &bp->bp_vend[9], 4);
        bp->bp_vend[13] = TAG_SERVERID;
        bp->bp_vend[14] = 4;
        bcopy(&bstate->dhcp_serverip.s_addr, &bp->bp_vend[15], 4);
        bp->bp_vend[19] = TAG_LEASETIME;
        bp->bp_vend[20] = 4;
        leasetime = htonl(30000);
        bcopy(&leasetime, &bp->bp_vend[21], 4);
        /*        
        if (flag & BOOTP_PXE) {
            bp->bp_vend[25] = TAG_CLASSID;
            bp->bp_vend[26] = 9;
            bcopy("PXEClient", &bp->bp_vend[27], 9);
            bp->bp_vend[36] = TAG_END;
        } else
        */
            bp->bp_vend[25] = TAG_END;

        bstate->expected_dhcpmsgtype = DHCPACK;

        if( 0 != bootpsend(bstate, udp_sock, bp, sizeof(*bp), send_addr) )
        {
            LOG_ERROR0( 0, "can't send UDP 2");
            return ECONNREFUSED;
        }

        if( bootprecv(bstate, udp_sock, &rbuf.rbootp, sizeof(rbuf.rbootp), 0) )
        {
            LOG_ERROR0( 0, "no reply");
            return ETIMEDOUT;
        }

    }
//#endif

    bstate->myip = rbuf.rbootp.bp_yiaddr;
    bstate->servip = rbuf.rbootp.bp_siaddr;
    //if(rootip.s_addr == INADDR_ANY) rootip = servip;

    // coverity.com doesn't understand bcopy
    memcpy( bstate->bootfile, rbuf.rbootp.bp_file, umin( sizeof(bstate->bootfile), sizeof(rbuf.rbootp.bp_file) ) );
    bstate->bootfile[sizeof(bstate->bootfile) - 1] = '\0';


    //LOG_FLOW( 1, "mask: %s", intoa(smask));

    /* We need a gateway if root is on a different net * /
    if (!SAMENET(myip, rootip, netmask)) {
        LOG_FLOW( 1, "need gateway for root ip");
    }

    / * Toss gateway if on a different net * /
    if (!SAMENET(myip, gateip, netmask)) {
        LOG_FLOW( 1, "gateway ip (%s) bad", inet_ntoa(gateip));
        gateip.s_addr = 0;
    } */

    /* Bump xid so next request will be unique. */
    ++xid;

    return 0;
}

/**
 * @brief Transmit a bootp request 
 * 
 * @param[in] send_addr Address to send to. Must be full broadcast or interface broadcast.
 * 
**/
static ssize_t
bootpsend(struct bootp_state *bstate, void *udp_sock, struct bootp *bp, size_t len, ipv4_addr send_addr)
{
    bp->bp_secs = htons((u_short)(time(0) - bstate->bot));

    LOG_FLOW( 3, "send udp %d bytes", len );

    i4sockaddr dest_addr;
    dest_addr.port = IPPORT_BOOTPS; // dest port
    dest_addr.addr.len = 4;
    dest_addr.addr.type = ADDR_TYPE_IP;
    NETADDR_TO_IPV4(dest_addr.addr) = send_addr;//IPV4_DOTADDR_TO_ADDR(0xFF, 0xFF, 0xFF, 0xFF);

    int rc = udp_sendto(udp_sock, bp, len, &dest_addr);
    if( 0 != rc )
    {
        LOG_ERROR( 0, "can't send UDP, rc = %d", rc);
        return -1;
    }

    return 0;
}

static errno_t
bootprecv( struct bootp_state *bstate, void *udp_sock, struct bootp *bp, size_t len, size_t *retlen )
{
    LOG_FLOW( 3, "wait for %d bytes", len );

    i4sockaddr dest_addr;
    dest_addr.port = IPPORT_BOOTPS;
    dest_addr.addr.len = 4;
    dest_addr.addr.type = ADDR_TYPE_IP;
    NETADDR_TO_IPV4(dest_addr.addr) = IPV4_DOTADDR_TO_ADDR(0, 0, 0, 0);

    //int n = udp_recvfrom(udp_sock, bp, len, &dest_addr, SOCK_FLAG_TIMEOUT, 2000000l);
    int n = udp_recvfrom(udp_sock, bp, len, &dest_addr, SOCK_FLAG_TIMEOUT, 6000000l);

    if( 0 >= n )
    {
        LOG_ERROR( 0, "UDP recv err = %d", n);
        return ETIMEDOUT; // TODO errno
    }

    LOG_INFO_( 9, "UDP msg from:      %s", inet_itoa(* ((u_int32_t *)&dest_addr.addr) ) );

    if (n == -1 || n < (int)(sizeof(struct bootp) - BOOTP_VENDSIZE))
        goto bad;

    LOG_FLOW( 3, "recv %d bytes", n);

    if (bp->bp_xid != htonl(xid)) {
        LOG_ERROR( 1, "expected xid 0x%x, got 0x%x",
                   xid, ntohl(bp->bp_xid));
        goto bad;
    }

    LOG_FLOW0( 3, "bootprecv: got one!");

    /* Suck out vendor info */
    if (bcmp(vm_rfc1048, bp->bp_vend, sizeof(vm_rfc1048)) == 0) {
        if(vend_rfc1048(bstate, bp->bp_vend, sizeof(bp->bp_vend)) != 0)
            goto bad;
    }
    else
        LOG_ERROR( 0, "bootprecv: unknown vendor 0x%lx", (long)bp->bp_vend);

    if(retlen) *retlen = n;

    return 0;
bad:
    //errno = 0;
    return EINVAL;
}


/**
 * @brief Decode DHCP tagged data.
 * 
 * 
**/

static int
vend_rfc1048(struct bootp_state *bstate, u_char *cp, u_int len)
{
    u_char *ep;
    u_int32_t size;
    u_char tag;

    LOG_FLOW( 3, "bootp info. len=%d", len);

    ep = cp + len;

    /* Step over magic cookie */
    //cp += sizeof(int);
    cp += 4;

    while (cp < ep) {
        tag = *cp++;
        size = *cp++;
        if (tag == TAG_END)
            break;

        LOG_FLOW( 4, "tag = %d, size = %d", tag, size );

        switch(tag)
        {
        case TAG_DOMAIN_SERVER:
            bcopy(cp, &bstate->dns.s_addr, sizeof(bstate->dns.s_addr));
            break;

        //case TAG_TIME_SERVER:            break;

        case TAG_LOG_SERVER:
            hexdump(cp, size, "Log", 0 );
            break;

        case TAG_NTP_SERVER:
            {
                //hexdump(cp, size, "NTP", 0 );
                int left = size;
                u_int32_t *ipp = (u_int32_t *)cp;
                while( left > 4 )
                {
                    left -= 4;
                    u_int32_t ntp_ip = htonl( *ipp++ );
                    LOG_INFO_( 1, "NTP server %s", inet_itoa( ntp_ip ) );
                // TODO SNTP is Posix app, how do we pass info? Environment?
                // phantom_setenv( const char *name, const char *value )
                // TODO object land may be not yet inited, can't call setenv, need some
                // rendevouz gear to meet kernel and object land
                }
            }
            break;

        case TAG_LEASETIME:
            LOG_ERROR( 1, "Ignored lease time %u sec", htonl( *((u_int32_t*)cp) ) );
            break;

        case TAG_SUBNET_MASK:
            bcopy(cp, &bstate->smask, sizeof(bstate->smask));
            LOG_INFO_( 5, "recv smask 0x%x", bstate->smask );
            break;

        case TAG_GATEWAY:
            bcopy(cp, &bstate->gateip.s_addr, sizeof(bstate->gateip.s_addr));
            break;

        case TAG_SWAPSERVER:
            /* let it override bp_siaddr */
            bcopy(cp, &bstate->rootip.s_addr, sizeof(bstate->rootip.s_addr));
            break;

        case TAG_ROOTPATH:
            strlcpy(bstate->rootpath, (char *)cp, umin( size, sizeof(bstate->rootpath) ) );
            break;

        case TAG_HOSTNAME:
            strlcpy(bstate->hostname, (char *)cp, umin( size, sizeof(bstate->hostname) ) );
            break;

        case TAG_DHCP_MSGTYPE:
            if(*cp != bstate->expected_dhcpmsgtype)
            {
                LOG_ERROR( 1, "Unexpected DHCP msg type, %d instead of %d", *cp, bstate->expected_dhcpmsgtype );
                return -1;
            }
            bstate->dhcp_ok = 1;
            break;

        case TAG_SERVERID:
            bcopy(cp, &bstate->dhcp_serverip.s_addr,
                  sizeof(bstate->dhcp_serverip.s_addr));
            break;

        default:
            LOG_ERROR( 0, "unknown tag = %d", tag );
            break;
        }


        cp += size;
    }

    return 0;
}








errno_t bootp(ifnet *iface)
{
    struct bootp_state          _bstate;
    void *			             udp_sock;
    struct bootp_state          *bstate = &_bstate;

    ipv4_addr                    send_addr;

    //send_addr = NETADDR_TO_IPV4(iface->addr_list->broadcast);
    send_addr = 0xFFFFFFFF; // Just broadcast, we have hack in kernel to use 1st avail iface for it

    memset( &_bstate, 0, sizeof(struct bootp_state) );

    _bstate.expected_dhcpmsgtype = -1;


    int err = iface->dev->dops.get_address(iface->dev, &_bstate.mac_addr, sizeof(_bstate.mac_addr));
    if(err < 0) {
        LOG_ERROR0( 0, "can't get interface MAC address");
        return ENXIO;
    }


    if( xid == 0 )
        xid = (int)time(0) ^ 0x1E0A4F; // Some strange number :)

    int tries = 3;
    errno_t e;

    do {
        if( udp_open(&udp_sock) )
        {
            LOG_ERROR0( 0, "UDP - can't prepare endpoint");
            return ENOTSOCK;
        }

        e = do_bootp( bstate, udp_sock, send_addr);

        udp_close(udp_sock);

    } while( e && (tries-- > 0) );


    if(e)
        LOG_ERROR( 0, "error %d", e);
    else
    {
        LOG_INFO_( 1, "DHCP netmask: 0x%08X", htonl(bstate->smask) );

        LOG_INFO_( 1, "DHCP ip:      %s", inet_ntoa(bstate->myip) );
        LOG_INFO_( 1, "gateway ip:   %s", inet_ntoa(bstate->gateip) );
        LOG_INFO_( 2, "root ip:      %s", inet_ntoa(bstate->rootip) );
        LOG_INFO_( 2, "server ip:    %s", inet_ntoa(bstate->servip) );

        LOG_INFO_( 1, "DNS ip:       %s", inet_ntoa(bstate->dns) );

        LOG_INFO_( 2, "rootpath:     '%s'", bstate->rootpath );
        LOG_INFO_( 1, "hostname:     '%s'", bstate->hostname );
        LOG_INFO_( 2, "bootfile:     '%s'", bstate->bootfile );

        // Now apply it to interface
        // TODO use if_simple_setup instead!
        //u_int32_t net_mask = htonl(~(bstate->smask));
        u_int32_t net_mask = htonl(bstate->smask);

        //u_int32_t net = (bstate->myip.s_addr) & ~(bstate->smask);
        u_int32_t net = htonl(bstate->myip.s_addr) & net_mask;
        LOG_INFO_( 1, "Net ip:        %s", inet_itoa(htonl(net)) );
        //LOG_INFO_( 1, "Net ip:        0x%X", net );


        ifaddr *address;

        // set the ip address for this net interface
        address = malloc(sizeof(ifaddr));

        address->addr.len = 4;
        address->addr.type = ADDR_TYPE_IP;
        NETADDR_TO_IPV4(address->addr) = htonl(bstate->myip.s_addr);

        address->netmask.len = 4;
        address->netmask.type = ADDR_TYPE_IP;
        NETADDR_TO_IPV4(address->netmask) = net_mask; //htonl(bstate->smask);

        u_int32_t bcast = htonl(bstate->myip.s_addr) | ~net_mask; //bstate->smask;
        LOG_INFO_( 2, "Broadcast addr = %08x ", bcast );

        address->broadcast.len = 4;
        address->broadcast.type = ADDR_TYPE_IP;
        NETADDR_TO_IPV4(address->broadcast) = bcast;

#if DUMP_ROUTES
        ipv4_route_dump();
#endif

        //if_bind_address(iface, address);
        if_replace_address(iface, address); // kill al prev addresses and use new one

        // TODO why the hell htonl here? Must ne ntohl instead?

        errno_t e = ipv4_route_remove_iface(iface->id);
        if(e) LOG_ERROR( 1, "Removilg routes to iface - failed, rc = %d", e );

        int rc;
        if( (rc = ipv4_route_add( net, net_mask, htonl(bstate->myip.s_addr), iface->id) ) )
        {
            LOG_ERROR( 1, "Adding route mask %08x - failed, rc = %d", net_mask, rc);
        }
        else
        {
            LOG_INFO_( 2, "Adding route mask %08x - ok", net_mask );
        }

        u_int32_t gate = htonl(bstate->gateip.s_addr);
        //LOG_INFO_( 1, "gate IP 0x%x", gate );
        LOG_INFO_( 1, "gate IP: %s", inet_itoa(gate) );

        if( gate == 0 ) LOG_ERROR0( 1, "Can't add default route, no gate addr" );        
        else if( (rc = ipv4_route_add_default( htonl(bstate->myip.s_addr), iface->id, gate ) ) )
        {
            LOG_ERROR( 1, "Adding default route - failed, rc = %d", rc);
        }
        else
        {
            LOG_INFO_( 2, "Adding default route 0x%X - ok", gate );
        }


#if DUMP_ROUTES
        ipv4_route_dump();
#endif

        // At least one char!
        if(*bstate->hostname)
            strlcpy( phantom_uname.nodename, bstate->hostname, _UTSNAME_NODENAME_LENGTH );

        if( bstate->dns.s_addr )
        {
            LOG_FLOW( 2, "Add DNS server %s", inet_ntoa(bstate->dns) );
            //e = dns_server_add( htonl(bstate->dns.s_addr) );
            e = dns_server_add( bstate->dns.s_addr );
            if( e ) LOG_ERROR( 1, "Adding DNS server %s failed, rc = %d", inet_ntoa(bstate->dns), e );
        }
    }


    return e;
}








#endif // HAVE_NET


