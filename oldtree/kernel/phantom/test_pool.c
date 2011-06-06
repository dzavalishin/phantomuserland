/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests - pool
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

#include <kernel/pool.h>


static pool_t *pool;

#define MAX 200


static void _create_free( int i )
{
    while(i-- > 0)
    {
        pool_handle_t  h = pool_create_el( pool, "aaa" );
        pool_release_el( pool, h );
        //pool_destroy_el( pool, h );
    }
}


static void _create( int i )
{
    while(i-- > 0)
        pool_create_el( pool, "aaa" );
}

int do_test_pool(const char *test_parm)
{
    (void) test_parm;

    pool = create_pool();

    _create_free( 500 );

    _create(20);

    _create_free( 500 );

    //_release(10);
    _create(20);
    //_release(10);

    _create_free( 500 );

    //destroy_pool(pool);
    return 0;
}

