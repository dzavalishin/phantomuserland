/*-
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

//#include <sys/cdefs.h>
//__FBSDID("$FreeBSD: src/lib/libc/string/strerror.c,v 1.16.6.1 2008/11/25 02:59:29 kensmith Exp $");


#include <phantom_libc.h>

#include <limits.h>
#include <errno.h>

#define	UPREFIX		"Unknown error"

/*
 * Define a buffer size big enough to describe a 64-bit signed integer
 * converted to ASCII decimal (19 bytes), with an optional leading sign
 * (1 byte); finally, we get the prefix, delimiter (": ") and a trailing
 * NUL from UPREFIX.
 */
#define	EBUFSIZE	(20 + 2 + sizeof(UPREFIX))

/*
 * Doing this by hand instead of linking with stdio(3) avoids bloat for
 * statically linked binaries.
 */
static void
    errstr(int num, char *uprefix, char *buf, size_t len)
{
    char *t;
    unsigned int uerr;
    char tmp[EBUFSIZE];

    t = tmp + sizeof(tmp);
    *--t = '\0';
    uerr = (num >= 0) ? num : -num;
    do {
        *--t = "0123456789"[uerr % 10];
    } while (uerr /= 10);
    if (num < 0)
        *--t = '-';
    *--t = ' ';
    *--t = ':';
    strlcpy(buf, uprefix, len);
    strlcat(buf, t, len);
}

int
strerror_r(int errnum, char *strerrbuf, size_t buflen)
{
    int retval = 0;

    if (errnum < 1 || errnum >= sys_nerr) {
        errstr(errnum,
               UPREFIX,
               strerrbuf, buflen);
        retval = EINVAL;
    } else {
        if (strlcpy(strerrbuf,
                    sys_errlist[errnum],
                    buflen) >= buflen)
            retval = ERANGE;
    }

    return (retval);
}

char *
strerror(int num)
{
    printf("Warning: strerror(%d) used", num);
    static char ebuf[256];

    if (strerror_r(num, ebuf, sizeof(ebuf)) != 0)
        return "strerror failed";
    return ebuf;
}





