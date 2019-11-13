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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

typedef int errno_t;

//#include "../../include/kernel/sem.h"
//#include "../../include/kernel/atomic.h"
#include <bits/errno_t.h>
#include <kernel/sem.h>
#include <kernel/atomic.h>


struct phantom_sem_impl
{
    //int spinlock;
    volatile int value;
};


errno_t hal_sem_init_etc(hal_sem_t *c, const char *name, int value )
{
	struct phantom_sem_impl *si = calloc( 1, sizeof(struct phantom_sem_impl) );
    if( 0 == si ) return ENOMEM;

	//si->spinlock = 0;
	si->value = value;

	c->impl = si;
	//printf("!!Sem Init %d ", si->value );
	return 0;
}

void hal_sem_release( hal_sem_t *s )
{
    struct phantom_sem_impl *si = s->impl;
    if(s->impl == 0 )
        return; // EINVAL; // TODO
    //printf("!!Sem SIGNAL!! ");
	ATOMIC_ADD_AND_FETCH( &si->value, 1 );
}


void 					hal_sem_destroy( hal_sem_t *s );


int hal_sem_acquire_etc( hal_sem_t *s, int val, int flags, long uSec )
{
	struct phantom_sem_impl *si = s->impl;
    if(s->impl == 0 || val <= 0)
        return EINVAL;

	if( flags ) printf("ERROR: sem flags != 0");

	while(1)
	{
		//printf("!!Sem %d want %d", si->value, val );
		int prev = ATOMIC_ADD_AND_FETCH( &si->value, -val );
		if( prev >= 0 ) 
		{
			//printf("!!Sem WAKE!! ");
			return 0;
		}
		//printf(" prev %d!! ", prev );

		ATOMIC_ADD_AND_FETCH( &si->value, val ); // put back

		usleep(100000); // actually spinlock - TODO make me better!
		//sleep(1); // tmp for debug
	}
}





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
