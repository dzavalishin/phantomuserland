#ifndef PRO_SNMP_VARS_H_
#define PRO_SNMP_VARS_H_

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
 * \file kernel/snmp/snmp_mib.h
 * \brief SNMP MIB support functions.
 *
 * \verbatim
 * $Id: snmp_mib.h 2737 2009-10-02 12:37:14Z haraldkipp $
 * \endverbatim
 */

#include <kernel/snmp/asn1.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

typedef int (WMETHOD)(int, uint8_t *, uint8_t, size_t, OID *, size_t);

typedef struct _SNMPVAR {
    /*! \brief Internal magic identifier. */
    uint8_t var_magic;
    /*! \brief Type of this variable. */
    char var_type;
    /*! \brief Access control. */
    uint16_t var_acl;
    /*! \brief Variable access funtion. */
    uint8_t *(*var_get)(CONST struct _SNMPVAR*, OID*, size_t*, int, size_t*, WMETHOD **);
    /*! \brief Number of sub-IDs in the name. */
    size_t var_namelen;
    /*! \brief Name (object identifier) of the variable. */
    OID var_name[MAX_OID_LEN];
} SNMPVAR;

/*@}*/

extern int SnmpMibRegister(OID[], size_t, SNMPVAR *, int);
extern uint8_t *SnmpMibFind(OID *, size_t *, uint8_t *, size_t *, uint16_t *, int, WMETHOD **, int *);

// TODO must be in separate header?
int MibRegisterOSVars(void);
int MibRegisterIfVars(void);


#endif
