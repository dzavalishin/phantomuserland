#ifndef PRO_SNMP_SESSION_H
#define PRO_SNMP_SESSION_H

/*
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

/*!
 * \file kernel/snmp/snmp_session.h
 * \brief SNMP client functions.
 *
 * \verbatim
 * $Id$
 * \endverbatim
 */

#include <sys/socket.h>

#include <kernel/snmp/snmp.h>
#include <kernel/snmp/snmp_pdu.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

typedef struct {
    long sess_version;
    size_t sess_id_len;
    uint8_t sess_id[MAX_SID_LEN + 1];
    int sess_read_view;
    int sess_write_view;
    //UDPSOCKET *sess_sock;
    void *sess_sock; // udp_socket
    uint32_t sess_rem_addr;
    uint16_t sess_rem_port;
} SNMP_SESSION;

/*@}*/

extern SNMP_SESSION *SnmpSessionOpen(uint32_t ip, uint16_t port, uint8_t *id, size_t id_len);
extern void SnmpSessionClose(SNMP_SESSION *session);

extern int SnmpSessionSendPdu(SNMP_SESSION *session, SNMP_PDU *pdu);

#endif
