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



int strcmp(unsigned char *s1, unsigned char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

char *strcat(char *s, char *add);
char *strcpy(char *to, char *from);
int strlen(const char *string);
char *strncpy(char *to, char *from, int count);

char *strchr(const char *p, int ch);
char *index(const char *p, int ch);

char *strrchr(const char *p, int ch);
char *rindex(const char *p, int ch);

char *strdup( const char *);

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

#endif // __STRING_H__


#endif
