#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "utils.h"


#include <kernel/amap.h>

#define MAP_FREE 0
#define MAP_USED 1
#define MAP_UNKNOWN 2

static amap_t map; 
static int counter = 0;

static void count( amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags, void *arg )
{
    (void) from;
    (void) flags;
    (void) arg;
    counter += n_elem;
}


TEST_FUNCT(amap_modify) {
    //printf("test case 1\n");

    int modified;

    amap_init( &map, 0, ~0, MAP_UNKNOWN );


    test_check_false(  amap_check_modify( &map, 10, 1, MAP_USED, &modified ) );
    test_check_false(  amap_check_modify( &map, 10, 1, MAP_USED, &modified ) );
    test_check_false( modified );

    test_check_false(  amap_check_modify( &map, 10, 1, MAP_FREE, &modified ) );
    test_check_true( modified );

    // Check if range has these attributes. Return 0 if it has.
    test_check_false( amap_check( &map, 10, 1, MAP_FREE ) );

    counter = 0;
    amap_iterate_flags( &map, count, 0, MAP_FREE );
    test_check_eq( counter, 1 );

    unsigned int i;

    for( i = 20; i < 50; i += 3 )
    {
        test_check_false( amap_check_modify( &map, i, 2, MAP_FREE, &modified ) );
    }

    for( i = 20; i < 50; i += 4 )
    {
        test_check_false( amap_check_modify( &map, i, 4, MAP_USED, &modified ) );
        test_check_true( modified );
    }

    for( i = 20; i < 100; i += 7 )
    {
        test_check_false( amap_check_modify( &map, i, 3, MAP_FREE, &modified ) );
    }

    amap_destroy( &map );


    /* Фейковый код */
    //CU_ASSERT_EQUAL(0, 0);
}


void runSuite(void) {

    CU_pSuite suite = CUnitCreateSuite("AMap");
    if (suite) {

        //amap_init( &map, 0, ~0, MAP_UNKNOWN );

        ADD_SUITE_TEST(suite, amap_modify)

        //amap_destroy( &map );


    }
}
