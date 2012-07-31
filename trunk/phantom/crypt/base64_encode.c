/*
 * Copyright (C) 2009 by Thermotemp GmbH. All rights reserved.
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
 * \file gorp/base64/base64_encode.c
 * \brief Base64 encoder.
 *
 * \verbatim
 *
 * $Log$
 * Revision 1.3  2009/03/08 20:18:37  haraldkipp
 * Replaced inttypes.h by stdint.h.
 *
 * Revision 1.2  2009/03/06 23:51:37  olereinhardt
 * Fixed minor compile bugs
 *
 * Revision 1.1  2009/03/06 17:46:21  olereinhardt
 * Initial checkin, base64 encoding and decoding routines
 *
 *
 * \endverbatim
 */

#define PRG_RDB(x) (*(x))

//#include <stdint.h>
#include <string.h>

#include <malloc.h>
#include <sys/types.h>

#include <kernel/crypt/base64.h>

/*!
 * \addtogroup xgBase64
 */
/*@{*/

static char base64etab[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*!
 * \brief Do base-64 encoding on a string. 
 *
 * Return newly allocated string filled with the bytes generated. 
 * The encoded size will be at least 4/3 the size of the text, and 
 * may be smaller larger if there needs to be some padding characters 
 * (blanks, newlines).
 *
 * \param str Points to the string to be encoded.
 * \return Newly allocated string containing the encoded data. Must be freed later
 */

char *NutEncodeBase64(const char* str)
{
    char    *encoded;
    size_t  length;
    size_t  encoded_length;
    int     idx, enc_pos;
    int32_t bits;
    int     cols;
    int     char_count; 
    
    /* Calculate buffer size for encoded data 
     * 8 Bit code converted into 6 Bit code ==> encoded code will be 4/3 larger
     * The algorithm will padd the encoded data with == so that the resulting 
     * length can be exactly deviced by 3
     * Also every 72 chars a <cr><lf> will be added
     */
    
    length = strlen(str);
    
    /* Calc the base code length, add one to the inital length to have space for rounding errors */
    encoded_length = ((length + 1) * 4) / 3;
    /* Add the size for the padding characters */
    encoded_length += encoded_length % 3;
    /* Now add the space for the inserted <cr><lf> characters and add one byte for the end of string NUL character*/
    encoded_length += (encoded_length / 72) * 2 + 3;
    /* Allocate the memory. */;
    encoded = calloc(1,encoded_length);
    
    if (encoded == 0) return 0;
    
    enc_pos = 0;
    char_count = 0;
    bits = 0;
    cols = 0;
    
    for (idx = 0; idx < length; idx ++) {
        bits += (int32_t)str[idx];
        char_count ++;
        if (char_count == 3) {
            encoded[enc_pos++] = PRG_RDB(&base64etab[(bits >> 18) & 0x3f]);
            encoded[enc_pos++] = PRG_RDB(&base64etab[(bits >> 12) & 0x3f]);
            encoded[enc_pos++] = PRG_RDB(&base64etab[(bits >> 6) & 0x3f]);
            encoded[enc_pos++] = PRG_RDB(&base64etab[bits & 0x3f]);
            cols += 4;
            if (cols == 72) {
                encoded[enc_pos++] = '\r';
                encoded[enc_pos++] = '\n';
                cols =0;
            }
            bits = 0;
            char_count = 0;
        } else {
            bits <<= 8;
        }
    }
    
    if (char_count != 0) {
        bits <<= 16 - (8 * char_count);
        encoded[enc_pos++] = PRG_RDB(&base64etab[bits >> 18]);
        encoded[enc_pos++] = PRG_RDB(&base64etab[(bits >> 12) & 0x3f]);
        if (char_count == 1) {
            encoded[enc_pos++] = '=';
            encoded[enc_pos++] = '=';
        } else {
            encoded[enc_pos++] = PRG_RDB(&base64etab[(bits >> 6) & 0x3f]);
            encoded[enc_pos++] = '=';
        }
    }
    encoded[enc_pos] = '\0';
    return encoded;
}

/*@}*/
