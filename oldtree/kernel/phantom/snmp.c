#if HAVE_NET
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

#include <compat/nutos.h>

#include <kernel/snmp/snmp.h>
#include <kernel/snmp/snmp_api.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

/*!
 * \brief Parse an SNMP variable.
 *
 * \param data  Pointer to start of the name/value pair.
 * \param dlen  Contains the number of valid bytes following the 
 *              start of the variable. On exit, it is returned as 
 *              the number of valid bytes following the end of 
 *              this variable.
 * \param name  Pointer to a buffer that receives the name (OID).
 * \param nlen  On entry, this contains the maximum number of 
 *              sub IDs accepted for the name. On exit, it is 
 *              returned as the actual number sub IDs found in
 *              the name.
 * \param type  Pointer to the variable that receives the ASN 
 *              type of the value.
 * \param value Pointer to variable that receives a pointer to
 *              the ASN1 encoded value of variable.
 * \param vlen  Pointer to the variable that receives the length
 *              of the value.
 *
 * \return Pointer to the first byte past the end of this name/value pair. 
 *         Returns NULL on any error.
 */
CONST uint8_t *SnmpVarParse(CONST uint8_t * data, size_t * dlen, OID * name, size_t * nlen, uint8_t * type,
                           uint8_t ** value, size_t * vlen)
{
    CONST uint8_t *dp;
    uint8_t vtype = ASN_SEQUENCE | ASN_CONSTRUCTOR;
    size_t len = *dlen;

    /* Get the object's length and check its type. */
    if ((dp = AsnSequenceParse(data, &len, vtype)) == NULL) {
        return NULL;
    }
    /* Get type, value and length of the name. */
    if ((dp = AsnOidParse(dp, &len, &vtype, name, nlen)) == NULL) {
        return NULL;
    }
    /* Check the name's type. */
    if (vtype != (uint8_t) (ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_OBJECT_ID)) {
        return NULL;
    }
    /* Return a pointer to the value. */
    *value = (uint8_t *) dp;
    /* Find out what type of object this is. */
    if ((dp = AsnHeaderParse(dp, &len, type)) == NULL) {
        return NULL;
    }
    *vlen = len;
    dp += *vlen;
    *dlen -= dp - data;

    return dp;
}

/*!
 * \brief Build an SNMP variable.
 *
 * \param data  Pointer to start of the output buffer.
 * \param dlen  Contains the number of valid bytes following the 
 *              start of the variable. On exit, it is returned as 
 *              the number of valid bytes following the end of 
 *              this variable.
 * \param name  Name (OID).
 * \param nlen  Number of sub IDs of the name.
 * \param type  ASN type of the value.
 * \param value Pointer to the value.
 * \param vlen  Length of the value.
 *
 * \return Pointer to the first byte past the end of this name/value pair. 
 *         Returns NULL on any error.
 */
uint8_t *SnmpVarBuild(uint8_t * data, size_t * dlen, CONST OID * name, size_t nlen, uint8_t type, CONST uint8_t * value, size_t vlen)
{
    size_t headerLen = 4;
    uint8_t *dp;

    /* 
     * The final length is not known now, thus the header will have to 
     * be build later. 
     */
    if (*dlen < headerLen) {
        SnmpStatsInc(SNMP_STAT_OUTTOOBIGS);
        return NULL;
    }
    *dlen -= headerLen;
    dp = data + headerLen;

    /* Build the name. */
    if ((dp = AsnOidBuild(dp, dlen, ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_OBJECT_ID, name, nlen)) == NULL) {
        SnmpStatsInc(SNMP_STAT_OUTTOOBIGS);
        return NULL;
    }
    /* Build the value. */
    switch (type) {
    case ASN_INTEGER:
        dp = AsnIntegerBuild(dp, dlen, type, (long *) value);
        break;
    case ASN_GAUGE:
    case ASN_COUNTER:
    case ASN_TIMETICKS:
    case ASN_UINTEGER:
        dp = AsnUnsignedBuild(dp, dlen, type, (uint32_t *) value);
        break;
    case ASN_COUNTER64:
        dp = AsnUnsigned64Build(dp, dlen, type, (UNSIGNED64 *) value);
        break;
    case ASN_OCTET_STR:
    case ASN_IPADDRESS:
    case ASN_OPAQUE:
    case ASN_NSAP:
        dp = AsnOctetStringBuild(dp, dlen, type, value, vlen);
        break;
    case ASN_OBJECT_ID:
        dp = AsnOidBuild(dp, dlen, type, (OID *) value, vlen / sizeof(OID));
        break;
    case ASN_NULL:
        dp = AsnNullBuild(dp, dlen, type);
        break;
    case ASN_BIT_STR:
        dp = AsnBitStringBuild(dp, dlen, type, value, vlen);
        break;
    case SNMP_NOSUCHOBJECT:
    case SNMP_NOSUCHINSTANCE:
    case SNMP_ENDOFMIBVIEW:
        dp = AsnNullBuild(dp, dlen, type);
        break;
    default:
        SnmpStatsInc(SNMP_STAT_OUTBADVALUES);
        return NULL;
    }
    /* Now build the header. */
    if (dp) {
        size_t dummyLen = (dp - data) - headerLen;
        AsnSequenceBuild(data, &dummyLen, ASN_SEQUENCE | ASN_CONSTRUCTOR, dummyLen);
    } else {
        SnmpStatsInc(SNMP_STAT_OUTTOOBIGS);
    }
    return dp;
}

/*@}*/
#endif // HAVE_NET
