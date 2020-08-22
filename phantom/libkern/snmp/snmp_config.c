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

#include <kernel/snmp/snmp_config.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

static VIEW_LIST *views;
static COMMUNITY_LIST *communities;

/*!
 * \param name Symbolic name of this view.
 * \param type Either SNMP_VIEW_INCLUDED or SNMP_VIEW_EXCLUDED.
 *
 * \return View index on success. Otherwise -1 is returned.
 */
int SnmpViewCreate(CONST char *name, CONST OID * subtree, size_t subtreelen, int type)
{
    static int nextview = 1;
    VIEW_LIST *vp;
    VIEW_LIST *nvp;
    VIEW_LIST *prev = NULL;
    size_t i;

    /* Check name length. */
    if (strlen(name) > (sizeof(vp->view_name) - 1)) {
        return -1;
    }

    /* Check if this name exists already. */
    for (vp = views; vp; prev = vp, vp = vp->next) {
        if (strcmp(name, vp->view_name) == 0) {
            break;
        }
    }

    /* Allocate a new view entry. */
    nvp = malloc(sizeof(VIEW_LIST));
    memset(nvp, 0, sizeof(VIEW_LIST));
    strcpy(nvp->view_name, name);
    nvp->view_type = type;
    nvp->view_subtree_len = subtreelen;
    for (i = 0; i < subtreelen; i++) {
        nvp->view_subtree[i] = subtree[i];
    }
    /* Set index, either of the existing entry or a new one. */
    nvp->view_index = vp ? vp->view_index : nextview++;

    /* Add the new entry to the linked list. */
    if (views) {
        for (; vp; prev = vp, vp = vp->next);
        prev->next = nvp;
    } else {
        views = nvp;
    }
    return nvp->view_index;
}

int SnmpViewFind(char *name)
{
    VIEW_LIST *vp;

    if (strcmp(name, "-") == 0) {
        return 0;
    }
    for (vp = views; vp; vp = vp->next) {
        if (strcmp(vp->view_name, name) == 0) {
            return vp->view_index;
        }
    }
    return -1;
}

/*!
 * \brief Find community entry by name.
 *
 * \param name      Community name.
 * \param readView  Pointer to a variable that receives the view index 
 *                  for read access. 
 * \param writeView Pointer to a variable that receives the view index 
 *                  for write access. 
 *
 * \return 0 on success, -1 otherwise.
 */
int SnmpCommunityFind(CONST char *name, int *readView, int *writeView)
{
    COMMUNITY_LIST *cp;

    for (cp = communities; cp; cp = cp->next) {
        if (strcmp(cp->comm_name, name) == 0) {
            if (readView) {
                *readView = cp->comm_read_view;
            }
            if (writeView) {
                *writeView = cp->comm_write_view;
            }
            return 0;
        }
    }
    return -1;
}

/*!
 * \brief Create a community entry.
 *
 * \param name      Community name.
 * \param readView  View index for read access, obtained from a previous 
 *                  call to SnmpViewCreate().
 * \param writeView View index for write access, obtained from a previous 
 *                  call to SnmpViewCreate().
 *
 * \return 0 on success, -1 otherwise.
 */
int SnmpCommunityCreate(CONST char *name, int readView, int writeView)
{
    COMMUNITY_LIST *cp;
    COMMUNITY_LIST *prev = 0;

    if (strlen(name) > (sizeof(cp->comm_name) - 1)) {
        return -1;
    }
    for (cp = communities; cp; cp = cp->next) {
        if (strcmp(name, cp->comm_name) == 0) {
            return 0;
        }
        prev = cp;
    }

    cp = malloc(sizeof(COMMUNITY_LIST));
    memset(cp, 0, sizeof(COMMUNITY_LIST));
    strcpy(cp->comm_name, name);
    cp->comm_read_view = readView;
    cp->comm_write_view = writeView;
    if (prev) {
        prev->next = cp;
    } else {
        communities = cp;
    }
    return 0;
}

/*@}*/
#endif // HAVE_NET
