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

#if 1
#include <string.h>
#else
int strerror_r(int errnum, char *strerrbuf, size_t buflen);

size_t strlcat(char *dst, const char *src, size_t  siz);
size_t strlcpy(char *dst, const char *src, size_t siz);



int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

char *strcat(char *s, const char *add);
char *strcpy(char *to, const char *from);
int strlen(const char *string);
char *strncpy(char *to, const char *from, int count);

char *strchr(const char *p, int ch);
char *index(const char *p, int ch);

char *strrchr(const char *p, int ch);
char *rindex(const char *p, int ch);

void *memcpy(void *dst0, const void *src0, size_t length);
void *memmove(void *dst0, const void *src0, size_t length);
void bcopy(const void *src0, void *dst0, size_t length);


void bzero(void *dst0, size_t length);
void *memset(void *dst0, int c0, size_t length);

long atol(const char *str);
long strtol(const char *nptr, char **endptr, int base); 
quad_t strtoq(const char *nptr, char **endptr, int base);
u_quad_t strtouq(const char *nptr, char **endptr, int base);

#endif


#if 1
#include <stdlib.h>
#else
void _exit(int code);
void exit(int code);
#endif

int kvprintf(char const *fmt, void (*func)(int, void*), void *arg, int radix, va_list ap);
int vsnrprintf(char *str, size_t size, int radix, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int snprintf(char *str, size_t size, const char *format, ...);

//int vsprintf(char *buf, const char *cfmt, va_list ap);
//int sprintf(char *buf, const char *cfmt, ...);

int vprintf(const char *fmt, va_list ap);
int printf(const char *fmt, ...);

int sscanf(const char *ibuf, const char *fmt, ...);


// console.c
int getchar(void);
int putchar(int c);







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
extern u_char const	bcd2bin_data[];
extern u_char const	bin2bcd_data[];
extern char const	hex2ascii_data[];

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

