/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Semaphores.
 *
**/

#ifndef SEM_H
#define SEM_H

#include <errno.h>

#if CONF_NEW_SEMA_FUNC
#  include <kernel/pool.h>
#endif

struct phantom_sem_impl;

struct hal_sem
{
#if CONF_NEW_SEMA_FUNC
    pool_handle_t h;
    struct phantom_sem_impl     *impl; // TODO killme - that's for kernel dumps to have fast access to sema name
#else
    struct phantom_sem_impl     *impl;
#endif
};

typedef struct hal_sem hal_sem_t;


int 					hal_sem_init( hal_sem_t *s, const char *name );

void 					hal_sem_release( hal_sem_t *s );
int 					hal_sem_acquire( hal_sem_t *s );

errno_t 				sem_get_count(hal_sem_t *s, int *count);

void 					hal_sem_destroy( hal_sem_t *s );

#define SEM_FLAG_NO_RESCHED             (1<<0)
#define SEM_FLAG_TIMEOUT                (1<<1)
#define SEM_FLAG_INTERRUPTABLE          (1<<2)
// Make sure acquire will block
#define SEM_FLAG_ZERO                   (1<<3)
// We ware destroying sema, do not attempt to lock any subsequent callers, return error
#define SEM_FLAG_INTERNAL_NOLOCK 	(1<<7)

int 					hal_sem_acquire_etc( hal_sem_t *s, int val, int flags, long uSec );

//* If positive - zeroes.
errno_t                                 hal_sem_zero( hal_sem_t *s );


#endif // SEM_H

