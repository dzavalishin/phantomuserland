#ifndef PRO_SNMP_CONFIG_H
#define PRO_SNMP_CONFIG_H

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
 * \file kernel/snmp/snmp_config.h
 * \brief SNMP configuration.
 *
 * \verbatim
 * $Id: snmp_config.h 2737 2009-10-02 12:37:14Z haraldkipp $
 * \endverbatim
 */

#include <kernel/snmp/snmp.h>

/*!
 * \addtogroup xgSNMP
 */
/*@{*/

/*
 * View types.
 */
#define SNMP_VIEW_INCLUDED  1
#define SNMP_VIEW_EXCLUDED  2

typedef struct _viewEntry {
	struct _viewEntry *next;
        int  view_index;
        int  view_type;
        size_t view_subtree_len;
        OID  view_subtree[MAX_OID_LEN];
        char view_name[16];
} VIEW_LIST;

typedef struct _communityEntry {
	struct _communityEntry *next;
        int  comm_read_view;
        int  comm_write_view;
        char comm_name[16];
} COMMUNITY_LIST;

/*@}*/

extern int SnmpViewCreate(CONST char *, CONST OID *, size_t, int);
extern int SnmpViewFind(char *);

extern int SnmpCommunityCreate(CONST char *, int, int);
extern int SnmpCommunityFind(CONST char *, int *, int *);

#endif
