/*-
 * Copyright (c) 1992, 1993
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
 *
 *	@(#)libkern.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD: src/sys/sys/libkern.h,v 1.56.2.1.2.1 2008/11/25 02:59:29 kensmith Exp $
 */

#ifndef _SYS_LIBKERN_H_
#define	_SYS_LIBKERN_H_

#include <phantom_types.h>

static __inline int imax(int a, int b) { return (a > b ? a : b); }
static __inline int imin(int a, int b) { return (a < b ? a : b); }
static __inline long lmax(long a, long b) { return (a > b ? a : b); }
static __inline long lmin(long a, long b) { return (a < b ? a : b); }
static __inline u_int umax(u_int a, u_int b) { return (a > b ? a : b); }
static __inline u_int umin(u_int a, u_int b) { return (a < b ? a : b); }
static __inline quad_t qmax(quad_t a, quad_t b) { return (a > b ? a : b); }
static __inline quad_t qmin(quad_t a, quad_t b) { return (a < b ? a : b); }
static __inline u_long ulmax(u_long a, u_long b) { return (a > b ? a : b); }
static __inline u_long ulmin(u_long a, u_long b) { return (a < b ? a : b); }
static __inline off_t omax(off_t a, off_t b) { return (a > b ? a : b); }
static __inline off_t omin(off_t a, off_t b) { return (a < b ? a : b); }

static __inline int abs(int a) { return (a < 0 ? -a : a); }
static __inline long labs(long a) { return (a < 0 ? -a : a); }
static __inline quad_t qabs(quad_t a) { return (a < 0 ? -a : a); }

#ifndef	HAVE_INLINE_FFS
int	 ffs(int);
#endif
#ifndef	HAVE_INLINE_FFSL
int	 ffsl(long);
#endif
#ifndef	HAVE_INLINE_FLS
int	 fls(int);
#endif
#ifndef	HAVE_INLINE_FLSL
int	 flsl(long);
#endif

/*
 * Find First Reset bit
 */
int ffr(int mask);


#if 0



/* Prototypes for non-quad routines. */
uint32_t arc4random(void);
void	 arc4rand(void *ptr, u_int len, int reseed);
int	 bcmp(const void *, const void *, size_t);
void	*bsearch(const void *, const void *, size_t,
	    size_t, int (*)(const void *, const void *));


int	 fnmatch(const char *, const char *, int);
void	 gets(char *, size_t, int);
int	 locc(int, char *, u_int);
char	*index(const char *, int);
char	*rindex(const char *, int);
int	 scanc(u_int, const u_char *, const u_char *, int);
int	 skpc(int, int, char *);
int	 strvalid(const char *, size_t);

extern uint32_t crc32_tab[];

static __inline uint32_t
crc32_raw(const void *buf, size_t size, uint32_t crc)
{
	const uint8_t *p = (const uint8_t *)buf;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	return (crc);
}

static __inline uint32_t
crc32(const void *buf, size_t size)
{
	uint32_t crc;

	crc = crc32_raw(buf, size, ~0U);
	return (crc ^ ~0U);
}


/* fnmatch() return values. */
#define	FNM_NOMATCH	1	/* Match failed. */

/* fnmatch() flags. */
#define	FNM_NOESCAPE	0x01	/* Disable backslash escaping. */
#define	FNM_PATHNAME	0x02	/* Slash must be matched by slash. */
#define	FNM_PERIOD	0x04	/* Period must be matched by period. */
#define	FNM_LEADING_DIR	0x08	/* Ignore /<tail> after Imatch. */
#define	FNM_CASEFOLD	0x10	/* Case insensitive search. */
#define	FNM_IGNORECASE	FNM_CASEFOLD
#define	FNM_FILE_NAME	FNM_PATHNAME

#endif /* !_SYS_LIBKERN_H_ */


#endif /* !_SYS_LIBKERN_H_ */
