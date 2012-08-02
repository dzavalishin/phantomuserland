#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "utils.h"


#include <kernel/pool.h>



static pool_t *pool;

#define MAX 200

static pool_handle_t  hs[MAX];
static int hsp = 0;

static pool_handle_t  _pop()
{
    assert(hsp > 0);
    return hs[--hsp];
}

static void _push(pool_handle_t h)
{
    assert(hsp < MAX);
    hs[hsp++] = h;
}

static int _empty() { return hsp == 0; }




static void _show_free()
{
    printf( "%d free, %d used in pool\n", pool_get_free( pool ), pool_get_used( pool ) );
}


static void _create_free( int i )
{
    printf( "create/free %d\n", i );

    while(i-- > 0)
    {
        //printf("%d ", i);
        pool_handle_t  h = pool_create_el( pool, "aaa" );
        test_check_ge(h, 0);
        test_check_false(pool_release_el( pool, h ));
        //pool_destroy_el( pool, h );
        //if( (i%10) == 0 ) printf(".");
        //if( (i%100) == 0 ) printf("\n");
    }
    //printf("\n");
    //_show_free();
}

static void _create( int i )
{
    printf( "create %d\n", i );

    while(i-- > 0)
    {
        pool_handle_t  h = pool_create_el( pool, "aaa" );
        test_check_ge(h, 0);
        _push( h );
    }
    //_show_free();
}


static void _release( int i)
{
    printf( "release %d\n", i );

    while(i-- > 0)
        test_check_false(pool_release_el( pool, _pop() ));
    //_show_free();
}

static void _release_all()
{
    SHOW_INFO0( 0, "release all" );

    while(!_empty())
        test_check_false(pool_release_el( pool, _pop() ));
    //_show_free();
}

static int ff_count;
static void *ff_el;
static errno_t _ff(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;
    (void) arg;

    ff_el = el;
    ff_count++;
    return 0;
}

static errno_t _ff_fail(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;
    (void) el;
    (void) arg;

    return ENOMEM;
}


TEST_FUNCT(pool) {
    //printf("test case 1\n");

    pool = create_pool();
    pool->flag_nofail = 0; // return errors, don't panic

    _show_free();

    pool_handle_t  h;

    SHOW_FLOW0( 0, "check make/kill" );

    void *el0 = "aaa";
    h = pool_create_el( pool, el0 );
    test_check_false(h < 0);
    //_show_free();
    test_check_true( 1 == pool_get_used( pool ) );

    void *el = pool_get_el( pool, h );
    test_check_eq( el0, el );
    test_check_true( 2 == pool_get_used( pool ) );

    test_check_false(pool_release_el( pool, h ));
    test_check_true( 1 == pool_get_used( pool ) );

    ff_el = 0;
    ff_count = 0;
    test_check_false(pool_foreach( pool, _ff, 0 ));
    test_check_eq( el0, ff_el );
    test_check_eq( ff_count, 1 );

    test_check_eq(pool_foreach( pool, _ff_fail, 0 ), ENOMEM);

    test_check_false(pool_release_el( pool, h ));
    test_check_true( 0 == pool_get_used( pool ) );
    //_show_free();

    ff_el = 0;
    ff_count = 0;
    test_check_false(pool_foreach( pool, _ff, 0 ));
    test_check_eq( 0, ff_el );
    test_check_eq( ff_count, 0 );


    //SHOW_FLOW0( 0, "check pool_destroy_el fail" );
    test_check_false(!pool_destroy_el( pool, h )); // must fail

    test_check_true( 0 == pool_get_used( pool ) );

    SHOW_FLOW0( 0, "check mass make/kill" );
    _create_free( 500 );

    SHOW_FLOW0( 0, "check create + mass make/kill" );
    _create(20);
    _create_free( 500 );

    SHOW_FLOW0( 0, "check kill/make/kill" );
    _release(10);
    _create(20);
    _release(10);

    SHOW_FLOW0( 0, "check mass make/kill" );
    _create_free( 500 );

    SHOW_FLOW0( 0, "check release all" );
    _release_all();
    test_check_true( 0 == pool_get_used( pool ) );

    test_check_false(destroy_pool(pool));

    // Now test autodelete

    SHOW_FLOW0( 0, "check autodelete" );
    pool = create_pool();
    pool->flag_nofail = 0; // return errors, don't panic
    pool->flag_autoclean = 1;

    _create( 100 );

    test_check_false(destroy_pool(pool));

}

TEST_FUNCT(foo2) {
   printf("test case 2\n");
    /* Фейковый код */
    CU_ASSERT_EQUAL(1, 1);
}

void runSuite(void) {
    /* Код тест-сьюта */

    //printf("test suite\n");

    CU_pSuite suite = CUnitCreateSuite("Pool");
    if (suite) {
        ADD_SUITE_TEST(suite, pool)
        //ADD_SUITE_TEST(suite, foo2)
    }
}
