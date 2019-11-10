/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Hosted semaphore impl
 *
 *
**/


#include "../../include/kernel/sem.h"




errno_t                 hal_sem_init_etc(hal_sem_t *c, const char *name, int value );

void 					hal_sem_release( hal_sem_t *s );


void 					hal_sem_destroy( hal_sem_t *s );


int 					hal_sem_acquire_etc( hal_sem_t *s, int val, int flags, long uSec );





int hal_sem_init( hal_sem_t *s, const char *name )
{
    return hal_sem_init_etc( s, name, 0 );
}


int hal_sem_acquire( hal_sem_t *s )
{
    return hal_sem_acquire_etc( s, 1, 0, 0 );
}


errno_t hal_sem_zero( hal_sem_t *s )
{
    if(s->impl == 0 )
        return EINVAL;

    if( s->impl->value > 0 )
        s->impl->value = 0;
	return 0;
}



errno_t sem_get_count(hal_sem_t *s, int *count)
{
    if(s->impl == 0 || count == 0)
        return EINVAL;

    *count = s->impl->value;
	return 0;
}
