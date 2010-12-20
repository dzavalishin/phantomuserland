/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests - memory
 *
 *
**/

#define DEBUG_MSG_PREFIX "test"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>
#include <phantom_libc.h>
#include <errno.h>
#include "test.h"



static void *memnotchar(void *addr, int c, size_t size)
{
    unsigned char *p = (unsigned char *)addr;

    while(size) {
        if(*p != c)
            return (void *)p;
        p++;
        size--;
    }
    return 0;
}


int do_test_malloc(const char *test_parm)
{
    (void) test_parm;

    const int max = 200;
    char *p[max];
    int sz[max];

    int i;

    for( i = 0; i < max; i++ )
    {
        sz[i] = 13*i;
        p[i] = malloc( sz[i] );
        if( p[i] == 0 )
            return ENOMEM;

        memset( p[i], i, sz[i] );
    }

    for( i = 0; i < max; i += 2 )
    {
        free( p[i] );
        if( memnotchar( p[i+1], i+1, sz[i+1]) )
            return EINVAL;
    }

    for( i = 1; i < max; i += 2 )
    {
        free( p[i] );
    }

    return 0;
}


