/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests - userland test suite counterpart
 *
 *
**/

#define DEBUG_MSG_PREFIX "uland_test"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>

#include <phantom_libc.h>
#include <sys/syslog.h>
#include <errno.h>

#include <newos/port.h>
#include <hal.h>
#include <threads.h>

#include "test.h"


static port_id uland_port;


int do_test_userland(const char *test_parm)
{
    (void) test_parm;

    char testdata[5];
    int res;
    int32_t code;


    strcpy(testdata, "abcd");

    SHOW_INFO0( 0, "port_create()");
    uland_port = port_create(1,    "__regress_test_port");

    test_check_ge(uland_port,0);


    SHOW_INFO0( 0, "port_write()");
    res = port_write(uland_port, 1, &testdata, sizeof(testdata));
    test_check_eq(res,0);

    testdata[0] = 0;

    SHOW_INFO0( 0, "port_read()");
    res = port_read_etc(uland_port, &code, &testdata, sizeof(testdata), PORT_FLAG_TIMEOUT, 1000000);
    test_check_eq(res,5);
    test_check_eq(code,0xAA);

    test_check_eq( strcmp(testdata, "abcd"), 0);


    SHOW_INFO0( 0, "port_read() - wait for userland to finish");
    res = port_read_etc(uland_port, &code, &testdata, sizeof(testdata), PORT_FLAG_TIMEOUT, 1000000);
    test_check_ge(res,0);
    test_check_eq(code,0x55);


    SHOW_INFO0( 0, "close port");
    res = port_close(uland_port);
    test_check_ne(res,0);

#if 0
    SHOW_INFO0( 0, "delete port");
    res = port_delete(test_p2);
    test_check_eq(res,0);
#endif
    SHOW_INFO0( 0, "end test");

    return 0;
}


