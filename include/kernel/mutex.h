/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Mutex.
 *
**/

#ifndef MUTEX_H
#define MUTEX_H

#include <errno.h>

struct hal_mutex
{
    struct phantom_mutex_impl *impl;
};

typedef struct hal_mutex hal_mutex_t;

errno_t                 hal_mutex_init(hal_mutex_t *m, const char *name);
errno_t                 hal_mutex_lock(hal_mutex_t *m);
errno_t                 hal_mutex_unlock(hal_mutex_t *m);
errno_t                 hal_mutex_destroy(hal_mutex_t *m);
//! NB! Races - mutex can get locked just after return.
// The only safe use is for ASSERT_LOCKED_MUTEX.
int                     hal_mutex_is_locked(hal_mutex_t *m);

#define ASSERT_LOCKED_MUTEX(m) assert(hal_mutex_is_locked(m))




#endif // MUTEX_H

