/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel C library. Derived from Mach/FreeBSD kernels.
 *
 * Kernel ready: yes
 * Preliminary: yes
 *
 *
**/

#ifndef PHANTOM_LIBC_H
#define PHANTOM_LIBC_H

#include <phantom_types.h>
#include <stdarg.h>
#include <malloc.h>

#ifndef NULL
#define NULL 0
#endif

extern const int sys_nerr;
extern const char *const sys_errlist[];

#include <string.h>
#include <stdlib.h>



int kvprintf(const char *fmt, void (*func)(int, void*), void *arg, int radix, va_list ap);
int vsnrprintf(char *str, size_t size, int radix, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int snprintf(char *str, size_t size, const char *format, ...);

//int vsprintf(char *buf, const char *cfmt, va_list ap);
//int sprintf(char *buf, const char *cfmt, ...);

int vprintf(const char *fmt, va_list ap);
int printf(const char *fmt, ...);

int sscanf(const char *ibuf, const char *fmt, ...);


void    perror(const char *);


// console.c
int getchar(void);
int putchar(int c);
int puts(const char *s);



u_int32_t random(void);
void srandom(u_int32_t seed);




#define isspace(c)	((c) == ' ' || ((c) >= '\t' && (c) <= '\r'))
#define isascii(c)	(((c) & ~0x7f) == 0)
#define isupper(c)	((c) >= 'A' && (c) <= 'Z')
#define islower(c)	((c) >= 'a' && (c) <= 'z')
#define isalpha(c)	(isupper(c) || islower(c))
#define isdigit(c)	((c) >= '0' && (c) <= '9')
#define isxdigit(c)	(isdigit(c) \
			  || ((c) >= 'A' && (c) <= 'F') \
			  || ((c) >= 'a' && (c) <= 'f'))
#define isprint(c)	((c) >= ' ' && (c) <= '~')

#define toupper(c)	((c) - 0x20 * (((c) >= 'a') && ((c) <= 'z')))
#define tolower(c)	((c) + 0x20 * (((c) >= 'A') && ((c) <= 'Z')))



extern char **environ;
char *getenv(const char *name);




/* BCD conversions. */
extern const u_char	bcd2bin_data[];
extern const u_char	bin2bcd_data[];
extern const char	hex2ascii_data[];

#define	bcd2bin(bcd)	(bcd2bin_data[bcd])
#define	bin2bcd(bin)	(bin2bcd_data[bin])
#define	hex2ascii(hex)	(hex2ascii_data[hex])




#define	HD_COLUMN_MASK	0xff
#define	HD_DELIM_MASK	0xff00
#define	HD_OMIT_COUNT	(1 << 16)
#define	HD_OMIT_HEX	(1 << 17)
#define	HD_OMIT_CHARS	(1 << 18)

void hexdump(const void *ptr, int length, const char *hdr, int flags);









#endif // PHANTOM_LIBC_H

