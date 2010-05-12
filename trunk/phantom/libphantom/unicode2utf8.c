#include <endian.h>
#include <errno.h>
#include <sys/types.h>

/*
 * Copyright 2007-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Tyler Dauwalder.
 *
 * This file may be used under the terms of the MIT License.
 */

/*! \brief Converts the given unicode character to utf8.
 */
void unicode_to_utf8_char(u_int32_t c, char **out)
{
    char *s = *out;

    if (c < 0x80)
        *(s++) = c;
    else if (c < 0x800) {
        *(s++) = 0xc0 | (c >> 6);
        *(s++) = 0x80 | (c & 0x3f);
    } else if (c < 0x10000) {
        *(s++) = 0xe0 | (c >> 12);
        *(s++) = 0x80 | ((c >> 6) & 0x3f);
        *(s++) = 0x80 | (c & 0x3f);
    } else if (c <= 0x10ffff) {
        *(s++) = 0xf0 | (c >> 18);
        *(s++) = 0x80 | ((c >> 12) & 0x3f);
        *(s++) = 0x80 | ((c >> 6) & 0x3f);
        *(s++) = 0x80 | (c & 0x3f);
    }

    *out = s;
}



// From EncodingComversions.cpp

// Pierre's (modified) Uber Macro

// NOTE: iso9660 seems to store the unicode text in big-endian form
#define u_to_utf8(str, uni_str)\
{\
	if ((B_BENDIAN_TO_HOST_INT16(uni_str[0])&0xff80) == 0)\
		*str++ = B_BENDIAN_TO_HOST_INT16(*uni_str++);\
	else if ((B_BENDIAN_TO_HOST_INT16(uni_str[0])&0xf800) == 0) {\
		str[0] = 0xc0|(B_BENDIAN_TO_HOST_INT16(uni_str[0])>>6);\
		str[1] = 0x80|(B_BENDIAN_TO_HOST_INT16(*uni_str++)&0x3f);\
		str += 2;\
	} else if ((B_BENDIAN_TO_HOST_INT16(uni_str[0])&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(B_BENDIAN_TO_HOST_INT16(uni_str[0])>>12);\
		str[1] = 0x80|((B_BENDIAN_TO_HOST_INT16(uni_str[0])>>6)&0x3f);\
		str[2] = 0x80|(B_BENDIAN_TO_HOST_INT16(*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((B_BENDIAN_TO_HOST_INT16(uni_str[0])-0xd7c0)<<10) | (B_BENDIAN_TO_HOST_INT16(uni_str[1])&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}


errno_t
unicode_to_utf8(const char  *src, int32_t *srcLen, char *dst, int32_t *dstLen)
{
    int32_t srcLimit = *srcLen;
    int32_t dstLimit = *dstLen;
    int32_t srcCount = 0;
    int32_t dstCount = 0;

    for (srcCount = 0; srcCount < srcLimit; srcCount += 2) {
        u_int16_t 	*UNICODE = (u_int16_t *)&src[srcCount];
        u_int8_t 	utf8[4];
        u_int8_t 	*UTF8 = utf8;
        int32_t 	utf8Len;
        int32_t 	j;

        u_to_utf8(UTF8, UNICODE);

        utf8Len = UTF8 - utf8;
        if (dstCount + utf8Len > dstLimit)
            break;

        for (j = 0; j < utf8Len; j++)
            dst[dstCount + j] = utf8[j];
        dstCount += utf8Len;
    }

    *srcLen = srcCount;
    *dstLen = dstCount;

    return dstCount > 0 ? 0 : ENOENT;
}

