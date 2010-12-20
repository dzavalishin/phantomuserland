/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * ICMP.
 *
**/

#include <kernel/config.h>

#if HAVE_NET

/*
 ** Copyright 2001, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */

#include "khash.h"
#include "net.h"
#include "ethernet.h"

#include "atomic.h"
#include "endian.h"
#include "hal.h"


#include "newos.h"


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
    case 0: { // echo reply
        break;
    }
    case 8: { // echo request
        icmp_echo_header *eheader = (icmp_echo_header *)header;

        // bounce this message right back
        eheader->preheader.type = 0; // echo reply
        eheader->preheader.checksum = 0;
        eheader->preheader.checksum = cbuf_ones_cksum16(buf, 0, 0xffff);
        return ipv4_output(buf, source_ipaddr, IP_PROT_ICMP);
    }
#if NET_CHATTY
    default:
        dprintf("unhandled icmp message\n");
#endif
    }

    err = NO_ERROR;

ditch_message:
    cbuf_free_chain(buf);

    return err;
}

#endif // HAVE_NET

