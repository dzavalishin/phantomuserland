/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Atomic funcs.
 *
 *
**/

#ifndef ATOMIC_H
#define ATOMIC_H

#define ATOMIC_ADD_AND_FETCH( __ptr, __val ) __sync_add_and_fetch( __ptr, __val )

// atomic_set can be used for non-intel?
#define ATOMIC_FETCH_AND_SET( __ptr, __val ) __sync_lock_test_and_set( __ptr, __val )

int atomic_add(volatile int *val, int incr);
int atomic_or(volatile int *val, int incr);
#if 0
/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

int atomic_and(int *val, int incr);
int atomic_set(volatile int *val, int set_to);
int test_and_set(int *val, int set_to, int test_val);

#endif

#endif // ATOMIC_H

