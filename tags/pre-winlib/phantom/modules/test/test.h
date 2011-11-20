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

#include <testenv.h>


void run_test( const char *test_name, const char *test_parm );



int do_test_getters(const char *test_parm);
int do_test_setters(const char *test_parm);

int do_test_misc(const char *test_parm);

int do_test_pipe(const char *test_parm);

int do_test_ports(const char *test_parm);

int do_test_tcpfs(const char *test_parm);





#endif // TEST_H

