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

#include <sys/types.h>

#include <stdlib.h>
#include <string.h>
//#include <memdebug.h>

#include <compat/nutos.h>

#include <kernel/snmp/asn1.h>

/*
 * Abstract Syntax Notation One, ASN.1 as defined in ISO/IS 8824 and 
 * ISO/IS 8825. This implements a subset of the above international 
 * standards that is sufficient for SNMP.
 */

/*!
 * \brief Interpret the length of the current object.
 *
 * @param data   Pointer to start of length field.
 * @param length Pointer to the variable that receives the value of this 
 *               length field.
 *
 * \return A pointer to the first byte after this length field (aka: the 
 *         start of the data field). Returns NULL on any error.
 */
static CONST uint8_t *AsnLenParse(CONST uint8_t * data, uint32_t * length)
{
    uint8_t lengthbyte = *data++;

    if (lengthbyte & ASN_LONG_LEN) {
        /* Long length. */
        lengthbyte &= ~ASN_LONG_LEN;
        if (lengthbyte == 0 || lengthbyte > sizeof(long)) {
            return NULL;
        }
        *length = 0;
        while (lengthbyte--) {
            *length <<= 8;
            *length |= *data++;
        }
    } else {
        /* Short length. */
        *length = lengthbyte;
    }
    return data;
}

/*!
 * \brief Build an ASN header for a specified length.
 *
 * \param data       Pointer to start of the object.
 * \param datalength Contains the number of available bytes following 
 *                   the start of the object. On exit, it is returned 
 *                   as the number of available bytes following the
 *                   encoded length of this object.
 * \param length     Length of object.
 *
 * \return A pointer to the first byte of the contents of this object.
 *         Returns NULL on any error.
 */
static uint8_t *AsnLenBuild(uint8_t * data, size_t * datalength, size_t length)
{
    if (length < 0x80) {
        /* Check for buffer overflow. */
        if (*datalength < 1) {
            return NULL;
        }
        *datalength -= 1;
        *data++ = (uint8_t) length;
    } else if (length <= 0xFF) {
        /* Check for buffer overflow. */
        if (*datalength < 2) {
            return NULL;
        }
        *datalength -= 2;
        *data++ = (uint8_t) (0x01 | ASN_LONG_LEN);
        *data++ = (uint8_t) length;
    } else {
        /* Check for buffer overflow. */
        if (*datalength < 3) {
            return NULL;
        }
        *datalength -= 3;
        *data++ = (uint8_t) (0x02 | ASN_LONG_LEN);
        *data++ = (uint8_t) (((unsigned) length >> 8) & 0xFF);
        *data++ = (uint8_t) (length & 0xFF);
    }
    return data;
}

/*!
 * \brief Interpret the ID and length of the next object.
 *
 * \param data       Pointer to start of the object.
 * \param datalength Contains the number of valid bytes following the 
 *                   start of the object. On exit, it is returned as 
 *                   the number of valid bytes following the ID and
 *                   length.
 * \param type       Pointer to the variable that receives the ASN type 
 *                   of the object.
 * \return A pointer to the first byte following ID and length (aka the 
 *         start of the data field). Returns NULL on any error.
 */
CONST uint8_t *AsnHeaderParse(CONST uint8_t * data, size_t * datalength, uint8_t * type)
{
    size_t header_len;
    uint32_t asn_length;
    CONST uint8_t *bufp = data;

    if (*datalength <= 0) {
        return NULL;
    }

    /*
     * The first byte of the header is the type. This only
     * works on data types < 30, i.e. no extension octets.
     */
    *type = *bufp;
    if ((*type & ASN_EXTENSION_ID) == ASN_EXTENSION_ID) {
        return NULL;
    }

    /*
     * Interpret the length (short or long) of this object.
     */
    if ((bufp = AsnLenParse(bufp + 1, &asn_length)) == NULL) {
        return NULL;
    }

    header_len = bufp - data;
    /* Data length exceeds packet size. */
    if (header_len + asn_length > *datalength) {
        return NULL;
    }
    *datalength = (int) asn_length;

    return bufp;
}

/*!
 * \brief Build an ASN header for an object with a given ID and length.
 *
 *  This only works on data types < 30, i.e. no extension octets.
 *  The maximum length is 0xFFFF;
 *
 * \param data       Pointer to start of object.
 * \param datalength Contains the number of available bytes following 
 *                   the start of the object. On exit, it is returned 
 *                   as the number of available bytes following the
 *                   encoded ID and length of this object.
 * \param type       ASN type of the object.
 * \param length     Length of object.
 *
 * \return Returns a pointer to the first byte of the contents of this object.
 *          Returns NULL on any error.
 */
uint8_t *AsnHeaderBuild(uint8_t * data, size_t * datalength, uint8_t type, size_t length)
{
    if (*datalength < 1) {
        return NULL;
    }
    *data++ = type;
    (*datalength)--;

    return AsnLenBuild(data, datalength, length);
}

/*!
 * \brief Check the type and get the length of the next object.
 *
 * Similare to AsnHeaderParse, but tests for expected type.
 *
 * \param data       Pointer to start of the object.
 * \param datalength Contains the number of valid bytes following the 
 *                   start of the object. On exit, it is returned as 
 *                   the number of valid bytes following the ID and
 *                   length.
 * \param type       The expected ASN type of the object.
 *
 * \return A pointer to the first byte following ID and length (aka the 
 *         start of the data field). Returns NULL on any error.
 */
CONST uint8_t *AsnSequenceParse(CONST uint8_t * data, size_t * datalength, uint8_t type)
{
    uint8_t t;

    if ((data = AsnHeaderParse(data, datalength, &t)) != NULL) {
        if (t != type) {
            data = NULL;
        }
    }
    return data;
}

/*!
 * \brief Build an ASN header for a sequence with a given type and length.
 *
 *  This only works on data types < 30, i.e. no extension octets.
 *  The maximum length is 0xFFFF;
 *
 * \param data       Pointer to start of object.
 * \param datalength Contains the number of available bytes following 
 *                   the start of the object. On exit, it is returned 
 *                   as the number of available bytes following the
 *                   encoded ID and length of this object.
 * \param type       ASN type of the object.
 * \param length     Length of object.
 *
 * \return Returns a pointer to the first byte of the contents of this object.
 *          Returns NULL on any error.
 */
uint8_t *AsnSequenceBuild(uint8_t * data, size_t * datalength, uint8_t type, size_t length)
{
    if (*datalength < 4) {
        /* Not enough space in output packet. */
        return NULL;
    }

    *datalength -= 4;
    *data++ = type;
    *data++ = (uint8_t) (0x02 | ASN_LONG_LEN);
    *data++ = (uint8_t) (((unsigned) length >> 8) & 0xFF);
    *data++ = (uint8_t) length;

    return data;
}

/*!
 * \brief Pull a long out of an ASN integer type.
 *
 * \param data       Pointer to start of the object.
 * \param datalength Contains the number of valid bytes following the 
 *                   start of the object. On exit, it is returned as 
 *                   the number of valid bytes following the end of 
 *                   this object.
 * \param type       Pointer to the variable that receives the ASN type 
 *                   of the object.
 * \param intp       Pointer to the variable that receives the value
 *                   of the object.
 *
 * \return Pointer to the first byte past the end of this object 
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
CONST uint8_t *AsnIntegerParse(CONST uint8_t * data, size_t * datalength, uint8_t * type, long *intp)
{
    CONST uint8_t *bufp = data;
    uint32_t asn_length;

    /* Get the type. */
    *type = *bufp++;
    /* Parse the length. */
    if ((bufp = AsnLenParse(bufp, &asn_length)) == NULL) {
        return NULL;
    }
    /* Check for overflow. */
    if (asn_length > sizeof(long) || asn_length + (bufp - data) > *datalength) {
        return NULL;
    }
    /* Update the number of valid bytes. */
    *datalength -= asn_length + (bufp - data);
    /* Determine sign. */
    if (*bufp & 0x80) {
        *intp = -1;
    } else {
        *intp = 0;
    }
    /* Retrieve the value. */
    while (asn_length--) {
        *intp <<= 8;
        *intp |= *bufp++;
    }
    return bufp;
}

/*!
 * \brief Build an ASN object containing an integer.
 *
 * \param data       Pointer to start of output buffer
 * \param datalength Contains the number of available bytes following 
 *                   the start of the object. On exit, it is returned 
 *                   as the number of available bytes following the end 
 *                   of this object.
 * \param type       ASN type of the object.
 * \param intp       Value of the object.
 * 
 * \return A pointer to the first byte past the end of this object
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
uint8_t *AsnIntegerBuild(uint8_t * data, size_t * datalength, uint8_t type, long *intp)
{
    uint32_t mask;
    long value = *intp;
    size_t size = sizeof(long);

    /*
     * Truncate unnecessary bytes off of the most significant end of 
     * this 2's complement integer. Skip any leading sequence of 
     * 9 consecutive 1's or 0's.
     */
    mask = 0x1FFUL << ((8 * (sizeof(long) - 1)) - 1);
    while (((value & mask) == 0 || (value & mask) == mask) && size > 1) {
        size--;
        value <<= 8;
    }
    /* We know the size, so let's build the header. */
    if ((data = AsnHeaderBuild(data, datalength, type, size)) == NULL) {
        return NULL;
    }
    /* Check if there's enough space for the value. */
    if (*datalength < size) {
        return NULL;
    }
    *datalength -= size;

    /* Store the value, MSB first. */
    mask = 0xFFUL << (8 * (sizeof(long) - 1));
    while (size--) {
        *data++ = (uint8_t) ((value & mask) >> (8 * (sizeof(long) - 1)));
        value <<= 8;
    }
    return data;
}

/*!
 * \brief Pull an unsigned long out of an ASN integer type.
 *
 * \param data       Pointer to start of the object.
 * \param datalength Contains the number of valid bytes following the 
 *                   start of the object. On exit, it is returned as 
 *                   the number of valid bytes following the end of 
 *                   this object.
 * \param type       Pointer to the variable that receives the ASN type 
 *                   of the object.
 * \param intp       Pointer to the variable that receives the value
 *                   of the object.
 *
 * \return Pointer to the first byte past the end of this object 
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
CONST uint8_t *AsnUnsignedParse(CONST uint8_t * data, size_t * datalength, uint8_t * type, uint32_t * intp)
{
    CONST uint8_t *bufp = data;
    uint32_t asn_length;

    /* Get the type. */
    *type = *bufp++;
    /* Parse the length. */
    if ((bufp = AsnLenParse(bufp, &asn_length)) == NULL) {
        return NULL;
    }
    /* Check for length overflow. */
    if (asn_length > sizeof(long) + 1 || (asn_length == sizeof(long) + 1 && *bufp != 0x00)) {
        return NULL;
    }
    /* Check for sufficient data. */
    if (asn_length + (bufp - data) > *datalength) {
        return NULL;
    }
    /* Update the number of valid bytes. */
    *datalength -= (int) asn_length + (bufp - data);
    /* Determine sign. */
    if (*bufp & 0x80) {
        *intp = -1;
    } else {
        *intp = 0;
    }
    /* Retrieve the value. */
    while (asn_length--) {
        *intp <<= 8;
        *intp |= *bufp++;
    }
    return bufp;
}

/*!
 * \brief Build an ASN object containing an unsigned integer.
 *
 * \param data       Pointer to start of output buffer
 * \param datalength Contains the number of available bytes following 
 *                   the start of the object. On exit, it is returned 
 *                   as the number of available bytes following the end 
 *                   of this object.
 * \param type       ASN type of the object.
 * \param intp       Value of the object.
 * 
 * \return A pointer to the first byte past the end of this object
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
uint8_t *AsnUnsignedBuild(uint8_t * data, size_t * datalength, uint8_t type, uint32_t * intp)
{
    int msb;
    uint32_t mask;
    uint32_t value = *intp;
    size_t size = sizeof(uint32_t);

    /* Check if MSB is set. */
    if (value & (0x80UL << (8 * (sizeof(long) - 1)))) {
        msb = 1;
        size++;
    } else {
        msb = 0;
        /*
         * Truncate unnecessary bytes off of the most significant end.
         * Skip any leading sequence of 9 consecutive 1's or 0's.
         */
        mask = 0x1FFUL << ((8 * (sizeof(long) - 1)) - 1);
        while ((((value & mask) == 0) || ((value & mask) == mask)) && size > 1) {
            size--;
            value <<= 8;
        }
    }
    /* We know the size, so let's build the header. */
    if ((data = AsnHeaderBuild(data, datalength, type, size)) == NULL) {
        return NULL;
    }
    /* Check if there's enough space for the value. */
    if (*datalength < size) {
        return NULL;
    }
    *datalength -= size;
    /* Add leading null byte if MSB set. */
    if (msb) {
        *data++ = '\0';
        size--;
    }
    /* Store the value, MSB first. */
    mask = 0xFFUL << (8 * (sizeof(long) - 1));
    while (size--) {
        *data++ = (uint8_t) ((value & mask) >> (8 * (sizeof(long) - 1)));
        value <<= 8;
    }
    return data;
}

/*!
 * \brief Pulls a string out of an ASN octet string type.
 *
 * \param data       Pointer to start of the object.
 * \param datalength Contains the number of valid bytes following the 
 *                   start of the object. On exit, it is returned as 
 *                   the number of valid bytes following the end of 
 *                   this object.
 * \param type       Pointer to the variable that receives the ASN type 
 *                   of the object.
 * \param string     Pointer to the variable that receives the value
 *                   of the object.
 * \param strlength  Contains the size of the string buffer on entry.
 *                   On exit, it is returned as the number of bytes 
 *                   stored in the string buffer.
 *
 * \return Pointer to the first byte past the end of this object 
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
CONST uint8_t *AsnOctetStringParse(CONST uint8_t * data, size_t * datalength, uint8_t * type, uint8_t * string, size_t * strlength)
{
    CONST uint8_t *bufp = data;
    uint32_t asn_length;

    /* Get the type. */
    *type = *bufp++;
    /* Get the length. */
    if ((bufp = AsnLenParse(bufp, &asn_length)) == NULL) {
        return NULL;
    }
    /* Check for overflow. */
    if (asn_length > *strlength || asn_length + (bufp - data) > *datalength) {
        return NULL;
    }
    if (asn_length) {
        /* Store value and update lengths. */
        memcpy(string, bufp, asn_length);
        *strlength = (size_t) asn_length;
    }
    *datalength -= (size_t) (asn_length + (bufp - data));

    return bufp + asn_length;
}

/*!
 * \brief Build an ASN object containing an octet string.
 *
 * \param data       Pointer to start of output buffer
 * \param datalength Contains the number of available bytes following 
 *                   the start of the object. On exit, it is returned 
 *                   as the number of available bytes following the end 
 *                   of this object.
 * \param type       ASN type of the object.
 * \param string     Pointer to the value. If NULL, the octet string will
 *                   be filled with zeros.
 * \param strlength  Number of bytes in the string value.
 * 
 * \return A pointer to the first byte past the end of this object
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
uint8_t *AsnOctetStringBuild(uint8_t * data, size_t * datalength, uint8_t type, CONST uint8_t * string, size_t strlength)
{
    if ((data = AsnHeaderBuild(data, datalength, type, strlength)) == NULL) {
        return NULL;
    }
    if (strlength) {
        /* Check for overflow. */
        if (*datalength < strlength) {
            return NULL;
        }
        *datalength -= strlength;
        if (string) {
            memcpy(data, string, strlength);
        } else {
            memset(data, 0, strlength);
        }
        data += strlength;
    }
    return data;
}

/*!
 * \brief Pulls an object identifier out of an ASN object ID type.
 *
 * \param data        Pointer to start of the object.
 * \param datalength  Contains the number of valid bytes following the 
 *                    start of the object. On exit, it is returned as 
 *                    the number of valid bytes following the end of 
 *                    this object.
 * \param type        Pointer to the variable that receives the ASN type 
 *                    of the object.
 * \param objid       Pointer to the variable that receives the object
 *                    identifier.
 * \param objidlength Points to a variable that contains the size of the 
 *                    output buffer on entry. On exit, it is returned as 
 *                    the number of sub IDs stored in the output buffer.
 *
 * \return Pointer to the first byte past the end of this object 
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
CONST uint8_t *AsnOidParse(CONST uint8_t * data, size_t * datalength, uint8_t * type, OID * objid, size_t * objidlength)
{
    CONST uint8_t *bufp = data;
    OID *oidp = objid + 1;
    uint32_t subidentifier;
    long length;
    uint32_t asn_length;

    /* Get type and length. */
    *type = *bufp++;
    if ((bufp = AsnLenParse(bufp, &asn_length)) == NULL) {
        return NULL;
    }
    /* Check for overflow and update number of remaining bytes. */
    if (asn_length + (bufp - data) > *datalength) {
        /* Message overflow. */
        return NULL;
    }
    *datalength -= (int) asn_length + (bufp - data);

    /* Handle invalid object ID encodings of the form 06 00 robustly. */
    if (asn_length == 0) {
        objid[0] = objid[1] = 0;
    }
    length = asn_length;
    /* Account for expansion of first byte. */
    (*objidlength)--;
    while (length > 0 && (*objidlength)-- > 0) {
        subidentifier = 0;
        /*
         * Shift and add in low order 7 bits.
         * Last byte has high bit clear.
         */
        do {
            subidentifier = (subidentifier << 7) + (*(uint8_t *) bufp & ~ASN_BIT8);
            length--;
        } while (*bufp++ & ASN_BIT8);
        *oidp++ = (OID) subidentifier;
    }

    /*
     * The first two subidentifiers are encoded into the first component 
     * with the value (X * 40) + Y, where
     *
     *  X is the value of the first subidentifier.
     *  Y is the value of the second subidentifier.
     */
    subidentifier = objid[1];
    if (subidentifier == 0x2B) {
        objid[0] = 1;
        objid[1] = 3;
    } else {
        objid[1] = (uint8_t) (subidentifier % 40);
        objid[0] = (uint8_t) ((subidentifier - objid[1]) / 40);
    }
    *objidlength = oidp - objid;

    return bufp;
}

/*!
 * \brief Build an ASN object identifier.
 *
 * \param data        Pointer to start of the object.
 * \param datalength  Contains the number of available bytes following 
 *                    the start of the object. On exit, it is returned 
 *                    as the number of available bytes following the end 
 *                    of this object.
 * \param type        ASN type of the object.
 * \param objid       Pointer to the object identifier.
 * \param objidlength Number of sub IDs in the object identifier.
 *
 * \return Pointer to the first byte past the end of this object 
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
uint8_t *AsnOidBuild(uint8_t * data, size_t * datalength, uint8_t type, CONST OID * objid, size_t objidlength)
{
    uint8_t *buf;
    uint32_t objid_val;
    uint32_t first_objid_val;
    size_t asnlength;
    CONST OID *op = objid;
    size_t i;

    if (objidlength == 0) {
        first_objid_val = 0;
        objidlength++;
    } else if (objid[0] > 2) {
        /* First sub identifier is bad. */
        return NULL;
    } else if (objidlength == 1) {
        first_objid_val = op[0] * 40;
    } else if (op[1] > 40 && op[0] < 2) {
        /* Second sub identifier is bad. */
        return NULL;
    } else if (objidlength > MAX_OID_LEN) {
        /* Bad object ID length. */
        return NULL;
    } else {
        first_objid_val = op[0] * 40 + op[1];
        objidlength--;
    }

    /*
     * Determine the number of bytes needed to store the encoded value.
     */
    if ((buf = malloc(MAX_OID_LEN * sizeof(OID))) == NULL) {
        return NULL;
    }
    asnlength = 0;
    for (i = 0; i < objidlength; i++) {
        if (i) {
            objid_val = *op++;
        } else {
            objid_val = first_objid_val;
            op = objid + 2;
        }
        if (objid_val < 0x80UL) {
            buf[i] = 1;
            asnlength += 1;
        } else if (objid_val < 0x4000UL) {
            buf[i] = 2;
            asnlength += 2;
        } else if (objid_val < 0x200000UL) {
            buf[i] = 3;
            asnlength += 3;
        } else if (objid_val < 0x10000000UL) {
            buf[i] = 4;
            asnlength += 4;
        } else {
            buf[i] = 5;
            asnlength += 5;
        }
    }
    /* Build the header after knowing the length. */
    if ((data = AsnHeaderBuild(data, datalength, type, asnlength)) == NULL || asnlength > *datalength) {
        free(buf);
        return NULL;
    }

    /*
     * Store the encoded OID value 
     */
    for (i = 0; i < objidlength; i++) {
        if (i) {
            objid_val = *op++;
        } else {
            objid_val = first_objid_val;
            op = objid + 2;
        }
        switch (buf[i]) {
        case 1:
            *data++ = (uint8_t) objid_val;
            break;
        case 2:
            *data++ = (uint8_t) ((objid_val >> 7) | 0x80);
            *data++ = (uint8_t) (objid_val & 0x07f);
            break;
        case 3:
            *data++ = (uint8_t) ((objid_val >> 14) | 0x80);
            *data++ = (uint8_t) ((objid_val >> 7 & 0x7f) | 0x80);
            *data++ = (uint8_t) (objid_val & 0x07f);
            break;
        case 4:
            *data++ = (uint8_t) ((objid_val >> 21) | 0x80);
            *data++ = (uint8_t) ((objid_val >> 14 & 0x7f) | 0x80);
            *data++ = (uint8_t) ((objid_val >> 7 & 0x7f) | 0x80);
            *data++ = (uint8_t) (objid_val & 0x07f);
            break;
        case 5:
            *data++ = (uint8_t) ((objid_val >> 28) | 0x80);
            *data++ = (uint8_t) ((objid_val >> 21 & 0x7f) | 0x80);
            *data++ = (uint8_t) ((objid_val >> 14 & 0x7f) | 0x80);
            *data++ = (uint8_t) ((objid_val >> 7 & 0x7f) | 0x80);
            *data++ = (uint8_t) (objid_val & 0x07f);
            break;
        }
    }

    *datalength -= asnlength;
    free(buf);

    return data;
}

/*!
 * \brief Parse an ASN null type.
 *
 * \param data        Pointer to start of the object.
 * \param datalength  Contains the number of valid bytes following the 
 *                    start of the object. On exit, it is returned as 
 *                    the number of valid bytes following the end of 
 *                    this object.
 * \param type        Pointer to the variable that receives the ASN type 
 *                    of the object.
 *
 * \return Pointer to the first byte past the end of this object 
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
CONST uint8_t *AsnNullParse(CONST uint8_t * data, size_t * datalength, uint8_t * type)
{
    CONST uint8_t *bufp = data;
    uint32_t asn_length;

    *type = *bufp++;
    if ((bufp = AsnLenParse(bufp, &asn_length)) == NULL) {
        return NULL;
    }
    if (asn_length) {
        return NULL;
    }
    *datalength -= (bufp - data);

    return bufp;
}

/*!
 * \brief Build an ASN null object.
 *
 * \param data        Pointer to start of the object.
 * \param datalength  Contains the number of available bytes following 
 *                    the start of the object. On exit, it is returned 
 *                    as the number of available bytes following the end 
 *                    of this object.
 * \param type        ASN type of the object.
 *
 * \return Pointer to the first byte past the end of this object 
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
uint8_t *AsnNullBuild(uint8_t * data, size_t * datalength, uint8_t type)
{
    return AsnHeaderBuild(data, datalength, type, 0);
}

/*!
 * \brief Pull a bitstring out of an ASN bitstring type.
 *
 * \param data       Pointer to start of the object.
 * \param datalength Contains the number of valid bytes following the 
 *                   start of the object. On exit, it is returned as 
 *                   the number of valid bytes following the end of 
 *                   this object.
 * \param type       Pointer to the variable that receives the ASN type 
 *                   of the object.
 * \param string     Pointer to the variable that receives the value
 *                   of the object.
 * \param strlength  Contains the size of the string buffer on entry.
 *                   On exit, it is returned as the number of bytes 
 *                   stored in the string buffer.
 *
 * \return Pointer to the first byte past the end of this object 
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
CONST uint8_t *AsnBitStringParse(CONST uint8_t * data, size_t * datalength, uint8_t * type, uint8_t * string, size_t * strlength)
{
    CONST uint8_t *bufp = data;
    uint32_t asn_length;

    *type = *bufp++;
    if ((bufp = AsnLenParse(bufp, &asn_length)) == NULL) {
        return NULL;
    }
    if (asn_length + (bufp - data) > *datalength) {
        return NULL;
    }
    if (asn_length < 1 || (size_t) asn_length > *strlength) {
        return NULL;
    }
    if (*bufp > 7) {
        return NULL;
    }
    memcpy(string, bufp, asn_length);
    *strlength = (size_t) asn_length;
    *datalength -= (size_t) asn_length + (bufp - data);

    return bufp + asn_length;
}

/*!
 * \brief Build an ASN bit string.
 *
 * \param data       Pointer to start of output buffer
 * \param datalength Contains the number of available bytes following 
 *                   the start of the object. On exit, it is returned 
 *                   as the number of available bytes following the end 
 *                   of this object.
 * \param type       ASN type of the object.
 * \param string     Pointer to the value. If NULL, the octet string will
 *                   be filled with zeros.
 * \param strlength  Number of bytes in the string value.
 * 
 * \return A pointer to the first byte past the end of this object
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
uint8_t *AsnBitStringBuild(uint8_t * data, size_t * datalength, uint8_t type, CONST uint8_t * string, size_t strlength)
{
    if ((data = AsnHeaderBuild(data, datalength, type, strlength)) == NULL) {
        return NULL;
    }
    if (strlength) {
        if (strlength > *datalength) {
            return NULL;
        }
        memcpy((char *) data, (char *) string, strlength);
        *datalength -= strlength;
        data += strlength;
    }
    return data;
}

/*!
 * \brief Pull a 64 bit unsigned long out of an ASN integer type.
 *
 * \param data       Pointer to start of the object.
 * \param datalength Contains the number of valid bytes following the 
 *                   start of the object. On exit, it is returned as 
 *                   the number of valid bytes following the end of 
 *                   this object.
 * \param type       Pointer to the variable that receives the ASN type 
 *                   of the object.
 * \param intp       Pointer to the variable that receives the value
 *                   of the object.
 *
 * \return Pointer to the first byte past the end of this object 
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
CONST uint8_t *AsnUnsigned64Parse(CONST uint8_t * data, size_t * datalength, uint8_t * type, UNSIGNED64 * cp)
{
    CONST uint8_t *bufp = data;
    uint32_t asn_length;
    uint32_t low = 0;
    uint32_t high = 0;
    size_t intsize = 4;

    *type = *bufp++;
    if ((bufp = AsnLenParse(bufp, &asn_length)) == NULL) {
        return NULL;
    }
    if (asn_length + (bufp - data) > *datalength) {
        return NULL;
    }
    if ((asn_length > (intsize * 2 + 1)) || ((asn_length == (intsize * 2) + 1) && *bufp != 0x00)) {
        return NULL;
    }
    *datalength -= (int) asn_length + (bufp - data);
    if (*bufp & 0x80) {
        low = (uint32_t) - 1;
        high = (uint32_t) - 1;
    }
    while (asn_length--) {
        high = (high << 8) | ((low & 0xFF000000) >> 24);
        low = (low << 8) | *bufp++;
    }
    cp->low = low;
    cp->high = high;
    return bufp;
}

/*!
 * \brief Build an ASN object containing a 64 bit unsigned integer.
 *
 * \param data       Pointer to start of output buffer
 * \param datalength Contains the number of available bytes following 
 *                   the start of the object. On exit, it is returned 
 *                   as the number of available bytes following the end 
 *                   of this object.
 * \param type       ASN type of the object.
 * \param intp       Value of the object.
 * 
 * \return A pointer to the first byte past the end of this object
 *         (i.e. the start of the next object). Returns NULL on any 
 *         error.
 */
uint8_t *AsnUnsigned64Build(uint8_t * data, size_t * datalength, uint8_t type, CONST UNSIGNED64 * cp)
{
    uint32_t low;
    uint32_t high;
    uint32_t mask;
    uint32_t mask2;
    int add_null_byte = 0;
    size_t intsize;

    intsize = 8;
    low = cp->low;
    high = cp->high;
    mask = 0xFFUL << (8 * (sizeof(long) - 1));
    /*
     * mask is 0xFF000000 on a big-endian machine
     */
    if ((uint8_t) ((high & mask) >> (8 * (sizeof(long) - 1))) & 0x80) {
        /* if MSB is set */
        add_null_byte = 1;
        intsize++;
    }
    /*
     * Truncate "unnecessary" bytes off of the most significant end of 
     * this 2's complement integer.
     * There should be no sequence of 9 consecutive 1's or 0's at the most
     * significant end of the integer.
     */
    mask2 = 0x1FFUL << ((8 * (sizeof(long) - 1)) - 1);
    /*
     * mask2 is 0xFF800000 on a big-endian machine
     */
    while ((((high & mask2) == 0) || ((high & mask2) == mask2))
           && intsize > 1) {
        intsize--;
        high = (high << 8)
            | ((low & mask) >> (8 * (sizeof(long) - 1)));
        low <<= 8;
    }
    data = AsnHeaderBuild(data, datalength, type, intsize);
    if (data == NULL || *datalength < intsize) {
        return NULL;
    }
    *datalength -= intsize;
    if (add_null_byte == 1) {
        *data++ = '\0';
        intsize--;
    }
    while (intsize--) {
        *data++ = (uint8_t) ((high & mask) >> (8 * (sizeof(long) - 1)));
        high = (high << 8)
            | ((low & mask) >> (8 * (sizeof(long) - 1)));
        low <<= 8;

    }
    return data;
}
