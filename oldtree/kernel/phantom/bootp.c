/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Bootp/DHCP client.
 *
**/


#define DEBUG_MSG_PREFIX "bootp"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include "udp.h"
#include <time.h>
#include <errno.h>


/*	$NetBSD: bootp.c,v 1.14 1998/02/16 11:10:54 drochner Exp $	*/

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
//__FBSDID("$FreeBSD: src/lib/libstand/bootp.c,v 1.6.6.1 2008/11/25 02:59:29 kensmith Exp $");

//#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <netinet/in_systm.h>

#include <string.h>

#define BOOTP_DEBUGxx
#define SUPPORT_DHCP

//#include "stand.h"
#include "net.h"
//#include "netif.h"
#include "bootp.h"


static struct in_addr servip;
static struct in_addr gateip;
static struct in_addr rootip;
static struct in_addr myip;


static char rootpath[1024];
static char hostname[1024];
static char bootfile[1024];





//static n_long	nmask, smask;
//static long	nmask, smask;
//static u_int32_t smask;
static unsigned int smask;

static time_t	bot;

static	char vm_rfc1048[4] = VM_RFC1048;
#ifdef BOOTP_VEND_CMU
static	char vm_cmu[4] = VM_CMU;
#endif

/* Local forwards */
static	ssize_t bootpsend(void *udp_sock, struct bootp *bp, size_t);
static	ssize_t bootprecv(void *udp_sock, struct bootp *bp, size_t);
static	int vend_rfc1048(u_char *, u_int);
#ifdef BOOTP_VEND_CMU
static	void vend_cmu(u_char *);
#endif

#ifdef SUPPORT_DHCP
static char expected_dhcpmsgtype = -1, dhcp_ok;
struct in_addr dhcp_serverip;
#endif


static int xid = 0;


/* Fetch required bootp infomation */
static errno_t do_bootp(void *udp_sock, void *mac_addr, int flag)
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

    SHOW_FLOW0( 1, "bootp start");

    if (!bot)
        bot = time(0);



    bp = &wbuf.wbootp;
    bzero(bp, sizeof(*bp));

    bp->bp_op = BOOTREQUEST;
    bp->bp_htype = 1;		/* 10Mb Ethernet (48 bits) */
    bp->bp_hlen = 6;
    bp->bp_xid = htonl(xid);

    //MACPY(d->myea, bp->bp_chaddr);
    memcpy( bp->bp_chaddr, mac_addr, 6 );

    //strncpy(bp->bp_file, bootfile, sizeof(bp->bp_file));
    bcopy(vm_rfc1048, bp->bp_vend, sizeof(vm_rfc1048));
#ifdef SUPPORT_DHCP
    bp->bp_vend[4] = TAG_DHCP_MSGTYPE;
    bp->bp_vend[5] = 1;
    bp->bp_vend[6] = DHCPDISCOVER;

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
#else
    bp->bp_vend[4] = TAG_END;
#endif

    //d->myip.s_addr = INADDR_ANY;
    //d->myport = htons(IPPORT_BOOTPC);
    //d->destip.s_addr = INADDR_BROADCAST;
    //d->destport = htons(IPPORT_BOOTPS);

#ifdef SUPPORT_DHCP
    expected_dhcpmsgtype = DHCPOFFER;
    dhcp_ok = 0;
#endif


    sockaddr src_addr;
    src_addr.port = IPPORT_BOOTPC; // local port

    src_addr.addr.len = 4;
    src_addr.addr.type = ADDR_TYPE_IP;
    // INADDR_ANY
    NETADDR_TO_IPV4(src_addr.addr) = IPV4_DOTADDR_TO_ADDR(0, 0, 0, 0);

    if( 0 != udp_bind(udp_sock, &src_addr) )
    {
        SHOW_ERROR0( 0, "can't bind UDP address");
        return ENOTCONN;
    }

    /*
    sockaddr dest_addr;
    dest_addr.port = IPPORT_BOOTPS; // dest port

    dest_addr.addr.len = 4;
    dest_addr.addr.type = ADDR_TYPE_IP;
    // INADDR_BROADCAST
    NETADDR_TO_IPV4(dest_addr.addr) = IPV4_DOTADDR_TO_ADDR(0xFF, 0xFF, 0xFF, 0xFF);
    */

    //if( 0 != udp_sendto(udp_sock, bp, sizeof(*bp), &dest_addr) )
    if( 0 != bootpsend(udp_sock, bp, sizeof(*bp)) )
    {
        SHOW_ERROR0( 0, "can't send UDP");
        return ECONNREFUSED;
    }

    //if( 0 >= udp_recvfrom(udp_sock, &rbuf.rbootp, sizeof(rbuf.rbootp), &dest_addr, SOCK_FLAG_TIMEOUT, 1000000l) )
    if( 0 >= bootprecv(udp_sock, &rbuf.rbootp, sizeof(rbuf.rbootp)) )
    {
        SHOW_ERROR0( 0, "no reply");
        return ETIMEDOUT;
    }

    


    /*
    if(sendrecv(d,
                bootpsend, bp, sizeof(*bp),
                bootprecv, &rbuf.rbootp, sizeof(rbuf.rbootp))
       == -1) {
        SHOW_ERROR0( 0, "no reply");
        return;
    }*/

#ifdef SUPPORT_DHCP
    if(dhcp_ok)
    {
        u_int32_t leasetime;
        bp->bp_vend[6] = DHCPREQUEST;
        bp->bp_vend[7] = TAG_REQ_ADDR;
        bp->bp_vend[8] = 4;
        bcopy(&rbuf.rbootp.bp_yiaddr, &bp->bp_vend[9], 4);
        bp->bp_vend[13] = TAG_SERVERID;
        bp->bp_vend[14] = 4;
        bcopy(&dhcp_serverip.s_addr, &bp->bp_vend[15], 4);
        bp->bp_vend[19] = TAG_LEASETIME;
        bp->bp_vend[20] = 4;
        leasetime = htonl(300);
        bcopy(&leasetime, &bp->bp_vend[21], 4);
        if (flag & BOOTP_PXE) {
            bp->bp_vend[25] = TAG_CLASSID;
            bp->bp_vend[26] = 9;
            bcopy("PXEClient", &bp->bp_vend[27], 9);
            bp->bp_vend[36] = TAG_END;
        } else
            bp->bp_vend[25] = TAG_END;

        expected_dhcpmsgtype = DHCPACK;


        if( 0 != bootpsend(udp_sock, bp, sizeof(*bp)) )
        {
            SHOW_ERROR0( 0, "can't send UDP");
            return ECONNREFUSED;
        }

        //if( 0 >= udp_recvfrom(udp_sock, &rbuf.rbootp, sizeof(rbuf.rbootp), &dest_addr, SOCK_FLAG_TIMEOUT, 1000000l) )
        if( 0 >= bootprecv(udp_sock, &rbuf.rbootp, sizeof(rbuf.rbootp)) )
        {
            SHOW_ERROR0( 0, "no reply");
            return ETIMEDOUT;
        }


        /*
        if(sendrecv(d,
                    bootpsend, bp, sizeof(*bp),
                    bootprecv, &rbuf.rbootp, sizeof(rbuf.rbootp))
           == -1) {
            SHOW_ERROR0( 0, "DHCPREQUEST failed");
            return;
        }*/
    }
#endif

    myip = rbuf.rbootp.bp_yiaddr;
    servip = rbuf.rbootp.bp_siaddr;
    //if(rootip.s_addr == INADDR_ANY) rootip = servip;
    bcopy(rbuf.rbootp.bp_file, bootfile, sizeof(bootfile));
    bootfile[sizeof(bootfile) - 1] = '\0';

    /*
    if (IN_CLASSA(ntohl(myip.s_addr)))
        nmask = htonl(IN_CLASSA_NET);
    else if (IN_CLASSB(ntohl(myip.s_addr)))
        nmask = htonl(IN_CLASSB_NET);
    else
        nmask = htonl(IN_CLASSC_NET);

    SHOW_FLOW( 1, "'native netmask' is %s", intoa(nmask));

    // Check subnet mask against net mask; toss if bogus 
    if ((nmask & smask) != nmask) {
        SHOW_ERROR( 1, "subnet mask (%s) bad", intoa(smask));
        smask = 0;
    }
    */

    /* Get subnet (or natural net) mask 
    netmask = nmask;
    if (smask)
    netmask = smask;
    */

    //SHOW_FLOW( 1, "mask: %s", intoa(smask));
    SHOW_FLOW( 1, "mask: 0x%08X", smask);

    /* We need a gateway if root is on a different net * /
    if (!SAMENET(myip, rootip, netmask)) {
        SHOW_FLOW( 1, "need gateway for root ip");
    }

    / * Toss gateway if on a different net * /
    if (!SAMENET(myip, gateip, netmask)) {
        SHOW_FLOW( 1, "gateway ip (%s) bad", inet_ntoa(gateip));
        gateip.s_addr = 0;
    } */

    SHOW_FLOW( 1, "gateway ip (%s) bad", inet_ntoa(gateip) );

    /* Bump xid so next request will be unique. */
    ++xid;

    return 0;
}

/* Transmit a bootp request */
static ssize_t
bootpsend(void *udp_sock, struct bootp *bp, size_t len)
{
    bp->bp_secs = htons((u_short)(time(0) - bot));

    SHOW_FLOW0( 1, "bootpsend: calling sendudp" );

    //return (sendudp(d, pkt, len));

    sockaddr dest_addr;
    dest_addr.port = IPPORT_BOOTPS; // dest port

    dest_addr.addr.len = 4;
    dest_addr.addr.type = ADDR_TYPE_IP;
    // INADDR_BROADCAST
    NETADDR_TO_IPV4(dest_addr.addr) = IPV4_DOTADDR_TO_ADDR(0xFF, 0xFF, 0xFF, 0xFF);

    if( 0 != udp_sendto(udp_sock, bp, len, &dest_addr) )
    {
        SHOW_ERROR0( 0, "can't send UDP");
        return -1;
    }

    return len;
}

static ssize_t
bootprecv(void *udp_sock, struct bootp *bp, size_t len)
{
    SHOW_FLOW0( 1, "bootp_recv");

    sockaddr dest_addr;
    dest_addr.port = IPPORT_BOOTPS; // dest port

    dest_addr.addr.len = 4;
    dest_addr.addr.type = ADDR_TYPE_IP;
    // INADDR_BROADCAST
    NETADDR_TO_IPV4(dest_addr.addr) = IPV4_DOTADDR_TO_ADDR(0xFF, 0xFF, 0xFF, 0xFF);

    int n;

    if( 0 >= ( n = udp_recvfrom(udp_sock, bp, len, &dest_addr, SOCK_FLAG_TIMEOUT, 1000000l)) )
    {
        SHOW_ERROR0( 0, "no reply");
        return -1;
    }

    if (n == -1 || n < (int)(sizeof(struct bootp) - BOOTP_VENDSIZE))
        goto bad;

    SHOW_FLOW( 1, "bootprecv: checked.  bp = 0x%lx, n = %d",
               (long)bp, (int)n);

    if (bp->bp_xid != htonl(xid)) {
        SHOW_FLOW( 1, "bootprecv: expected xid 0x%lx, got 0x%x",
                   xid, ntohl(bp->bp_xid));
        goto bad;
    }

    SHOW_FLOW0( 1, "bootprecv: got one!");

    /* Suck out vendor info */
    if (bcmp(vm_rfc1048, bp->bp_vend, sizeof(vm_rfc1048)) == 0) {
        if(vend_rfc1048(bp->bp_vend, sizeof(bp->bp_vend)) != 0)
            goto bad;
    }
#ifdef BOOTP_VEND_CMU
    else if (bcmp(vm_cmu, bp->bp_vend, sizeof(vm_cmu)) == 0)
        vend_cmu(bp->bp_vend);
#endif
    else
        SHOW_ERROR( 0, "bootprecv: unknown vendor 0x%lx", (long)bp->bp_vend);

    return(n);
bad:
    //errno = 0;
    return (-1);
}

static int
vend_rfc1048(u_char *cp, u_int len)
{
    u_char *ep;
    int size;
    u_char tag;

    SHOW_FLOW( 1, "vend_rfc1048 bootp info. len=%d", len);

    ep = cp + len;

    /* Step over magic cookie */
    cp += sizeof(int);

    while (cp < ep) {
        tag = *cp++;
        size = *cp++;
        if (tag == TAG_END)
            break;

        if (tag == TAG_SUBNET_MASK) {
            bcopy(cp, &smask, sizeof(smask));
        }
        if (tag == TAG_GATEWAY) {
            bcopy(cp, &gateip.s_addr, sizeof(gateip.s_addr));
        }
        if (tag == TAG_SWAPSERVER) {
            /* let it override bp_siaddr */
            bcopy(cp, &rootip.s_addr, sizeof(rootip.s_addr));
        }
        if (tag == TAG_ROOTPATH) {
            strncpy(rootpath, (char *)cp, sizeof(rootpath));
            rootpath[size] = '\0';
        }
        if (tag == TAG_HOSTNAME) {
            strncpy(hostname, (char *)cp, sizeof(hostname));
            hostname[size] = '\0';
        }
#ifdef SUPPORT_DHCP
        if (tag == TAG_DHCP_MSGTYPE) {
            if(*cp != expected_dhcpmsgtype)
                return(-1);
            dhcp_ok = 1;
        }
        if (tag == TAG_SERVERID) {
            bcopy(cp, &dhcp_serverip.s_addr,
                  sizeof(dhcp_serverip.s_addr));
        }
#endif
        cp += size;
    }
    return(0);
}

#ifdef BOOTP_VEND_CMU
static void
vend_cmu(u_char *cp)
{
    struct cmu_vend *vp;

    SHOW_FLOW0( 1, "vend_cmu bootp info.");

    vp = (struct cmu_vend *)cp;

    if (vp->v_smask.s_addr != 0) {
        smask = vp->v_smask.s_addr;
    }
    if (vp->v_dgate.s_addr != 0) {
        gateip = vp->v_dgate;
    }
}
#endif







errno_t bootp(ifnet *iface)
{
    void *udp_sock;
    char mac_addr[6];

    int err = iface->dev->dops.get_address(iface->dev, mac_addr, 6);
    if(err < 0) {
        SHOW_ERROR0( 0, "can't get interface MAC address");
        return ENXIO;
    }


    if( xid == 0 )
        xid = (int)time(0) ^ 0x1E0A4F; // Some strange number :)

    if( udp_open(&udp_sock) )
    {
        SHOW_ERROR0( 0, "UDP - can't prepare endpoint");
        return ENOTSOCK;
    }

    errno_t e = do_bootp(udp_sock, mac_addr, 0);

    udp_close(udp_sock);

    if(e)
        SHOW_ERROR( 0, "error %d", e);


    return e;
}










