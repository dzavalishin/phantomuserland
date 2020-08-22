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
#include <kernel/snmp/snmp_agent.h>
#include <kernel/snmp/snmp_auth.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

/*!
 * \brief Parse header of community string based message.
 *
 * Retrieves version and community.
 *
 * \param data    Points to the message.
 * \param length  Bytes left in message.
 * \param sidp    Pointer to a buffer that receives the community string.
 * \param slen    Length of the community string.
 * \param version Message version
 */
CONST uint8_t *SnmpAuthParse(CONST uint8_t * data, size_t * length, uint8_t * sidp, size_t * slen, long *version)
{
    uint8_t type = ASN_SEQUENCE | ASN_CONSTRUCTOR;

    /* Check header type. */
    if ((data = AsnSequenceParse(data, length, type)) == NULL) {
        return NULL;
    }

    /* Get SNMP version. */
    if ((data = AsnIntegerParse(data, length, &type, version)) == NULL) {
        return NULL;
    }

    /* Get SNMP community. */
    if ((data = AsnOctetStringParse(data, length, &type, sidp, slen)) == NULL) {
        return NULL;
    }
    if (*version == SNMP_VERSION_1) {
        sidp[*slen] = '\0';
    }
    return data;
}

/*!
 * \brief Build header of community string based message.
 */
uint8_t *SnmpAuthBuild(SNMP_SESSION * session, uint8_t * data, size_t * length, size_t messagelen)
{
    data = AsnSequenceBuild(data, length, ASN_SEQUENCE | ASN_CONSTRUCTOR, messagelen + session->sess_id_len + 5);
    if (data == NULL) {
        return NULL;
    }
    data = AsnIntegerBuild(data, length, ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER, (long *) &session->sess_version);
    if (data == NULL) {
        return NULL;
    }
    data = AsnOctetStringBuild(data, length, ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_OCTET_STR, session->sess_id, session->sess_id_len);
    if (data == NULL) {
        return NULL;
    }
    return data;
}

/*@}*/
#endif // HAVE_NET
