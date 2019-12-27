/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * ICMP.
 *
 * Based on NewOS code: 
 * 
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 *
**/

#define DEBUG_MSG_PREFIX "icmp"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_info 10
#define debug_level_error 10

#include <kernel/debug.h>
#include <kernel/config.h>

#if HAVE_NET

#include <kernel/khash.h>
#include <kernel/net.h>
#include <kernel/net/ethernet.h>

#include <kernel/atomic.h>
#include <endian.h>
#include <hal.h>

#include <arpa/inet.h>


#define dprintf lprintf

#define ICMP_ER		0	/* echo reply */
#define ICMP_DUR	3	/* destination unreachable */
#define ICMP_SQ		4	/* source quench */
#define ICMP_RD		5	/* redirect */
#define ICMP_ECHO	8	/* echo */
#define ICMP_TE		11	/* time exceeded */
#define ICMP_PP		12	/* parameter problem */
#define ICMP_TS		13	/* timestamp */
#define ICMP_TSR	14	/* timestamp reply */
#define ICMP_IRQ	15	/* information request */
#define ICMP_IR		16	/* information reply */

#define	ICMP_DUR_NET    0	/* net unreachable */
#define	ICMP_DUR_HOST   1	/* host unreachable */
#define	ICMP_DUR_PROTO  2	/* protocol unreachable */
#define	ICMP_DUR_PORT   3	/* port unreachable */
#define	ICMP_DUR_FRAG   4	/* fragmentation needed and DF set */
#define	ICMP_DUR_SR     5	/* source route failed */


typedef struct icmp_header {
    uint8 type;
    uint8 code;
    uint16 checksum;
} _PACKED icmp_header;

typedef struct icmp_echo_header {
    icmp_header preheader;
    uint16 identifier;
    uint16 sequence;
} _PACKED icmp_echo_header;

int icmp_input(cbuf *buf, ifnet *i, ipv4_addr source_ipaddr)
{
    icmp_header *header;
    int err;

    (void) i;

    header = (icmp_header *)cbuf_get_ptr(buf, 0);

#if NET_CHATTY
    dprintf("icmp_message: header type %d, code %d, checksum 0x%x, length %Ld\n", header->type, header->code, header->checksum, (long long)cbuf_get_len(buf));
    dprintf(" buffer len %d\n", cbuf_get_len(buf));
#endif

    // calculate the checksum on the whole thing
    if(cbuf_ones_cksum16(buf, 0, 0xffff) != 0) {
        dprintf("icmp message fails cksum\n");
#if NET_CHATTY
        {
            unsigned int i;
            for(i=0; i<cbuf_get_len(buf); i++) {
                if((i % 8) == 0) {
                    dprintf("\n0x%x: ", i);
                }
                dprintf("0x%x ", *(unsigned char *)cbuf_get_ptr(buf, i));
            }
            dprintf("\n");
        }
#endif
        err = ERR_NET_BAD_PACKET;
        goto ditch_message;
    }

    switch(header->type) {
    case ICMP_ER: { // echo reply
        LOG_INFO_( 1, "got icmp reply from %s", inet_itoa( source_ipaddr ) );
        break;
    }
    case ICMP_ECHO: { // echo request
        icmp_echo_header *eheader = (icmp_echo_header *)header;

        // bounce this message right back
        eheader->preheader.type = 0; // echo reply
        eheader->preheader.checksum = 0;
        eheader->preheader.checksum = cbuf_ones_cksum16(buf, 0, 0xffff);
        return ipv4_output(buf, source_ipaddr, IP_PROT_ICMP);
    }

    default:
        LOG_ERROR( 1, "unhandled icmp message type = %d", header->type );
        break;
    }

    err = NO_ERROR;

ditch_message:
    cbuf_free_chain(buf);

    return err;
}

#endif // HAVE_NET

