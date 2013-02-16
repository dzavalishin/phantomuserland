/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * String/mem ops.
 *
 *
**/

#ifndef __STRING_H__
#define __STRING_H__

#include "phantom_types.h"

int 	strerror_r(int errnum, char *strerrbuf, size_t buflen);

char * 	strtok_r(register char *s, register const char *delim, char **last);



size_t 	strlcat(char *dst, const char *src, size_t  siz);
size_t 	strlcpy(char *dst, const char *src, size_t siz);



int 	strcmp(const char *s1, const char *s2);
int 	strncmp(const char *s1, const char *s2, size_t n);

char *	strcat(char *s, const char *add);
char *	strcpy(char *to, const char *from);
//int 	strlen(const char *string);
size_t 	strlen(const char *string);

char *	strncpy(char *to, const char *from, ssize_t count);
// size_t is correct, but kernel dies!
//char *	strncpy(char *to, const char *from, size_t count);

char *	strchr(const char *p, int ch);
char *	index(const char *p, int ch);

char *	strrchr(const char *p, int ch);
char *	rindex(const char *p, int ch);

char *	strdup( const char *);
char *	strndup(const char *s, size_t n);

char * 	strupr(char *in);


void *	memcpy(void *dst0, const void *src0, size_t length);
void *	memmove(void *dst0, const void *src0, size_t length);
void 	bcopy(const void *src0, void *dst0, size_t length);

int 	memcmp(const void *s1v, const void *s2v, size_t size);
int 	bcmp(const void *s1, const void *s2, size_t n);


void 	bzero(void *dst0, size_t length);
void *	memset(void *dst0, int c0, size_t length);

long 	atol(const char *str);
long 	strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char * __restrict nptr, char ** __restrict endptr, int base);

double strtod(const char *__restrict str, char **__restrict endptr  );
long double strtold(const char *str, char **endptr );

quad_t 	strtoq(const char *nptr, char **endptr, int base);
u_quad_t strtouq(const char *nptr, char **endptr, int base);


void *	memchr(const void *buf, int c, size_t len);
void *	memscan(void *addr, int c, size_t size);
void *  memccpy(void *t, const void *f, int c, size_t n);


int 	strcoll(const char *s1, const char *s2);
char *	strncat(char *dest, const char *src, size_t count);
int 	strnicmp(const char *s1, const char *s2, size_t len);
int 	stricmp(const char *s1, const char *s2);
int 	strncasecmp(const char *s1, const char *s2, size_t len);
int 	strcasecmp(const char *s1, const char *s2);
size_t 	strnlen(const char *s, size_t count);
char *	strpbrk(const char *cs, const char *ct);
size_t 	strspn(const char *s, const char *accept);
char *	strstr(const char *s1, const char *s2);
size_t 	strxfrm(char *dest, const char *src, size_t n);

char *	strnstrn(const char *s1, int l1, const char *s2, int l2);


int     strncasecmp(const char *s1, const char *s2, size_t n);
int     strcasecmp(const char *s1, const char *s2);






#endif // __STRING_H__


