#ifndef STDIO_H
#define STDIO_H

#include <phantom_libc.h>

#ifndef KERNEL

struct _FILE {};

typedef struct _FILE FILE;

FILE *fopen( const char *name, const char *mode );

#endif // KERNEL

#endif // STDIO_H
