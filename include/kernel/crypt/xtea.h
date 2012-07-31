#ifndef _XTEA_H_
#define _XTEA_H_
/* 
 * Copyright of XTEA encryption algorithm by David Wheeler and Roger Needham 
 * at the Computer Laboratory of Cambridge University.
 * 
 * Kindly released to Public Domain.
 */

/*
 * Copyright of Implementation 2010 by Ulrich Prinz (uprinz2@netscape.net)
 *
 * All rights reserved.
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

/*
 * \file gorp/crypt/xtea.h
 * \brief Fast and effcient Extended Tiny Encryption Algorythm.
 *
 * \verbatim
 * $Id$
 * \endverbatim
 */

#include <sys/types.h>

typedef u_int32_t XTeaKeyBlock_t[4];

extern void XTeaCrypt(u_int32_t *w,  const u_int32_t *v, const XTeaKeyBlock_t k);
extern void XTeaDecrypt(u_int32_t *w, const u_int32_t *v, const XTeaKeyBlock_t k);

extern void XTeaCryptStr( char *dst, const char *src, size_t len, const char *pass);
extern void XTeaDecryptStr( char * dst, const char *src, size_t len, const char *pass);

#endif /* _XTEA_H_ */
