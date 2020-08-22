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

#include <sys/types.h>

#include <stdlib.h>
#include <string.h>
//#include <memdebug.h>

#include <compat/nutos.h>

#include <kernel/snmp/snmp.h>
#include <kernel/snmp/snmp_api.h>
#include <kernel/snmp/snmp_mib.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

/*
 * The subtree structure contains a subtree prefix which applies to
 * all variables in the associated variable list.
 *
 * No subtree may be a subtree of another subtree in this list.
 * i.e.:   1.2
 *         1.2.0
 */
typedef struct _SUBTREE {
    /*! \brief Pointer to the next branch. */
    struct _SUBTREE *sub_next;
    /*! \brief Number of variables. */
    int sub_numvars;
    /*! \brief Pointer to the array of variables. */
    SNMPVAR *sub_vars;
    /*! \brief Length of the base name. */
    size_t sub_namelen;
    /*! \brief Base name of this branch. */
    OID sub_name[MAX_OID_LEN];
} SUBTREE;

static SUBTREE *mibtree;

/*!
 * \brief Register MIB variables.
 */
int SnmpMibRegister(OID basename[], size_t baselen, SNMPVAR * vars, int num)
{
    SUBTREE **tpp;
    SUBTREE *branch;

    /* Create a new branch. */
    if ((branch = malloc(sizeof(SUBTREE))) == NULL) {
        return -1;
    }
    branch->sub_numvars = num;
    branch->sub_vars = vars;
    branch->sub_namelen = baselen;
    memcpy(branch->sub_name, basename, baselen * sizeof(OID));

    /* Locate the new branch's insertion point. */
    for (tpp = &mibtree; *tpp; tpp = &(*tpp)->sub_next) {
        if (SnmpOidCmp((*tpp)->sub_name, (*tpp)->sub_namelen, branch->sub_name, branch->sub_namelen) > 0) {
            break;
        }
    }
    /* Insert the branch. */
    branch->sub_next = *tpp;
    *tpp = branch;

    return 0;
}

/*!
 * \brief Find MIB variable.
 *
 * \param name
 * \param namelen
 * \param type
 * \param len
 * \param acl
 * \param exact
 * \param wmethod
 * \param no_obj
 */
uint8_t *SnmpMibFind(OID * name, size_t * namelen, uint8_t * type, size_t * len, uint16_t * acl, int exact, WMETHOD ** wmethod,
                    int *no_obj)
{
    SUBTREE *tp;
    SNMPVAR *vp = 0;
    int i;
    uint8_t *access = NULL;
    int rc;
    OID *suffix;
    size_t sufflen;
    OID *ori_oid = NULL;
    size_t ori_len = 0;
    int found = 0;

    /* 
     * If not looking for an exact match, keep a copy of the original name. 
     */
    if (!exact) {
        if ((ori_oid = malloc(*namelen * sizeof(OID))) == NULL) {
            return NULL;
        }
        memcpy(ori_oid, name, *namelen * sizeof(OID));
        ori_len = *namelen;
    }
    *wmethod = NULL;

    /*
     * Walk along the linked list of subtrees.
     */
    for (tp = mibtree; tp; tp = tp->sub_next) {
        /* 
         * Check if name is part of this subtree. Or, if we don't need an exact match,
         * if the name is in front of the subtree.
         */
        rc = SnmpOidTreeCmp(name, *namelen, tp->sub_name, tp->sub_namelen);
        if (rc == 0 || (rc < 0 && !exact)) {
            sufflen = *namelen - tp->sub_namelen;
            suffix = name + tp->sub_namelen;

            for (i = 0, vp = tp->sub_vars; i < tp->sub_numvars; i++, vp++) {
                if (vp->var_namelen && (exact || rc >= 0)) {
                    rc = SnmpOidTreeCmp(suffix, sufflen, vp->var_name, vp->var_namelen);
                }

                if ((exact && rc == 0) || (!exact && rc <= 0) || vp->var_namelen == 0) {
                    access = (*(vp->var_get)) (vp, name, namelen, exact, len, wmethod);
                    if (wmethod) {
                        *acl = vp->var_acl;
                    }
                    if (exact) {
                        found = 1;
                    }
                    if (access) {
                        break;
                    }
                }
                if (exact && rc <= 0) {
                    *type = vp->var_type;
                    *acl = vp->var_acl;
                    *no_obj = !found;
                    free(ori_oid);
                    return NULL;
                }
            }
            if (access) {
                break;
            }
        }
    }
    if (tp == NULL) {
        if (!access && !exact) {
            memcpy(name, ori_oid, ori_len * sizeof(OID));
            *namelen = ori_len;
            free(ori_oid);
        }
        *no_obj = !found;
        return NULL;
    }
    if (ori_oid) {
        free(ori_oid);
    }

    /*
     * vp now points to the approprate struct.
     */
    *type = vp->var_type;
    *acl = vp->var_acl;

    return access;
}

/*@}*/
#endif // HAVE_NET
