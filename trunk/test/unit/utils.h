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


//#include <sys/cdefs.h>

//void test_fail(int rc); // Call from any test to return to test runner and signal failure
//void test_fail_msg(int rc, const char *msg); // Call from any test to return to test runner and signal failure

//#define test_check_true(expr) if( !expr ) test_fail_msg( -1, #expr " is not true at " __XSTRING( __LINE__ ) );
//#define test_check_false(expr) if( expr ) test_fail_msg( -1, #expr " is not false at " __XSTRING(  __LINE__ ) );

//#define test_check_eq(expr, val) if( expr != val ) test_fail_msg( -1, #expr " != " #val " at " __XSTRING(  __LINE__ ) );
//#define test_check_ne(expr, val) if( expr == val ) test_fail_msg( -1, #expr " == " #val " at " __XSTRING(  __LINE__ ) );
//#define test_check_gt(expr, val) if( expr <= val ) test_fail_msg( -1, #expr " <= " #val " at " __XSTRING(  __LINE__ ) );
//#define test_check_ge(expr, val) if( expr < val ) test_fail_msg( -1, #expr " < " #val " at " __XSTRING(  __LINE__ ) );

#define test_check_true(expr)    CU_ASSERT_TRUE(expr)
#define test_check_false(expr)   CU_ASSERT_FALSE(expr)

#define test_check_eq(expr, val) CU_ASSERT_EQUAL(expr, val)
#define test_check_ne(expr, val) CU_ASSERT_NOT_EQUAL(expr, val)


#define test_check_gt(expr, val) if( expr <= val ) CU_FAIL( #expr " <= " #val );
#define test_check_ge(expr, val) if( expr < val )  CU_FAIL( #expr " < " #val );

#define SHOW_FLOW0( lev, msg ) printf( msg "\n" )
#define SHOW_INFO0( lev, msg ) printf( msg "\n" )

#endif
