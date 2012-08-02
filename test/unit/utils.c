#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <CUnit/Basic.h>

#include "utils.h"

void CUnitUInitialize(void)
{
    CU_cleanup_registry();
}

void CUnitInitialize(void)
{
    if (CU_initialize_registry() != CUE_SUCCESS) {
        //fprintf(stderr, "Failed to initialize the CUnit registry: %d\n", CU_get_error());
        printf( "Failed to initialize the CUnit registry: %d\n", CU_get_error() );
        exit(1);
    }
}

static int initSuite(void) {
    return 0;
}

static int cleanSuite(void) {
    return 0;
}

CU_pSuite CUnitCreateSuite(const char* title)
{
    CU_pSuite suite = NULL;
    suite = CU_add_suite(title, initSuite, cleanSuite);
    if (suite == NULL) {
        CU_cleanup_registry();
        return NULL;
    }

    return suite;
}


//void test_fail(int rc); // Call from any test to return to test runner and signal failure
//void test_fail_msg(int rc, const char *msg); // Call from any test to return to test runner and signal failure

int hal_mutex_init( void *a, const char *b ) { (void) a; (void) b; }
int hal_mutex_lock( void *a ) { (void) a; }
int hal_mutex_unlock( void *a ) { (void) a; }
int hal_mutex_is_locked( void *a ) { (void) a; return 1; }

int debug_max_level_error = 10;
