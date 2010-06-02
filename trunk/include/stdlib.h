#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <phantom_types.h>

void _exit(int code) __attribute__ ((noreturn));
void exit(int code) __attribute__ ((noreturn));

void qsort(void *base, size_t nmemb, size_t element_size,
        int (*cmp)(const void *, const void *));

#endif // __STDLIB_H__

