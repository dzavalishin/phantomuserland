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


#include <testenv.h>

int do_test_misc(const char *test_parm);


int do_test_malloc(const char *test_parm);
int do_test_physmem(const char *test_parm);
int do_test_physalloc_gen(const char *test_parm);

// TODO test physmem alloc: allocator separately and core/locore/vaddr instances separately

int do_test_cbuf(const char *test_parm);

int do_test_udp_send(const char *test_parm);
int do_test_udp_syslog(const char *test_parm);

int do_test_resolver(const char *test_parm);

int do_test_tftp(const char *test_parm);

int do_test_tcp_connect(const char *test_parm);

int do_test_video(const char *test_parm);

// TODO test TCP? how?

// TODO test TRFS? how?

// TODO test new disk io
// TODO test paging io



// TODO test stopping videodriver, starting VGA driver, etc



// TODO test virtio drivers (and write 'em first)


// TODO test elf load

// TODO test time of day, sleep, spin sleeps, etc

// TODO test mutexes, conds, semas incl timeouts

int do_test_01_threads(const char *test_parm);

int do_test_threads(const char *test_parm);

int do_test_dpc(const char *test_parm);

int do_test_sem(const char *test_parm);

int do_test_many_threads(const char *test_parm);


int do_test_timed_call(const char *test_parm);





int do_test_amap(const char *test_parm);
int do_test_pool(const char *test_parm);
int do_test_hdir(const char *test_parm);

int do_test_ports(const char *test_parm);


int do_test_absname(const char *test_parm);
int do_test_userland(const char *test_parm);

int do_test_rectangles(const char *test_parm);

int do_test_crypt(const char *test_parm);


int do_test_wtty(const char *test_parm);

#endif // TEST_H

