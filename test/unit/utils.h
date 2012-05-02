#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#define TEST_FUNCT(name) \
        static void test_##name() 

#define ADD_SUITE_TEST(suite, name) \
    if ((NULL == CU_add_test(suite, #name, (CU_TestFunc)test_##name))) {\
        CU_cleanup_registry();\
        return;\
    }\

CU_pSuite CUnitCreateSuite(const char* title);
void CUnitInitialize(void);
void CUnitUInitialize(void);

#endif 
