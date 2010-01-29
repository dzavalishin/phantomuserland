#if 0
#include "phantom_libc.h"
#else

#ifndef __STRING_H__
#define __STRING_H__

#include "phantom_types.h"

int strerror_r(int errnum, char *strerrbuf, size_t buflen);

char * strtok_r(register char *s, register const char *delim, char **last);



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

char *strdup( const char *);
char *strndup(const char *s, size_t n);


void *memcpy(void *dst0, const void *src0, size_t length);
void *memmove(void *dst0, const void *src0, size_t length);
void bcopy(const void *src0, void *dst0, size_t length);

int memcmp(const void *s1v, const void *s2v, size_t size);
int bcmp(const void *s1, const void *s2, size_t n);


void bzero(void *dst0, size_t length);
void *memset(void *dst0, int c0, size_t length);

long atol(const char *str);
long strtol(const char *nptr, char **endptr, int base); 
quad_t strtoq(const char *nptr, char **endptr, int base);
u_quad_t strtouq(const char *nptr, char **endptr, int base);


void *memchr(void const *buf, int c, size_t len);
void *memscan(void *addr, int c, size_t size);
int strcoll(const char *s1, const char *s2);
char *strncat(char *dest, char const *src, size_t count);
int strnicmp(char const *s1, char const *s2, size_t len);
int strncasecmp(char const *s1, char const *s2, size_t len);
size_t strnlen(char const *s, size_t count);
char *strpbrk(char const *cs, char const *ct);
size_t strspn(char const *s, char const *accept);
char *strstr(char const *s1, char const *s2);
size_t strxfrm(char *dest, const char *src, size_t n);









#endif // __STRING_H__


#endif
