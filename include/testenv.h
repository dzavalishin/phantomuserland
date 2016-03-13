/**
 *
 * Phantom OS - Phantom kernel include file.
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Test suit support header.
 * NB! This is used in both kernel and user mode tests.
 *
 *
**/

#ifndef TESTENV_H
#define TESTENV_H

#include <errno.h>
#include <sys/cdefs.h>

/* coverity[+kill] */
void test_fail(errno_t rc) __dead2; // Call from any test to return to test runner and signal failure

/* coverity[+kill] */
void test_fail_msg(errno_t rc, const char *msg) __dead2; // Call from any test to return to test runner and signal failure

void on_fail_call( void (*f)( void *arg), void *arg ); // Call f on failure


#define test_check_true(expr) if( !(expr) ) test_fail_msg( -1, #expr " is not true at " __XSTRING( __LINE__ ) );
#define test_check_false(expr) if( expr ) test_fail_msg( -1, #expr " is not false at " __XSTRING(  __LINE__ ) );

#define test_check_eq(expr, val) if( (expr) != (val) ) test_fail_msg( -1, #expr " != " #val " at " __XSTRING(  __LINE__ ) );
#define test_check_ne(expr, val) if( (expr) == (val) ) test_fail_msg( -1, #expr " == " #val " at " __XSTRING(  __LINE__ ) );
#define test_check_gt(expr, val) if( (expr) <= (val) ) test_fail_msg( -1, #expr " <= " #val " at " __XSTRING(  __LINE__ ) );
#define test_check_ge(expr, val) if( (expr) <  (val) ) test_fail_msg( -1, #expr " < " #val " at " __XSTRING(  __LINE__ ) );



#endif // TESTENV_H
