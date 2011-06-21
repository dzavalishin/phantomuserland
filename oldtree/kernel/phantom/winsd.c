#if HAVE_NET
/*
 * Copyright (C) 2004 by Jean Pierre Gauthier. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY <YOUR NAME> AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <YOUR NAME>
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**********************************************************
 *
 * Netbios WINS (RFC 1002) Name Query daemon
 *
 * Only Query Request Client Routine sending/Positive Name
 * Query Response receiving are implemented.
 *
 * When the Netbios Name Query request UDP datagram is on
 * the ethernet network, asking "Who is 'name'?", wins daemon
 * answers with the specified 'ipaddr' Ethernut IP address.
 *
 * Answer to Microsoft Windows/Internet Explorer calls by
 * "http://name" command line (and even directly "name" as
 * command line if "name" is not a shared folder).
 *
 ********************************************************* */

#define DEBUG_MSG_PREFIX "winsd"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/net.h>
#include <kernel/net/udp.h>

#include <threads.h>
#include <stdlib.h>
#include <string.h>

#include <malloc.h>
//#include <sys/socket.h>
//#include <sys/sock_var.h>
#include <sys/types.h>
#include <kernel/init.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>


#define NETBIOS_UDP_PORT 137


/* ********************************************************* */

/*!
 * \addtogroup xgWins
 */
/*@{*/

typedef struct {
    u_int16_t id;
    u_int16_t flags;
    u_int16_t quests;
    u_int16_t answers;
    u_int16_t authrr;
    u_int16_t addrr;
    u_int8_t namelen;
    u_int8_t name[33];
    u_int16_t type;
    u_int16_t class;              /* end of request */
    u_int32_t ttl;
    u_int16_t len_rep;
    u_int8_t node_flags;
    u_int8_t node_type;
    u_int32_t ip_addr;             /* end of answer */
} WINSHEADER;

static u_int32_t wins_ipaddr;


static errno_t do_wins(void *prot_data, u_int8_t *encoded )
{
    sockaddr addr;
    WINSHEADER pkt;
    errno_t rc;

    SHOW_FLOW( 1, "local compressed name='%s'", encoded);

    /* infinite loop / Netbios deamon */
    for (;;) {

        if( 0 >= (rc = udp_recvfrom(prot_data, &pkt, sizeof(WINSHEADER), &addr, 0, 0 )) )
        {
            SHOW_ERROR( 1, "WINS UDP recv err %d", rc );
            continue;
        }

        SHOW_FLOW( 2, "WINS request for '%s'", (char *)pkt.name );

        /* RFC1002 Name Query Request verification */
        if (((ntohs(pkt.flags) & 0xf800) != 0) ||      /* */
            (ntohs(pkt.quests) != 1) ||        /* */
            (pkt.namelen != 0x20) ||   /* */
            (ntohs(pkt.type) != 32) || /* */
            (ntohs(pkt.class) != 1) || /* */
            (strcmp((char *)pkt.name, (char *)encoded)))
            continue;           /* bad request, try again */

        SHOW_FLOW( 2, "WINS reply with '%s'", (char *)encoded );

        /* build RFC1002 Positive Name Query Response */
        pkt.flags = htons(0x8580);     /* Response flags */
        pkt.answers = htons(1);
        pkt.ttl = htonl((u_int32_t) 60);  /* 60 seconds validity */
        pkt.len_rep = htons(6);
        pkt.node_flags = pkt.node_type = pkt.quests = 0;     /* B-type node, etc... */
        pkt.ip_addr = wins_ipaddr;  /* Returned IP Address, end of answer */

        addr.port = NETBIOS_UDP_PORT;

        if( 0 != (rc = udp_sendto(prot_data, &pkt, sizeof(WINSHEADER), &addr)) )
        {
            SHOW_ERROR( 1, "WINS UDP send err %d", rc );
        }

        memset( &pkt, 0, sizeof(WINSHEADER) );
    }

    return 0;
}



/* ********************************************************* */
/* name : netbios label (15 chars max), ipaddr : network ordered IP address bytes */
static errno_t do_winsd_thread(char * name)
{
    u_int8_t encoded[33];
    u_int8_t car;
    int i, j;

    if(strlen(name) > 15)
    {
        SHOW_ERROR( 0, "Wins name too long: '%s'", name );
        return EINVAL;
    }

    memset( &encoded, 0, sizeof(encoded) );

    int len = strlen(name);

    /* label  'compression' */
    j = 0;
    for (i = 0; i < 16; i++)
    {
        car = toupper(i < len ? name[i] : ' ');
        if (i == 15)
            car = 0;
        encoded[j] = (car >> 4) + 'A';
        encoded[j + 1] = (car & 0xf) + 'A';
        j += 2;
    }
    encoded[j] = 0;



    void *prot_data;

    if( udp_open(&prot_data) )
    {
        SHOW_ERROR0( 0, "UDP - can't prepare endpoint");
        return EIO;
    }

    errno_t ret = do_wins( prot_data, encoded );
    udp_close(prot_data);

    if( ret )
        SHOW_ERROR( 0 , "winsd failed, rc = %d", ret );

    return ret;
}

static int n_wins_threads = 0;

static void winsd_thread(void)
{
    n_wins_threads++;

    do_winsd_thread("phantom");

    n_wins_threads--;
}


void init_wins(u_int32_t ip_addr)
{
    wins_ipaddr = ip_addr;
    if(n_wins_threads)
    {
        SHOW_ERROR0( 0 , "winsd is already running" );
        return;
    }

    hal_start_kernel_thread(winsd_thread);
}
#else // HAVE_NET
#include <sys/types.h>
void init_wins(u_int32_t ip_addr)
{
    (void) ip_addr;
}
#endif // HAVE_NET

