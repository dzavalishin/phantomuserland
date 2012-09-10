/*
 * Copyright 2007 by egnite Software GmbH
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * For additional information see http://www.ethernut.de/
 */

#define DEBUG_MSG_PREFIX "snmp"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


//#include <dev/board.h>

#include <sys/types.h>
#include <ctype.h>
#include <errno.h>

//#include <arpa/inet.h>
//#include <net/route.h>
//#include <pro/dhcp.h>

//#include <sys/version.h>
//#include <sys/timer.h>

#include <compat/nutos.h>

#include <kernel/snmp/snmp_config.h>
#include <kernel/snmp/snmp_mib.h>
#include <kernel/snmp/snmp_api.h>
#include <kernel/snmp/snmp_agent.h>

#include <kernel/init.h>
#include <kernel/net/udp.h>

#include <stdio.h>
//#include <io.h>

//#include "mib2sys.h"
//#include "mib2if.h"

/*!
 * \example snmpd/snmpd.c
 *
 * Basic SNMP Agent.
 */
static char *version = "0.2.0";



static void snmp_daemon_thread(void)
{
    OID view_all[] = { SNMP_OID_INTERNET };
    int view_idx;
    int rc = 0;


    /*
     * Print banner.
     */
    //printf("\n\nSNMP Agent %s\nPhantom OS %s\n", version, NutVersionString());
    SHOW_FLOW( 1, "SNMP Agent %s", version);
    //puts("Compiled by " COMPILERNAME);
    //puts("Configure network");

    void *udps;

    rc = udp_open( &udps );
    if( rc )
    {
        SHOW_ERROR0( 0, "can't get udp socket" );
        goto fail;
    }

    i4sockaddr addr;
    addr.port = SNMP_PORT;

    if( udp_bind( udps, &addr ) )
    {
        SHOW_ERROR( 0, "can't bind to %d", addr.port );
        goto cfail;
    }

    // TODO MIBs

    /* Register system variables. */
    if (MibRegisterOSVars())
        SHOW_ERROR0( 0, "Failed to register MibSys" );
    /* Register interface variables. */
    //if (MibRegisterIfVars())            printf("Failed to register MibIf\n");

    /* Create views. */
    if ((view_idx = SnmpViewCreate("all", view_all, sizeof(view_all), SNMP_VIEW_INCLUDED)) <= 0) {
        SHOW_ERROR0( 0, "Failed to create view");
        goto cfail;
    }
    /* Create communities. */
    if (SnmpCommunityCreate("public", view_idx, view_idx) || SnmpCommunityCreate("private", view_idx, view_idx)) {
        SHOW_ERROR0( 0, "Failed to create communities");
        goto cfail;
    }

    /* Run agent. */
    SnmpAgent(udps);

cfail:
    /* Program stopped. */
    udp_close(udps);
//#endif

fail:
    return;
}

INIT_ME( 0, 0, snmp_daemon_thread );

