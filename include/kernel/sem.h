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

struct phantom_sem_impl;

struct hal_sem
{
    struct phantom_sem_impl     *impl;
};

typedef struct hal_sem hal_sem_t;


int 					hal_sem_init( hal_sem_t *s, const char *name );

void 					hal_sem_release( hal_sem_t *s );
int 					hal_sem_acquire( hal_sem_t *s );

errno_t 				sem_get_count(hal_sem_t *s, int *count);

void 					hal_sem_destroy( hal_sem_t *s );

#define SEM_FLAG_NO_RESCHED 1
#define SEM_FLAG_TIMEOUT 2
#define SEM_FLAG_INTERRUPTABLE 4

int 					hal_sem_acquire_etc( hal_sem_t *s, int val, int flags, long uSec );

#endif // SEM_H

