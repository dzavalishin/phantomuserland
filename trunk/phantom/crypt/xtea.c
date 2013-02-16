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
 * \file gorp/crypt/xtea.c
 * \brief Fast and effcient Extended Tiny Encryption Algorythm.
 *
 * \verbatim
 * $Id$
 * \endverbatim
 */

//#include <stdint.h>
#include <string.h>

#include <kernel/crypt/xtea.h>

#define XTDELTA  0x9e3779b9
#define XTSUM    0xC6EF3720
#define ROUNDS   32

/*!
 * \brief Encrypt a 64bit block with XTEA algorythm.
 *
 * \para w  Destination pointer to 2x32 bit encrypted block.
 * \para v  Source pointer of 2x32 bit plain text block.
 * \para k  Pointer to 128 bit (4*32bit) key block.
 */
void XTeaCrypt(u_int32_t *w,  const u_int32_t *v, const XTeaKeyBlock_t k)
{
    register u_int32_t y=v[0];
    register u_int32_t z=v[1];
    register u_int32_t sum=0;
    register u_int32_t delta=XTDELTA;
    u_int8_t n=ROUNDS;

    while (n-- > 0) {
        y += (((z << 4) ^ (z >> 5)) + z) ^ (sum + k[sum&3]);
        sum += delta;
        z += (((y << 4) ^ (y >> 5)) + y) ^ (sum + k[sum>>11 & 3]);
    }

    w[0]=y; w[1]=z;
}

/*!
 * \brief Decrypt a 64bit block with XTEA algorythm.
 *
 * \para w  Destination pointer to 2x32 bit decrypted block.
 * \para v  Source pointer of 2x32 bit encrypted block.
 * \para k  Pointer to 128 bit (4*32bit) key block.
 */
void XTeaDecrypt(u_int32_t *w, const u_int32_t *v, const XTeaKeyBlock_t k)
{
    register u_int32_t y=v[0];
    register u_int32_t z=v[1];
    register u_int32_t sum=XTSUM;
    register u_int32_t delta=XTDELTA;
    u_int8_t n=32;

    while (n-- > 0) {
        z -= (((y << 4) ^ (y >> 5)) + y) ^ (sum + k[sum>>11 & 3]);
        sum -= delta;
        y -= (((z << 4) ^ (z >> 5)) + z) ^ (sum + k[sum&3]);
    }

    w[0]=y; w[1]=z;
}

/*!
 * \brief Encrypt a string with XTEA algorythm.
 *
 * Respect that the strings or buffers of src and dst strings
 * have to have a length of multiple of 8 as it is a 64 bit based
 * algorhythm.
 *
 * \para dst  Target buffer to place the encrypted string into.
 * \para src  Source buffer to get the clear text string from.
 * \para len  Length of the string to encrypt.
 * \para pass Buffer of 64 bytes used as passphrase.
 */
void XTeaCryptStr( char *dst, const char *src, size_t len, const char *pass)
{
    size_t l;
    size_t i;
    XTeaKeyBlock_t K = { 0,0,0,0 };

    len &= ~0x7; // mask off 3 bits - must be multiple of 8, strip extra

    /* Prepare pass as XTEA Key Block */
    l = strlen(pass);
    if( l>sizeof( XTeaKeyBlock_t))
        l = sizeof( XTeaKeyBlock_t);
    memcpy( K, pass, l);

    i = 0;
    //l = strlen( src);

    while (i<len) {
        XTeaCrypt( (u_int32_t*)dst, (const u_int32_t*)src, K);
        src+=8; dst+=8; i+=8;
    }
}

/*!
 * \brief Decrypt a string with XTEA algorythm.
 *
 * Respect that the strings or buffers of src and dst strings
 * have to have a length of multiple of 8 as it is a 64 bit based
 * algorhythm.
 *
 * \para dst  Target buffer to place the decrypted string into.
 * \para src  Source buffer to get the encrypted string from.
 * \para len  Length of the string to decrypt.
 * \para pass Buffer of 64 bytes used as passphrase.
 */
void XTeaDecryptStr( char * dst, const char *src, size_t len, const char *pass)
{
    size_t l;
    size_t i;
    XTeaKeyBlock_t K = { 0,0,0,0 };

    len &= ~0x7; // mask off 3 bits - must be multiple of 8, strip extra

    /* Prepare pass as XTEA Key Block */
    l = strlen(pass);
    if( l>sizeof( XTeaKeyBlock_t))
        l = sizeof( XTeaKeyBlock_t);
    memcpy( K, pass, l);

    i = 0;
    //l = strlen( src );

    while (i<len) {
        XTeaDecrypt( (u_int32_t*)dst, (u_int32_t const*)src, K);
        src+=8; dst+=8; i+=8;
    }
}

