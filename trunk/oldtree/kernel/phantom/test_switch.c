/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Test suit test selector
 *
 *
**/

#define DEBUG_MSG_PREFIX "boot"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include "config.h"
#include <phantom_libc.h>
#include <errno.h>
#include "misc.h"
#include "test.h"

#define TEST(name) ({ if( all || (0 == strcmp( test_name, #name )) ) report( do_test_##name(test_parm), #name ); })

void report( int rc, const char *test_name )
{
    if( !rc )
    {
        printf("KERNEL TEST PASSED: %s\n", test_name );
        return;
    }

    printf("!!! KERNEL TEST FAILED: %s -> %d\n", test_name, rc );
    // todo strerror(rc)
}


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










void run_test( const char *test_name, const char *test_parm )
{
    int all = 0 == strcmp(test_name, "all" );

    TEST(malloc);
}
