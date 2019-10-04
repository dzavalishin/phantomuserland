/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * C stndard lib. TODO extend.
 *
 *
**/

#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <phantom_types.h>

void _exit(int code) __attribute__ ((noreturn));
void exit(int code) __attribute__ ((noreturn));

void qsort(void *base, size_t nmemb, size_t element_size,
        int (*cmp)(const void *, const void *));

long strtol(const char *nptr, char **endptr, int base);
unsigned long
strtoul(const char * __restrict nptr, char ** __restrict endptr, int base);

int atoi(const char *str);
long atol(const char *str);

int  atoin(const char *str, size_t n); // for phantom strings, n is string len
long atoln(const char *str, size_t n); // for phantom strings, n is string len


#endif // __STDLIB_H__

