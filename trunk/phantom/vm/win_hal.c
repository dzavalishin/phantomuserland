/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: no
 * Preliminary: yes
 *
 * This source file implements Windows based wrapper for VM to
 * run in Windows-hosted environment.
 *
**/

#include <stdarg.h>
#include <stdlib.h>


#include "gcc_replacements.h"

#include "hal.h"

struct hardware_abstraction_level    	hal;

int phantom_is_a_real_kernel() { return 0; }


void        hal_init( vmem_ptr_t va, long vs )
{
	hal_printf("Win32 HAL init\n");

	hal.object_vspace = va;
	hal.object_vsize = vs;

	pvm_alloc_init( va, vs );

	//hal.object_vsize = 40 * 1024 * 1024; // 40 MB

	//hal.object_vspace = (void *)PHANTOM_AMAP_START_VM_POOL;

	//hal_printf("HAL init VM at 0x%X\n", hal.object_vspace);


	//hal_init_vm_map();

}

vmem_ptr_t hal_object_space_address() { return hal.object_vspace; }



void    hal_halt()
{
	//fflush(stderr);
	hal_printf("\n\nhal halt called, exiting.\n");
	getchar();
	exit(1);
}


void        hal_sleep_msec( int miliseconds )
{
	//usleep(1000*miliseconds);
	sleep( ((miliseconds-1)/1000)+1 );
}





// -----------------------------------------------------------------------
// Spinlocks


#define	_spin_unlock(p) \
	({  register int _u__ ; \
	    __asm__ volatile("xorl %0, %0; \n\
			  xchgl %0, %1" \
			: "=&r" (_u__), "=m" (*(p)) ); \
	    0; })

// ret 1 on success
#define	_spin_try_lock(p)\
	(!({  register int _r__; \
	    __asm__ volatile("movl $1, %0; \n\
			  xchgl %0, %1" \
			: "=&r" (_r__), "=m" (*(p)) ); \
	    _r__; }))


void hal_spin_init(hal_spinlock_t *sl) { sl->lock = 0; }

void hal_spin_lock(hal_spinlock_t *sl)
{
    while( !  _spin_try_lock( &(sl->lock)  ) )
        while( sl->lock )
            ;
}

void hal_spin_unlock(hal_spinlock_t *sl)
{
    _spin_unlock(&(sl->lock));
}

// -----------------------------------------------------------------------

// alloc wants it
int phantom_virtual_machine_threads_stopped = 0;



// -----------------------------------------------------------------------
// TODO - implement those ones


int hal_mutex_init(hal_mutex_t *m, const char *name)
{
    return 0;
}

int hal_mutex_lock(hal_mutex_t *m)
{
    return 0;
}

int hal_mutex_unlock(hal_mutex_t *m)
{
    return 0;
}



int hal_cond_init( hal_cond_t *c, const char *name )
{
    return 0;
}







