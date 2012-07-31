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


TEST_FUNCT(foo) {
    printf("test case 1\n");
     /* Фейковый код */
    CU_ASSERT_EQUAL(0, 0);
}


void runSuite(void) {

    CU_pSuite suite = CUnitCreateSuite("AMap");
    if (suite) {

        amap_init( &map, 0, ~0, MAP_UNKNOWN );


        ADD_SUITE_TEST(suite, foo)

        amap_destroy( &map );


    }
}
