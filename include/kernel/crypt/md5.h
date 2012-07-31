#ifndef _MD5_H_
#define _MD5_H_

/*
 * Copyright (C) 2009 by Thermotemp GmbH. All rights reserved.
 *
 * This code is based on a public domain implementation of md5.c by 
 * Colin Plumb, June 1993
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
 * THIS SOFTWARE IS PROVIDED BY THERMOTEMP GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THERMOTEMP
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * For additional information see http://www.ethernut.de/
 */

/*!
 * \addtogroup xgHashes
 */
/*@{*/

//#include <stdint.h>
#include <sys/types.h>

/*!
 * \file gorp/hashes/md5.h
 * \brief MD5 hash implementation
 */

//__BEGIN_DECLS

/*!
 * \struct MD5CONTEXT md5.h
 * \brief MD5 context structure.
 */
typedef struct _MD5CONTEXT {
    u_int32_t buf[4];
    u_int32_t bits[2];
    u_int8_t  in[64];
} MD5CONTEXT;

/* Function prototypes. */

void MD5_Init(MD5CONTEXT *context);
void MD5_Update(MD5CONTEXT *context, u_int8_t const *buf, u_int32_t len);
void MD5_Final(MD5CONTEXT *context, u_int8_t digest[16]);

//__END_DECLS

/*@}*/

#endif
