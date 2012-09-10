#if 0
/*
 * Copyright 2009 by egnite GmbH
 * Copyright 1998-2007 by egnite Software GmbH
 * Copyright 1988, 1989, 1991, 1992 by Carnegie Mellon University
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

//#include <sys/confnet.h>

#include <stdlib.h>
#include <string.h>

#include <compat/nutos.h>

#include <kernel/net/udp.h>

#include <kernel/snmp/snmp_auth.h>
#include <kernel/snmp/snmp_session.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

static uint32_t snmp_reqid;
static uint8_t temp_buffer[SNMP_MAX_LEN];

/*!
 * \brief Open SNMP session.
 *
 * \param ip    Remote IP address.
 * \param port  Remote port number.
 * \param id    Community name.
 * \param idlen Length of the community name.
 *
 * \return Pointer to a newly created session info.
 */
SNMP_SESSION *SnmpSessionOpen(uint32_t ip, uint16_t port, uint8_t * id, size_t idlen)
{
    SNMP_SESSION *session = calloc(1, sizeof(SNMP_SESSION));

    if (session)
    {
        void *udps;

        int rc = udp_open( &udps );
        if( rc ) goto fail;

        if( udp_listen( udps ) )
        {
            udp_close( udps );
            goto fail;
        }



        /* Create UDP socket at random local port. */
        session->sess_sock = NutUdpCreateSocket(0);
        if (session->sess_sock == NULL) {
        fail:
            /* Failed to create socket. */
            free(session);
            session = NULL;
        } else {
            /* Set remote address/port. */
            session->sess_rem_addr = ip;
            session->sess_rem_port = port;
            /* Set session identifier. */
            memcpy(session->sess_id, id, idlen);
            session->sess_id_len = idlen;
        }
    }
    return session;
}

/*!
 * \brief Close SNMP session.
 *
 * \param session Points to the session info, obtained by calling 
 *                SnmpSessionOpen().
 */
void SnmpSessionClose(SNMP_SESSION * session)
{
    if (session->sess_sock) {
        NutUdpDestroySocket(session->sess_sock);
    }
    free(session);
}

/*!
 * \brief Build SNMP packet.
 *
 * \param session Points to the session info, obtained by calling 
 *                SnmpSessionOpen().
 * \param pdu     Points to the PDU info, obtained by calling 
 *                SnmpPduCreate().
 * \param packet  Pointer to a packet buffer.
 * \param psize Points to a variable that contains the number of 
 *                available bytes in the packet buffer. On exit, it is 
 *                returned as the number of valid bytes in the packet buffer.
 *
 * \return 0 on success, -1 otherwise.
 */
static int SnmpMsgBuild(SNMP_SESSION * session, SNMP_PDU * pdu, uint8_t * packet, size_t * psize)
{
    uint8_t *cp;
    SNMP_VARLIST *vp;
    size_t length;
    int total;
    long lval;

    /*
     * Encode the list of variable bindings.
     */
    length = *psize;
    cp = packet;
    for (vp = pdu->pdu_variables; vp; vp = vp->var_next) {
        cp = SnmpVarBuild(cp, &length, vp->var_name, vp->var_nlen, vp->var_type, vp->var_vptr, vp->var_vlen);
        if (cp == NULL) {
            return -1;
        }
    }
    total = cp - packet;

    /*
     * Encode the sequence header of the variable bindings.
     */
    length = SNMP_MAX_LEN;
    cp = AsnHeaderBuild(temp_buffer, &length, ASN_SEQUENCE | ASN_CONSTRUCTOR, total);
    if (cp == NULL) {
        return -1;
    }

    /*
     * Append the variable bindings to its header.
     */
    memcpy(cp, packet, total);
    total += cp - temp_buffer;

    /*
     * Encode the PDU.
     *
     * Note the different format used for traps.
     */
    length = *psize;
    if (pdu->pdu_cmd == SNMP_MSG_TRAP) {
        /* Enterprise. */
        cp = AsnOidBuild(packet, &length, ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_OBJECT_ID, pdu->pdu_enterprise, pdu->pdu_enterprise_length);
        if (cp == NULL) {
            return -1;
        }
        /* Agent address. */
        cp = AsnOctetStringBuild(cp, &length, ASN_IPADDRESS, (u_char *) & confnet.cdn_ip_addr, sizeof(confnet.cdn_ip_addr));
        if (cp == NULL) {
            return -1;
        }
        /* Generic trap. */
        lval = (long)pdu->pdu_trap_type;
        cp = AsnIntegerBuild(cp, &length, ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER, &lval);
        if (cp == NULL) {
            return -1;
        }
        /* Specific trap. */
        lval = (long)pdu->pdu_specific_type;
        cp = AsnIntegerBuild(cp, &length, ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER, &lval);
        if (cp == NULL) {
            return -1;
        }
        /* Time stamp. */
        cp = AsnIntegerBuild(cp, &length, ASN_TIMETICKS, (long *) &pdu->pdu_time);
        if (cp == NULL) {
            return -1;
        }
    } else {
        /* Request ID. */
        cp = AsnIntegerBuild(packet, &length, ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER, (long *) &pdu->pdu_reqid);
        if (cp == NULL) {
            return -1;
        }
        /* Error status. */
        cp = AsnIntegerBuild(cp, &length, ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER, (long *) &pdu->pdu_errstat);
        if (cp == NULL) {
            return -1;
        }
        /* Error index. */
        cp = AsnIntegerBuild(cp, &length, ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER, (long *) &pdu->pdu_errindex);
        if (cp == NULL) {
            return -1;
        }
    }

    /*
     * Append the variable bindings.
     */
    if (length < total) {
        return -1;
    }
    memcpy(cp, temp_buffer, total);
    total += cp - packet;

    /*
     * Encode the PDU header containing its type.
     */
    length = SNMP_MAX_LEN;
    cp = AsnHeaderBuild(temp_buffer, &length, (u_char) pdu->pdu_cmd, total);
    if (cp == NULL || length < total) {
        return -1;
    }

    /*
     * Append the PDU body.
     */
    memcpy(cp, packet, total);
    total += cp - temp_buffer;

    /*
     * Encode the message header with version and community string.
     */
    length = *psize;
    cp = SnmpAuthBuild(session, packet, &length, total);
    if (cp == NULL) {
        return -1;
    }

    /*
     * Append the PDU.
     */
    if ((*psize - (cp - packet)) < total) {
        return -1;
    }
    memcpy(cp, temp_buffer, total);
    total += cp - packet;
    *psize = total;

    return 0;
}

/*!
 * \brief Send PDU.
 *
 * \param session Points to the session info, obtained by calling 
 *                SnmpSessionOpen().
 * \param pdu     Points to the PDU info, obtained by calling 
 *                SnmpPduCreate().
 *
 * \return 0 on success, -1 otherwise.
 */
int SnmpSessionSendPdu(SNMP_SESSION * session, SNMP_PDU * pdu)
{
    int rc = -1;
    uint8_t *packet;
    size_t length;

    if (pdu->pdu_cmd == SNMP_MSG_TRAP) {
        /* Bogus request ID for traps. */
        pdu->pdu_reqid = 1;
    }
    else if (pdu->pdu_reqid == 0) {
        pdu->pdu_reqid = ++snmp_reqid;
    }

    length = SNMP_MAX_LEN;
    packet = malloc(length);
    if (packet) {
        if (SnmpMsgBuild(session, pdu, packet, &length) == 0) {
            rc = NutUdpSendTo(session->sess_sock, session->sess_rem_addr, session->sess_rem_port, packet, length);
        }
        free(packet);
    }
    return rc;
}

/*@}*/

#endif
