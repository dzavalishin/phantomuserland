#ifndef PRO_SNMP_API_H
#define PRO_SNMP_API_H

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
 * \file kernel/snmp/snmp_api.h
 * \brief SNMP API functions.
 *
 * \verbatim
 * $Id: snmp_api.h 2737 2009-10-02 12:37:14Z haraldkipp $
 * \endverbatim
 */

#include <kernel/snmp/asn1.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

#define SNMP_MAX_MSG_SIZE          1472 /* ethernet MTU minus IP/UDP header */


/*
 * SNMP statistic counters.
 */
#define  SNMP_STAT_INPKTS                0
#define  SNMP_STAT_OUTPKTS               1
#define  SNMP_STAT_INBADVERSIONS         2
#define  SNMP_STAT_INBADCOMMUNITYNAMES   3
#define  SNMP_STAT_INBADCOMMUNITYUSES    4
#define  SNMP_STAT_INASNPARSEERRS        5
#define  SNMP_STAT_INTOOBIGS             6
#define  SNMP_STAT_INNOSUCHNAMES         7
#define  SNMP_STAT_INBADVALUES           8
#define  SNMP_STAT_INREADONLYS           9
#define  SNMP_STAT_INGENERRS             10
#define  SNMP_STAT_INTOTALREQVARS        11
#define  SNMP_STAT_INTOTALSETVARS        12
#define  SNMP_STAT_INGETREQUESTS         13
#define  SNMP_STAT_INGETNEXTS            14
#define  SNMP_STAT_INSETREQUESTS         15
#define  SNMP_STAT_INGETRESPONSES        16
#define  SNMP_STAT_INTRAPS               17
#define  SNMP_STAT_OUTTOOBIGS            18
#define  SNMP_STAT_OUTNOSUCHNAMES        19
#define  SNMP_STAT_OUTBADVALUES          20
#define  SNMP_STAT_OUTGENERRS            21
#define  SNMP_STAT_OUTGETREQUESTS        22
#define  SNMP_STAT_OUTGETNEXTS           23
#define  SNMP_STAT_OUTSETREQUESTS        24
#define  SNMP_STAT_OUTGETRESPONSES       25
#define  SNMP_STAT_OUTTRAPS              26
#define  SNMP_STAT_ENABLEAUTHENTRAPS     27

#define  SNMP_STAT_MAX                   28

/*@}*/

extern int SnmpOidCmp(CONST OID *, size_t, CONST OID *, size_t);
extern int SnmpOidLenCmp(CONST OID *name1, CONST OID *name2, size_t len);
extern int SnmpOidTreeCmp(CONST OID *, size_t, CONST OID *, size_t);
extern int SnmpOidCmpIdx(CONST OID *name1, size_t len1, CONST OID *name2, size_t len2, OID index);
extern int SnmpOidEquals(CONST OID *, size_t, CONST OID *, size_t);

extern void SnmpStatsInc(int);
extern uint32_t SnmpStatsGet(int);
extern void SnmpStatsSet(int, uint32_t);

#endif
