/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests - BeOS ports
 *
 *
**/

#define DEBUG_MSG_PREFIX "test"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include "config.h"

#include <phantom_libc.h>
#include <sys/syslog.h>
#include <errno.h>

#include <newos/port.h>
#include <hal.h>

#include "test.h"


static port_id test_p1, test_p2, test_p3, test_p4;


static void port_test_thread_func(void* arg)
{
    (void) arg;

    int msg_code;
    int n;
    char buf[6];
    buf[5] = "----";

    static const char *expected = "abcd";

    SHOW_INFO0( 0, "porttest: thread running");

    n = port_read(test_p1, &msg_code, &buf, 3);
    SHOW_INFO( 0, "port_read #1 code %d len %d buf %s", msg_code, n, buf);
    test_check_eq(n,3);
    test_check_eq( strncmp(buf, expected, 3), 0 );

    n = port_read(test_p1, &msg_code, &buf, 4);
    SHOW_INFO( 0, "port_read #1 code %d len %d buf %s", msg_code, n, buf);
    test_check_eq(n,4);
    test_check_eq( strncmp(buf, expected, 4), 0 );

    buf[4] = 'X';
    n = port_read(test_p1, &msg_code, &buf, 5);
    SHOW_INFO( 0, "port_read #1 code %d len %d buf %s", msg_code, n, buf);
    test_check_eq(n,5);
    test_check_eq( strcmp(buf, expected), 0 );
#if 0
    SHOW_INFO0( 0, "porttest: testing delete p1 from other thread");
    n = port_delete(test_p1);
    SHOW_INFO0( 0, "porttest: end port_test_thread_func()");
    test_check_eq(n,0);
#endif
}



int do_test_ports(const char *test_parm)
{
    (void) test_parm;

    char testdata[5];
    //thread_id t;
    int res;
    int32_t dummy;
    int32_t dummy2;


    strcpy(testdata, "abcd");

    SHOW_INFO0( 0, "porttest: port_create()");
    test_p1 = port_create(1,    "test port #1");
    test_p2 = port_create(10,   "test port #2");
    test_p3 = port_create(1024, "test port #3");
    test_p4 = port_create(1024, "test port #4");

    test_check_ge(test_p1,0);
    test_check_ge(test_p2,0);
    test_check_ge(test_p3,0);
    test_check_ge(test_p4,0);


    SHOW_INFO0( 0, "porttest: port_find()");
    int found = port_find("test port #1");
    SHOW_INFO( 0, "'test port #1' has id %d (should be %d)", found, test_p1);
    test_check_eq(found, test_p1);

    SHOW_INFO0( 0, "porttest: port_write() on 1, 2 and 3");
    res = port_write(test_p1, 1, &testdata, sizeof(testdata));
    test_check_eq(res,0);
    res = port_write(test_p2, 666, &testdata, sizeof(testdata));
    test_check_eq(res,0);
    res = port_write(test_p3, 999, &testdata, sizeof(testdata));
    test_check_eq(res,0);
    SHOW_INFO( 0, "porttest: port_count(test_p1) = %d", port_count(test_p1));

    SHOW_INFO0( 0, "porttest: port_write() on 1 with timeout of 1 sec (blocks 1 sec)");
    res = port_write_etc(test_p1, 1, &testdata, sizeof(testdata), PORT_FLAG_TIMEOUT, 1000000);
    //test_check_ne(res,0); // TODO check for actual errno
    test_check_eq(res,-ETIMEDOUT);

    SHOW_INFO0( 0, "porttest: port_write() on 2 with timeout of 1 sec (wont block)");
    res = port_write_etc(test_p2, 777, &testdata, sizeof(testdata), PORT_FLAG_TIMEOUT, 1000000);
    SHOW_INFO( 0, "porttest: res=%d, %s", res, res == 0 ? "ok" : "BAD");
    test_check_eq(res,0);

    SHOW_INFO0( 0, "porttest: port_read() on empty port 4 with timeout of 1 sec (blocks 1 sec)");
    res = port_read_etc(test_p4, &dummy, &dummy2, sizeof(dummy2), PORT_FLAG_TIMEOUT, 1000000);
    SHOW_INFO( 0, "porttest: res=%d, %s", res, res == -ETIMEDOUT ? "ok" : "BAD");
    test_check_eq(res,-ETIMEDOUT);

    SHOW_INFO0( 0, "porttest: spawning thread for port 1");

    /*
    t = thread_create_kernel_thread("port_test", port_test_thread_func, NULL);
    // resume thread
    thread_resume_thread(t);
    */

    //hal_start_kernel_thread(port_test_thread_func);
    // TODO try usermode thread and test syscalls?
    res = hal_start_thread( port_test_thread_func, 0, 0);
    test_check_ge(res,0);

    SHOW_INFO0( 0, "porttest: write");
    res = port_write(test_p1, 1, &testdata, sizeof(testdata));
    test_check_eq(res,0);

    // now we can write more (no blocking)
    SHOW_INFO0( 0, "porttest: write #2");
    res = port_write(test_p1, 2, &testdata, sizeof(testdata));
    test_check_eq(res,0);
    SHOW_INFO0( 0, "porttest: write #3");
    res = port_write(test_p1, 3, &testdata, sizeof(testdata));
    test_check_eq(res,0);

    SHOW_INFO0( 0, "porttest: waiting on spawned thread");

    // TODO Fix
    //thread_wait_on_thread(t, NULL);
    hal_sleep_msec(1000);

    SHOW_INFO0( 0, "porttest: close p1");
    port_close(test_p2);
    SHOW_INFO0( 0, "porttest: attempt write p1 after close");
    res = port_write(test_p2, 4, &testdata, sizeof(testdata));
    SHOW_INFO( 0, "porttest: port_write ret %d", res);
    test_check_ne(res,0);

#if 0
    SHOW_INFO0( 0, "porttest: testing delete p2");
    res = port_delete(test_p2);
    test_check_eq(res,0);
#endif
    SHOW_INFO0( 0, "porttest: end test main thread");

    return 0;
}


