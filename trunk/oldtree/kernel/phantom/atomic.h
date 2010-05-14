/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_INCLUDE_LIBC_SYS_H
#define _NEWOS_INCLUDE_LIBC_SYS_H

int atomic_add(int *val, int incr);
int atomic_and(int *val, int incr);
int atomic_or(int *val, int incr);
int atomic_set(int *val, int set_to);
int test_and_set(int *val, int set_to, int test_val);

#endif

