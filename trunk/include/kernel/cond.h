/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Conditional var. 
 * See http://code.google.com/p/phantomuserland/w/edit/KernelSync
 *
**/

#ifndef COND_H
#define COND_H

#include <errno.h>
#include <kernel/mutex.h>

struct hal_cond
{
    struct phantom_cond_impl *impl;
};

typedef struct hal_cond hal_cond_t;


errno_t		hal_cond_init( hal_cond_t *c, const char *name );
errno_t		hal_cond_wait( hal_cond_t *c, hal_mutex_t *m );
errno_t 	hal_cond_timedwait( hal_cond_t *c, hal_mutex_t *m, long msecTimeout );
errno_t		hal_cond_signal( hal_cond_t *c );
errno_t		hal_cond_broadcast( hal_cond_t *c );
errno_t		hal_cond_destroy( hal_cond_t *c );



#endif // COND_H

