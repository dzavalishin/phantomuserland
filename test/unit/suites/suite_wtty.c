#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "utils.h"
#include "wtty.h"

#define BS 1024

TEST_FUNCT(foo) {
	char buf[BS];

    //printf("test case 1\n");
	wtty_t	w	= wtty_init();

	int rc;

	rc = wtty_read( w, buf, BS, 1);	// nowait
    CU_ASSERT_EQUAL(rc, 0);

	rc = wtty_putc( w, 'a' );
    CU_ASSERT_EQUAL(rc, 0);

	rc = wtty_read( w, buf, BS, 1);	// nowait
    CU_ASSERT_EQUAL(rc, 1);


	wtty_destroy( w );
}
TEST_FUNCT(foo2) {
    //printf("test case 2\n");
    /* Фейковый код */
    CU_ASSERT_EQUAL(1, 1);
}

void runSuite(void) {
    /* Код тест-сьюта */

    //printf("test suite\n");

    CU_pSuite suite = CUnitCreateSuite("Wtty");
    if (suite) {
        ADD_SUITE_TEST(suite, foo)
        ADD_SUITE_TEST(suite, foo2)
    }
}
