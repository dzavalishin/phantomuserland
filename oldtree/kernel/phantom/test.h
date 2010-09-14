/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests header.
 *
 *
**/

#ifndef TEST_H
#define TEST_H

#include <errno.h>
#include <sys/cdefs.h>

void test_fail(errno_t rc); // Call from any test to return to test runner and signal failure
void test_fail_msg(errno_t rc, const char *msg); // Call from any test to return to test runner and signal failure

#define test_check_true(expr) if( !expr ) test_fail_msg( -1, #expr " is not true at " __XSTRING( __LINE__ ) );
#define test_check_false(expr) if( expr ) test_fail_msg( -1, #expr " is not false at " __XSTRING(  __LINE__ ) );

#define test_check_eq(expr, val) if( expr != val ) test_fail_msg( -1, #expr " != " #val " at " __XSTRING(  __LINE__ ) );
#define test_check_ne(expr, val) if( expr == val ) test_fail_msg( -1, #expr " == " #val " at " __XSTRING(  __LINE__ ) );
#define test_check_gt(expr, val) if( expr <= val ) test_fail_msg( -1, #expr " <= " #val " at " __XSTRING(  __LINE__ ) );
#define test_check_ge(expr, val) if( expr < val ) test_fail_msg( -1, #expr " < " #val " at " __XSTRING(  __LINE__ ) );



int do_test_malloc(const char *test_parm);

// TODO test amap
// TODO test physmem alloc: allocator separately and core/locore/vaddr instances separately

int do_test_udp_send(const char *test_parm);
int do_test_udp_syslog(const char *test_parm);

// TODO test TCP? how?

// TODO test TRFS? how?

// TODO test new disk io
// TODO test paging io



// TODO test stopping videodriver, starting VGA driver, etc



// TODO test virtio drivers (and write 'em first)


// TODO test elf load


// TODO test time of day, timed_call, sleep, spin sleeps, etc

// TODO test timed call incl phantom_undo_timed_call()

// TODO test threads, mutexes, conds, semas incl timeouts

int do_test_threads(const char *test_parm);

int do_test_dpc(const char *test_parm);


int do_test_timed_call(const char *test_parm);


int do_test_amap(const char *test_parm);

int do_test_ports(const char *test_parm);


#endif // TEST_H

