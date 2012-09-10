#ifndef PRO_SNMP_PDU_H
#define PRO_SNMP_PDU_H

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

/*!
 * \file kernel/snmp/snmp_client.h
 * \brief SNMP client functions.
 *
 * \verbatim
 * $Id$
 * \endverbatim
 */

#include <kernel/snmp/asn1.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

typedef struct _SNMP_VARLIST SNMP_VARLIST;

struct _SNMP_VARLIST {
    SNMP_VARLIST *var_next;
    OID *var_name;
    int var_nlen;
    uint8_t var_type;
    long var_val;
    uint8_t *var_vptr;
    int var_vlen;
};

typedef struct {
    /*! \brief Type of this PDU. */
    int pdu_cmd;

    uint32_t  pdu_reqid;    /* Request id */
    uint32_t  pdu_errstat;  /* Error status */
    uint32_t  pdu_errindex; /* Error index */

    /* Trap information */
    OID *pdu_enterprise;/* System OID */
    int     pdu_enterprise_length;
    uint32_t  pdu_agent_addr;     /* address of object generating trap */
    int pdu_trap_type;      /* trap type */
    int pdu_specific_type;  /* specific type */
    uint32_t pdu_time;   /* Uptime */

    SNMP_VARLIST *pdu_variables;
} SNMP_PDU;

/*@}*/

extern SNMP_PDU *SnmpPduCreate(int cmd, CONST OID *ep, size_t ep_len);
extern void SnmpPduDestroy(SNMP_PDU *pdu);

extern int SnmpPduAddVariable(SNMP_PDU *pdu, OID *name, size_t nlen, uint8_t type, uint8_t *value, size_t vlen);

#endif
