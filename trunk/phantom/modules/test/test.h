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


int do_test_udp_send(const char *test_parm);
int do_test_udp_syslog(const char *test_parm);

// TODO test TCP? how?

// TODO test spawn/exec


// TODO test time of day, sleep, etc

// TODO test threads, mutexes, conds, semas incl timeouts

int do_test_threads(const char *test_parm);


int do_test_timer(const char *test_parm);


int do_test_ports(const char *test_parm);


#endif // TEST_H

