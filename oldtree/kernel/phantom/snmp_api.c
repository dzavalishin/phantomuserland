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

#include <string.h>

#include <compat/nutos.h>

#include <kernel/snmp/snmp_api.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

/*
 * generic statistics counter functions 
 */
static uint32_t statistics[SNMP_STAT_MAX];


/*!
 * \brief Compare object identifiers up to a specified length.
 *
 * \param name1 First object identifier.
 * \param name2 Second object identifier.
 * \param len   Number of sub identifiers to compare.
 */
int SnmpOidLenCmp(CONST OID * name1, CONST OID * name2, size_t len)
{
    /* Find first non-matching element. */
    while (len--) {
        if (*name1 < *name2) {
            /* First is lower than second. */
            return -1;
        }
        if (*name1++ > *name2++) {
            /* First is larger than second. */
            return 1;
        }
    }
    /* Elements match up to the given length. */
    return 0;
}

/*!
 * \brief Compare object identifiers.
 *
 * \param name1 First object identifier.
 * \param len1  Length of first object identifier.
 * \param name2 Second object identifier.
 * \param len2  Length of second object identifier.
 *
 * \return 0 if both are equal, 1 if first element is larger or -1
 *         if first element is lower than the second one.
 */
int SnmpOidCmp(CONST OID * name1, size_t len1, CONST OID * name2, size_t len2)
{
    /* Compare elements up to the length of shortest name. */
    int rc = SnmpOidLenCmp(name1, name2, (len1 < len2) ? len1 : len2);
    /* If equal, compare lengths. */
    if (rc == 0) {
        if (len1 < len2) {
            rc = -1;
        } else if (len1 > len2) {
            rc = 1;
        }
    }
    return rc;
}

/*!
 * \brief Compare object identifier with tree element.
 *
 * \param name1 Object identifier.
 * \param len1  Length of object identifier.
 * \param name2 Tree identifier.
 * \param len2  Length of tree identifier.
 *
 * \return 0 if the object identifier is part of the subtree, -1 if it 
 *         is located before the tree element or 1 if it is located
 *         after the tree element.
 */
int SnmpOidTreeCmp(CONST OID * objid, size_t objlen, CONST OID * treeid, size_t treelen)
{
    /* Compare elements up to the length of shortest name. */
    int rc = SnmpOidLenCmp(objid, treeid, (objlen < treelen) ? objlen : treelen);
    /* If equal, compare lengths. */
    if (rc == 0 && objlen < treelen) {
        rc = -1;
    }
    return rc;
}

/*!
 * \brief Compare object identifiers with index added.
 *
 * \param name1 First object identifier.
 * \param len1  Length of first object identifier.
 * \param name2 Second object identifier.
 * \param len2  Length of second object identifier.
 * \param index Index sub identifier.
 *
 * \return 0 if both are equal, 1 if first element is larger or -1
 *         if first element is lower than the second one.
 */
int SnmpOidCmpIdx(CONST OID * name1, size_t len1, CONST OID * name2, size_t len2, OID index)
{
    size_t len = (len1 < len2) ? len1 : len2;
    /* Compare elements up to the length of shortest name. */
    int rc = SnmpOidLenCmp(name1, name2, len);
    /* If equal, compare lengths. */
    if (rc == 0) {
        if (len1 < len2) {
            rc = -1;
        } else if (len1 > len2) {
            if (*(name1 + len) < index) {
                /* First is lower than second. */
                rc = -1;
            } else if (*(name1 + len) > index) {
                /* First is larger than second. */
                rc = 1;
            } else if (len1 > len2 + 1) {
                rc = 1;
            }
        }
    }
    return rc;
}

/*
 * This should be faster than doing a SnmpOidCmp for different 
 * length OIDs, since the length is checked first and if != returns
 * immediately.  
 *
 * Might be very slighly faster if lengths are ==.
 *
 * \param name1 A pointer to the first OID.
 * \param len1  Length of the first OID.
 * \param name2 A pointer to the second OID.
 * \param len2  Length of the second OID.
 *
 * \return 0 if they are equal, -1 if they are not.
 */
int SnmpOidEquals(CONST OID * name1, size_t len1, CONST OID * name2, size_t len2)
{
    if (len1 != len2 || memcmp(name1, name2, len1)) {
        return -1;
    }
    return 0;
}


void SnmpStatsInc(int which)
{
    if (which >= 0 && which < SNMP_STAT_MAX) {
        statistics[which]++;
    }
}

uint32_t SnmpStatsGet(int which)
{
    if (which >= 0 && which < SNMP_STAT_MAX) {
        return statistics[which];
    }
    return 0;
}

void SnmpStatsSet(int which, uint32_t value)
{
    if (which >= 0 && which < SNMP_STAT_MAX) {
        statistics[which] = value;
    }
}

/*@}*/
#endif // HAVE_NET
