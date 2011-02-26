/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef ATOMIC_H
#define ATOMIC_H

int atomic_add(int *val, int incr);
int atomic_and(int *val, int incr);
int atomic_or(int *val, int incr);
int atomic_set(volatile int *val, int set_to);
int test_and_set(int *val, int set_to, int test_val);

#endif // ATOMIC_H

