#ifndef PRO_SNMP_H
#define PRO_SNMP_H

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
 * \file kernel/snmp/snmp.h
 * \brief Definitions for Simple Network Management Protocol.
 *
 * \verbatim
 * $Id: snmp.h 2737 2009-10-02 12:37:14Z haraldkipp $
 * \endverbatim
 */

#include <compat/nutos.h>

#include <kernel/snmp/asn1.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

#ifndef SNMP_PORT
/*! \brief Standard UDP port for SNMP agents to receive requests messages. 
 */
#define SNMP_PORT       161
#endif

#ifndef SNMP_TRAP_PORT
/*! \brief Standard UDP port for SNMP managers to receive notificaion messages. 
 */
#define SNMP_TRAP_PORT  162
#endif

#ifndef SNMP_MAX_LEN
/*! \brief Default maximum message size. 
 */
#define SNMP_MAX_LEN    500
#endif

/*! \brief SNMPv1.
 *
 * The original version, defined by RFC 1157.
 */
#define SNMP_VERSION_1	    0

/*! \brief SNMPv2c.
 *
 * Community string-based SNMPv2, which was an attempt to combine the 
 * protocol operations of SNMPv2 with the security of SNMPv1, defined 
 * by RFCs 1901, 1905, and 1906.
 *
 * Partly supplied by this code, work is in progress.
 */
#define SNMP_VERSION_2C	    1

/*! \brief SNMPv3.
 *
 * An attempt by the IETF working group to merge the SNMPv2u and SNMPv2* 
 * proposals into a more widely accepted SNMPv3. The original version, 
 * defined by RFC 1157.
 *
 * Not yet supported by this code.
 */
#define SNMP_VERSION_3      3

/*
 * PDU types in SNMPv1, SNMPv2c and SNMPv3.
 */
#define SNMP_MSG_GET        (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x0)
#define SNMP_MSG_GETNEXT    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x1)
#define SNMP_MSG_RESPONSE   (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x2)
#define SNMP_MSG_SET        (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x3)

/*
 * PDU types in SNMPv1. 
 */
#define SNMP_MSG_TRAP       (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x4)

/*
 * PDU types in SNMPv2c and SNMPv3 
 */
#define SNMP_MSG_GETBULK    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x5)
#define SNMP_MSG_INFORM     (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x6)
#define SNMP_MSG_TRAP2      (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x7)

/*
 * PDU types in SNMPv3 
 */
#define SNMP_MSG_REPORT     (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x8)

/*
 * Exception values for SNMPv2c and SNMPv3 
 */
#define SNMP_NOSUCHOBJECT    (ASN_CONTEXT | ASN_PRIMITIVE | 0x0)
#define SNMP_NOSUCHINSTANCE  (ASN_CONTEXT | ASN_PRIMITIVE | 0x1)
#define SNMP_ENDOFMIBVIEW    (ASN_CONTEXT | ASN_PRIMITIVE | 0x2)

/*
 * Error codes in SNMPv1, SNMPv2c and SNMPv3 PDUs.
 */
#define SNMP_ERR_NOERROR    		0
#define SNMP_ERR_TOOBIG	    		1
#define SNMP_ERR_NOSUCHNAME 		2
#define SNMP_ERR_BADVALUE   		3
#define SNMP_ERR_READONLY   		4
#define SNMP_ERR_GENERR	    		5

/*
 * Error codes in SNMPv2c and SNMPv3 PDUs.
 */
#define SNMP_ERR_NOACCESS               6
#define SNMP_ERR_WRONGTYPE              7
#define SNMP_ERR_WRONGLENGTH            8
#define SNMP_ERR_WRONGENCODING          9
#define SNMP_ERR_WRONGVALUE             10
#define SNMP_ERR_NOCREATION             11
#define SNMP_ERR_INCONSISTENTVALUE      12
#define SNMP_ERR_RESOURCEUNAVAILABLE    13
#define SNMP_ERR_COMMITFAILED           14
#define SNMP_ERR_UNDOFAILED             15
#define SNMP_ERR_AUTHORIZATIONERROR     16
#define SNMP_ERR_NOTWRITABLE            17
#define SNMP_ERR_INCONSISTENTNAME	18

/*
 * Values of the generic-trap field in trap PDUs.
 */
#define SNMP_TRAP_COLDSTART		0
#define SNMP_TRAP_WARMSTART		1
#define SNMP_TRAP_LINKDOWN		2
#define SNMP_TRAP_LINKUP		3
#define SNMP_TRAP_AUTHFAIL		4
#define SNMP_TRAP_EGPNEIGHBORLOSS	5
#define SNMP_TRAP_ENTERPRISESPECIFIC	6

/*
 * Basic OID values 
 */
#define SNMP_OID_INTERNET       1, 3, 6, 1
#define SNMP_OID_ENTERPRISES    SNMP_OID_INTERNET, 4, 1
#define SNMP_OID_MIB2           SNMP_OID_INTERNET, 2, 1
#define SNMP_OID_SNMPV2         SNMP_OID_INTERNET, 6
#define SNMP_OID_SNMPMODULES    SNMP_OID_SNMPV2, 3



#define SNMP_PARSE_ERROR	-1
#define SNMP_BUILD_ERROR	-2

/*! \brief Maximum length of a community name. */
#define MAX_SID_LEN	32
/*! \brief Maximum number of sub IDs in an OID. */
#define MAX_NAME_LEN	128

#define SNMP_ACT_RESERVE1    0
#define SNMP_ACT_RESERVE2    1
#define SNMP_ACT_COMMIT      2
#define SNMP_ACT_ACTION      3
#define SNMP_ACT_FREE        4

/*@}*/

extern CONST uint8_t *SnmpVarParse(CONST uint8_t *, size_t *, OID *, size_t *, uint8_t *, uint8_t **, size_t *);
extern uint8_t *SnmpVarBuild(uint8_t *, size_t *, CONST OID *, size_t, uint8_t , CONST uint8_t *, size_t);

#endif
