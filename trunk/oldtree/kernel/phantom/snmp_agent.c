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

#define DEBUG_MSG_PREFIX "snmp"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <compat/nutos.h>


#include <sys/types.h>

#include <stdlib.h>
#include <string.h>
//#include <memdebug.h>

//#include <netinet/in.h>
//#include <arpa/inet.h>

#include <kernel/net/udp.h>


#include <kernel/snmp/snmp_config.h>
#include <kernel/snmp/snmp.h>
#include <kernel/snmp/snmp_api.h>
#include <kernel/snmp/snmp_auth.h>
#include <kernel/snmp/snmp_mib.h>
#include <kernel/snmp/snmp_agent.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

/*
 * Using this as a global had been derived from the original CMU code.
 * It is very ugly (shiffer), but may require some effort to transform
 * it into something local.
 */
static uint8_t *packet_end;

static void SetVariable(CONST uint8_t * var_val, uint8_t var_val_type, uint8_t * statP, size_t statLen)
{
    size_t buffersize = 1000;

    switch (var_val_type) {
    case ASN_INTEGER:
    case ASN_COUNTER:
    case ASN_GAUGE:
    case ASN_TIMETICKS:
        AsnIntegerParse(var_val, &buffersize, &var_val_type, (long *) statP);
        break;
    case ASN_OCTET_STR:
    case ASN_IPADDRESS:
    case ASN_OPAQUE:
        AsnOctetStringParse(var_val, &buffersize, &var_val_type, statP, &statLen);
        break;
    case ASN_OBJECT_ID:
        AsnOidParse(var_val, &buffersize, &var_val_type, (OID *) statP, &statLen);
        break;
    }
}

/*!
 * \brief Parse a list of variables.
 *
 * \param data       Pointer to the start of the list.
 * \param length     Contains the number of valid bytes following the 
 *                   start of the list.
 * \param out_data   Pointer to the output buffer.
 * \param out_length Number of bytes available in the output buffer.
 * \param index      Error index.
 * \param msgtype    Type of the incoming packet.
 * \param action     Action to perform, either SNMP_ACT_RESERVE1, 
 *                   SNMP_ACT_RESERVE2, SNMP_ACT_COMMIT, SNMP_ACT_ACTION 
 *                   or SNMP_ACT_FREE.
 *
 * \return 0 on success. Otherwise an error code is returned.
 *
 */
static int SnmpVarListParse(SNMP_SESSION * sess, CONST uint8_t * data, size_t length, uint8_t * out_data, size_t out_length,
                            long *index, int msgtype, int action)
{
    OID var_name[MAX_OID_LEN];
    size_t var_name_len;
    size_t var_val_len;
    uint8_t var_val_type;
    uint8_t *var_val;
    uint8_t statType;
    uint8_t *statP;
    size_t statLen;
    uint16_t acl;
    int exact, err;
    WMETHOD *wmethod;
    uint8_t *headerP;
    uint8_t *var_list_start;
    size_t dummyLen;
    int noSuchObject = 0;

    exact = (msgtype != SNMP_MSG_GETNEXT);
    /* Check if the list starts with a sequence header and get its length. */
    if ((data = AsnSequenceParse(data, &length, ASN_SEQUENCE | ASN_CONSTRUCTOR)) == NULL) {
        return SNMP_PARSE_ERROR;
    }

    /* Build ASN header. */
    headerP = out_data;
    if ((out_data = AsnSequenceBuild(out_data, &out_length, ASN_SEQUENCE | ASN_CONSTRUCTOR, 0)) == NULL) {
        return SNMP_BUILD_ERROR;
    }
    var_list_start = out_data;

    *index = 1;
    while (length > 0) {
        /* Get name and ASN1 encoded value of the next variable. */
        var_name_len = MAX_OID_LEN;
        if ((data = SnmpVarParse(data, &length, var_name, &var_name_len, &var_val_type, &var_val, &var_val_len)) == NULL) {
            return SNMP_PARSE_ERROR;
        }

        /* Now attempt to retrieve the variable on the local entity. */
        statP = SnmpMibFind(var_name, &var_name_len, &statType, &statLen, &acl, exact, &wmethod, &noSuchObject);

        /* Check access. */
        if (msgtype == SNMP_MSG_SET) {
            /* Make sure we have write access. */
            if (acl != ACL_RWRITE) {
                return sess->sess_version == SNMP_VERSION_1 ? SNMP_ERR_NOSUCHNAME : SNMP_ERR_NOTWRITABLE;
            }
            if (wmethod == NULL) {
                if (statP == NULL) {
                    return sess->sess_version == SNMP_VERSION_1 ? SNMP_ERR_NOSUCHNAME : SNMP_ERR_NOCREATION;
                }
                /* Check if the type and value is consistent with this entity's variable. */
                if (var_val_len > statLen || var_val_type != statType) {
                    return sess->sess_version == SNMP_VERSION_1 ? SNMP_ERR_BADVALUE : SNMP_ERR_WRONGTYPE;
                }
                /* Actually do the set if necessary. */
                if (action == SNMP_ACT_COMMIT) {
                    SetVariable(var_val, var_val_type, statP, statLen);
                }
            } else {
                err = (*wmethod) (action, var_val, var_val_type, var_val_len, var_name, var_name_len);

                /*
                 * Map the SNMPv2 error codes to SNMPv1 error codes (RFC 2089).
                 */
                if (err && sess->sess_version == SNMP_VERSION_1) {
                    switch (err) {
                    case SNMP_ERR_WRONGVALUE:
                    case SNMP_ERR_WRONGENCODING:
                    case SNMP_ERR_WRONGTYPE:
                    case SNMP_ERR_WRONGLENGTH:
                    case SNMP_ERR_INCONSISTENTVALUE:
                        err = SNMP_ERR_BADVALUE;
                        break;
                    case SNMP_ERR_NOACCESS:
                    case SNMP_ERR_NOTWRITABLE:
                    case SNMP_ERR_NOCREATION:
                    case SNMP_ERR_INCONSISTENTNAME:
                    case SNMP_ERR_AUTHORIZATIONERROR:
                        err = SNMP_ERR_NOSUCHNAME;
                        break;
                    default:
                        err = SNMP_ERR_GENERR;
                        break;
                    }
                    return err;
                }
            }
        } else {
            /* Retrieve the value and place it into the outgoing packet. */
            if (statP == NULL) {
                statLen = 0;
                if (exact) {
                    if (noSuchObject) {
                        statType = SNMP_NOSUCHOBJECT;
                    } else {
                        statType = SNMP_NOSUCHINSTANCE;
                    }
                } else {
                    statType = SNMP_ENDOFMIBVIEW;
                }
            }
            out_data = SnmpVarBuild(out_data, &out_length, var_name, var_name_len, statType, statP, statLen);
            if (out_data == NULL) {
                return SNMP_ERR_TOOBIG;
            }
        }
        (*index)++;
    }

    if (msgtype != SNMP_MSG_SET) {
        /*
         * Save a pointer to the end of the packet and
         * rebuild header with the actual lengths
         */
        packet_end = out_data;
        dummyLen = packet_end - var_list_start;
        if (AsnSequenceBuild(headerP, &dummyLen, ASN_SEQUENCE | ASN_CONSTRUCTOR, dummyLen) == NULL) {
            return SNMP_ERR_TOOBIG;
        }
    }
    *index = 0;

    return 0;
}

/*!
 * \brief Clone input packet.
 *
 * Creates a packet identical to the input packet, except for the error 
 * status and the error index which are set according to the specified
 * parameters.
 *
 * \return 0 upon success and -1 upon failure.
 */
static int SnmpCreateIdentical(SNMP_SESSION * sess, CONST uint8_t * snmp_in, uint8_t * snmp_out, size_t snmp_length, long errstat,
                               long errindex)
{
    uint8_t *data;
    uint8_t type;
    long dummy;
    size_t length;
    size_t headerLength;
    uint8_t *headerPtr;
    CONST uint8_t *reqidPtr;
    uint8_t *errstatPtr;
    uint8_t *errindexPtr;
    CONST uint8_t *varListPtr;

    /* Copy packet contents. */
    memcpy(snmp_out, snmp_in, snmp_length);
    length = snmp_length;
    if ((headerPtr = (uint8_t *) SnmpAuthParse(snmp_out, &length, sess->sess_id, &sess->sess_id_len, &dummy)) == NULL) {
        return -1;
    }
    sess->sess_id[sess->sess_id_len] = 0;

    if ((reqidPtr = AsnHeaderParse(headerPtr, &length, (uint8_t *) & dummy)) == NULL) {
        return -1;
    }
    headerLength = length;

    /* Request id. */
    if ((errstatPtr = (uint8_t *) AsnIntegerParse(reqidPtr, &length, &type, &dummy)) == NULL) {
        return -1;
    }
    /* Error status. */
    if ((errindexPtr = (uint8_t *) AsnIntegerParse(errstatPtr, &length, &type, &dummy)) == NULL) {
        return -1;
    }
    /* Error index. */
    if ((varListPtr = AsnIntegerParse(errindexPtr, &length, &type, &dummy)) == NULL) {
        return -1;
    }

    if ((data = AsnHeaderBuild(headerPtr, &headerLength, SNMP_MSG_RESPONSE, headerLength)) == NULL) {
        return -1;
    }
    length = snmp_length;
    type = (uint8_t) (ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER);
    if ((data = AsnIntegerBuild(errstatPtr, &length, type, &errstat)) != errindexPtr) {
        return -1;
    }
    if ((data = AsnIntegerBuild(errindexPtr, &length, type, &errindex)) != varListPtr) {
        return -1;
    }
    packet_end = snmp_out + snmp_length;

    return 0;
}

/*!
 * \brief Parse incoming and create outgoing packet.
 *
 * \param in_data  Pointer to the incoming packet.
 * \param in_len   Number of valid bytes in the incoming packet.
 * \param out_data Pointer to a buffer for the outgoing packet.
 * \param out_len  Pointer to the variable that receives the number of
 *                 bytes in the outgoing packet.
 * \param out_len  Pointer to a variable which contains the size of the
 *                 output buffer on entry. On exit, it is returned 
 *                 as the number of valid bytes in the output buffer.
 *
 * \return 0 upon success and -1 upon failure.
 */
int SnmpAgentProcessRequest(SNMP_SESSION * sess, CONST uint8_t * in_data, size_t in_len, uint8_t * out_data, size_t * out_len)
{
    long zero = 0;
    uint8_t msgtype;
    uint8_t type;
    long reqid;
    long errstat;
    long errindex;
    long dummyindex;
    uint8_t *out_auth;
    uint8_t *out_header;
    uint8_t *out_reqid;
    CONST uint8_t *data;
    size_t len;

    SnmpStatsInc(SNMP_STAT_INPKTS);

    /* Get version and community from the packet header. */
    len = in_len;
    sess->sess_id_len = sizeof(sess->sess_id) - 1;
    if ((data = SnmpAuthParse(in_data, &len, sess->sess_id, &sess->sess_id_len, &sess->sess_version)) == NULL) {
        SnmpStatsInc(SNMP_STAT_INASNPARSEERRS);
        return -1;
    }

    /* Check authentication. */
    if (sess->sess_version == SNMP_VERSION_1 || sess->sess_version == SNMP_VERSION_2C) {
        if (SnmpCommunityFind((char *) sess->sess_id, &sess->sess_read_view, &sess->sess_write_view)) {
            /* TODO: Create SNMPv2 report. */
            SnmpStatsInc(SNMP_STAT_INBADCOMMUNITYNAMES);
            return -1;
        }
    } else {
        /* Unsupported SNMP version. */
        SnmpStatsInc(SNMP_STAT_INBADVERSIONS);
        return -1;
    }

    /* Parse request header and check type. */
    if ((data = AsnHeaderParse(data, &len, &msgtype)) == NULL) {
        SnmpStatsInc(SNMP_STAT_INASNPARSEERRS);
        return -1;
    }
    if (msgtype == SNMP_MSG_GETBULK) {
        /* SNMPv2 bulk requests are not yet supported. */
        return -1;
    } else if (msgtype != SNMP_MSG_GET && msgtype != SNMP_MSG_GETNEXT && msgtype != SNMP_MSG_SET) {
        /* Bad request type. */
        return -1;
    }

    /* Parse request ID. */
    if ((data = AsnIntegerParse(data, &len, &type, &reqid)) == NULL) {
        SnmpStatsInc(SNMP_STAT_INASNPARSEERRS);
        return -1;
    }
    /* Parse error status. */
    if ((data = AsnIntegerParse(data, &len, &type, &errstat)) == NULL) {
        SnmpStatsInc(SNMP_STAT_INASNPARSEERRS);
        return -1;
    }
    /* Parse error index. */
    if ((data = AsnIntegerParse(data, &len, &type, &errindex)) == NULL) {
        SnmpStatsInc(SNMP_STAT_INASNPARSEERRS);
        return -1;
    }

    /*
     * Now start cobbling together what is known about the output packet. 
     * The final lengths are not known now, so they will have to be 
     * recomputed later.
     */
    out_auth = out_data;
    if ((out_header = SnmpAuthBuild(sess, out_auth, out_len, 0)) == NULL) {
        return -1;
    }
    if ((out_reqid = AsnSequenceBuild(out_header, out_len, SNMP_MSG_RESPONSE, 0)) == NULL) {
        return -1;
    }
    /* Return identical request ID. */
    type = (uint8_t) (ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER);
    if ((out_data = AsnIntegerBuild(out_reqid, out_len, type, &reqid)) == NULL) {
        return -1;
    }
    /* Assume that error status will be zero. */
    if ((out_data = AsnIntegerBuild(out_data, out_len, type, &zero)) == NULL) {
        return -1;
    }
    /* Assume that error index will be zero. */
    if ((out_data = AsnIntegerBuild(out_data, out_len, type, &zero)) == NULL) {
        return -1;
    }

    /*
     * Walk through the list of variables and retrieve each one, 
     * placing its value in the output packet.
     *
     * TODO: Handle bulk requests.
     */
    errstat = SnmpVarListParse(sess, data, len, out_data, *out_len, &errindex, msgtype, SNMP_ACT_RESERVE1);

    /*
     * Sets require 3 to 4 passes through the var_op_list. The first two 
     * passes verify that all types, lengths, and values are valid and 
     * may reserve resources and the third does the set and a fourth 
     * executes any actions. Then the identical GET RESPONSE packet is
     * returned.
     *
     * If either of the first two passes returns an error, another pass 
     * is made so that any reserved resources can be freed.
     */
    if (msgtype == SNMP_MSG_SET) {
        if (errstat == 0) {
            errstat = SnmpVarListParse(sess, data, len, out_data, *out_len, &errindex, msgtype, SNMP_ACT_RESERVE2);
        }
        if (errstat == 0) {
            errstat = SnmpVarListParse(sess, data, len, out_data, *out_len, &dummyindex, msgtype, SNMP_ACT_COMMIT);
            SnmpVarListParse(sess, data, len, out_data, *out_len, &dummyindex, msgtype, errstat ? SNMP_ACT_FREE : SNMP_ACT_ACTION);
            if (SnmpCreateIdentical(sess, in_data, out_auth, in_len, 0L, 0L)) {
                return -1;
            }
            *out_len = packet_end - out_auth;
            return 0;
        } else {
            SnmpVarListParse(sess, data, len, out_data, *out_len, &dummyindex, msgtype, SNMP_ACT_FREE);
        }
    }

    if (errstat) {
        /* Create an error response. */
        if (SnmpCreateIdentical(sess, in_data, out_auth, in_len, errstat, errindex)) {
            return -1;
        }
        *out_len = packet_end - out_auth;
        return 0;
    }

    /* 
     * Re-encode the headers with the real lengths.
     */
    *out_len = packet_end - out_header;
    out_data = AsnSequenceBuild(out_header, out_len, SNMP_MSG_RESPONSE, packet_end - out_reqid);
    if (out_data != out_reqid) {
        return -1;
    }
    *out_len = packet_end - out_auth;
    out_data = SnmpAuthBuild(sess, out_auth, out_len, packet_end - out_header);
    *out_len = packet_end - out_auth;

    (void) out_data;

    return 0;
}

/*!
 * \brief Run SNMP agent.
 *
 * Normally runs in an endless loop, which is only left in case of an error.
 *
 * \param sock UDP socket to use.
 *
 * \return Always -1.
 */
int SnmpAgent(void * sock)
{
    int rc = -1;

    //uint32_t raddr;
    //uint16_t rport;
    i4sockaddr saddr;


    size_t out_len;
    uint8_t *in_data = malloc(SNMP_MAX_LEN);
    uint8_t *out_data = malloc(SNMP_MAX_LEN);
    SNMP_SESSION *sess = malloc(sizeof(SNMP_SESSION));

    if (in_data && out_data && sess) {
        for (;;) {
            //rc = NutUdpReceiveFrom(sock, &raddr, &rport, in_data, SNMP_MAX_LEN, 0);
            rc = udp_recvfrom( sock, in_data, SNMP_MAX_LEN, &saddr, 0, 0 );
            if (rc < 0) {
                //break;
                continue;
            }
            out_len = SNMP_MAX_LEN;
            memset(sess, 0, sizeof(SNMP_SESSION));
            if (SnmpAgentProcessRequest(sess, in_data, (size_t) rc, out_data, &out_len) == 0)
            {
                rc = udp_sendto( sock, out_data, out_len, &saddr);

                //if (NutUdpSendTo(sock, raddr, rport, out_data, out_len) == 0) {
                if(rc == 0) {
                    SnmpStatsInc(SNMP_STAT_OUTPKTS);
                }
            }
        }
    } else {
        rc = -1;
    }
    if (in_data) {
        free(in_data);
    }
    if (out_data) {
        free(out_data);
    }
    if (sess) {
        free(sess);
    }
    return rc;
}

/*@}*/
#endif // HAVE_NET
