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
        fprintf(stderr, "Failed to initialize the CUnit registry: %d\n", CU_get_error());
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
