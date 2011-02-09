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

#include <hal.h>


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

#define MSIZE (4096*2)

int do_test_physmem(const char *test_parm)
{
    (void) test_parm;

    void *va;
    physaddr_t pa;

    char buf[MSIZE];

    hal_pv_alloc( &pa, &va, MSIZE );

    test_check_true( va != 0 );
    test_check_true( pa != 0 );

    memset( va, 0, MSIZE );
    memcpy_p2v( buf, pa, MSIZE );
    if( memnotchar( buf, 0, MSIZE ) )
        test_fail_msg( EINVAL, "not 0");


    memset( buf, 0xFF, MSIZE );
    memcpy_v2p( pa, buf, MSIZE );
    if( memnotchar( va, 0xFF, MSIZE ) )
        test_fail_msg( EINVAL, "not 1");

    memset( va, 0, MSIZE );

    memcpy_v2p( pa, "AAA", 3 );
    if( memnotchar( va, 'A', 3 ) )
        test_fail_msg( EINVAL, "not A");

    if( memnotchar( va+3, 0, MSIZE-3 ) )
        test_fail_msg( EINVAL, "not A0");


    memset( va, 0, MSIZE );

    memcpy_v2p( pa+10, "BBB", 3 );
    if( memnotchar( va+10, 'B', 3 ) )
        test_fail_msg( EINVAL, "not B");

    if( memnotchar( va, 0, 10 ) )
        test_fail_msg( EINVAL, "not B0-");

    if( memnotchar( va+13, 0, MSIZE-13 ) )
        test_fail_msg( EINVAL, "not B0+");


    // Cross page
    memset( va, 0, MSIZE );
#define SH (4096-4)

    memcpy_v2p( pa+SH, "EEEEEEEE", 8 );
    if( memnotchar( va+SH, 'E', 8 ) )
        test_fail_msg( EINVAL, "not E");

    if( memnotchar( va, 0, SH ) )
        test_fail_msg( EINVAL, "not E0-");

    if( memnotchar( va+SH+8, 0, MSIZE-SH-8 ) )
        test_fail_msg( EINVAL, "not E0+");



#if 0 // not impl
    memset( va, 0, MSIZE );

    memset( va+20, 'C', 3 );
    memcpy_p2v( buf, pa+20, 3 );
    if( memnotchar( buf, 'C', 3 ) )
        test_fail_msg( EINVAL, "not C");
#endif

    hal_pv_free( pa, va, MSIZE );

    return 0;
}

