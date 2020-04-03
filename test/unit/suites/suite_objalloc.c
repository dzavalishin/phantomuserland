#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "utils.h"

#if 0 // attempts to bring in too much

static char pvm_object_space[1024*1024*32];

TEST_FUNCT(init_arenas) {
    printf("init alloc\n");

    //pvm_alloc_init( pvm_object_space, sizeof(pvm_object_space) );
	alloc_init_arenas( pvm_object_space, sizeof(pvm_object_space) );

    CU_ASSERT_EQUAL(0, 0);
}
TEST_FUNCT(foo2) {
    //printf("test case 2\n");
    /* Фейковый код */
    CU_ASSERT_EQUAL(1, 1);
}

void runSuite(void) {
    /* Код тест-сьюта */

    //printf("test suite\n");

    CU_pSuite suite = CUnitCreateSuite("ObjectAlloc");
    if (suite) {
        ADD_SUITE_TEST(suite, init_arenas)
        ADD_SUITE_TEST(suite, foo2)
    }
}

#endif
