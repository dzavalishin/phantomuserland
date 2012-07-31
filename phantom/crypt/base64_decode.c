/*
 * Copyright (C) 2009 by Thermotemp GmbH. All rights reserved. 
 *
 * These routines where mainly taken from pro/dencode.c
 * Copyright (C) 2001-2003 by egnite Software GmbH. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY EGNITE SOFTWARE GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL EGNITE
 * SOFTWARE GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
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

#include <sys/types.h> 
#include <kernel/crypt/base64.h>

/*!
 * \addtogroup xgBase64
 */
/*@{*/

/* Base-64 decoding.  This represents binary data as printable ASCII
** characters.  Three 8-bit binary bytes are turned into four 6-bit
** values, like so:
**
**   [11111111]  [22222222]  [33333333]
**
**   [111111] [112222] [222233] [333333]
**
** Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
*/

/* Since base-64 encodes strings do not have any character above 127,
 * we need just the first 128 bytes. Furthermore there is no char
 * below 32, so we can save 32 additional bytes of flash.
 */
static char base64dtab[96] = {
/*
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
*/
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
/*
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 */
};

/*!
 * \brief Do base-64 decoding on a string. 
 *
 * Ignore any non-base64 bytes.
 * Return the actual number of bytes generated. The decoded size will
 * be at most 3/4 the size of the encoded, and may be smaller if there
 * are padding characters (blanks, newlines).
 *
 * \param str Points to the base64 encoded string to be decoded
 * \return Return the actual number of bytes generated.
 */

/*
 * Do base-64 decoding on a string. 
 */
char *NutDecodeBase64(char * str)
{
    /* bug fix from Damian Slee. */
    char code;
    char *sp;
    char *tp;
    char last = -1;
    char step = 0;

    for (tp = sp = str; *sp; ++sp) {
    	if (*sp < 32)
    	    continue;
        if ((code = PRG_RDB(&base64dtab[(int) *sp - 32])) == (char)-1)
            continue;
        switch (step++) {
        case 1:
            *tp++ = ((last << 2) | ((code & 0x30) >> 4));
            break;
        case 2:
            *tp++ = (((last & 0xf) << 4) | ((code & 0x3c) >> 2));
            break;
        case 3:
            *tp++ = (((last & 0x03) << 6) | code);
            step = 0;
            break;
        }
        last = code;
    }
    *tp = 0;
    return str;
}

/*@}*/
