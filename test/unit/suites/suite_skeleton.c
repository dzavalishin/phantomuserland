#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "utils.h"

TEST_FUNCT(foo) {
    printf("test case 1 - must fail\n");
     /* Фейковый код */
    CU_ASSERT_EQUAL(0, 1);
}
TEST_FUNCT(foo2) {
   printf("test case 2\n");
    /* Фейковый код */
    CU_ASSERT_EQUAL(1, 1);
}

void runSuite(void) {
    /* Код тест-сьюта */

    printf("test suite\n");

    CU_pSuite suite = CUnitCreateSuite("Suite1");
    if (suite) {
        ADD_SUITE_TEST(suite, foo)
        ADD_SUITE_TEST(suite, foo2)
    }
}
