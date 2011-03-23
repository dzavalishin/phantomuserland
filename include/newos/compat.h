/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * NewOS code compatibility defines.
 * Used to simplify adaptation of NewOS (and, possibly, Haiky) code.
 *
 *
**/


/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_H
#define _NEWOS_H

#include <phantom_types.h>
#include <kernel/page.h>


#define SYS_MAX_OS_NAME_LEN 32

// definitions to import newos code easily

//typedef unsigned long addr_t;
//typedef unsigned int ssize_t;
typedef int thread_id;



#ifndef offsetof
#define offsetof(type, member) ((size_t)&(((type *)0)->member))
#endif



#define false 0
#define true 0xFF

/*
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#else
#warning PAGE_SIZE defined twice
#endif
*/
#define PAGE_ALIGN(x) (((x) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1))


#define system_time hal_system_time




#define dprintf printf
#define kmalloc malloc
#define kfree free

#define ASSERT assert





#define sem_id hal_sem_t
#define sem_delete(s) hal_sem_destroy(&(s))
#define sem_release(s) hal_sem_release(&(s))
#define sem_acquire(s) hal_sem_acquire(&(s))

#define sem_acquire_etc(s, v, fl, tm, xx ) hal_sem_acquire_etc( &(s), v, fl, tm )

#define mutex_init(m,name) hal_mutex_init(m)
#define mutex_lock hal_mutex_lock
#define mutex_unlock hal_mutex_unlock
#define mutex hal_mutex_t
#define mutex_destroy hal_mutex_destroy

#define acquire_spinlock hal_spin_lock
#define release_spinlock hal_spin_unlock

#define int_disable_interrupts()  int __newos_intstate = hal_save_cli()
#define int_restore_interrupts()  if(__newos_intstate) hal_sti()


#define thread_create_kernel_thread(n,f,a) hal_start_kernel_thread_arg(f,a)
#define thread_set_priority(t,pri) hal_set_thread_priority( t, pri )

// TODO BUG! Hardcode!
#define THREAD_MAX_RT_PRIORITY 31

#define thread_kill_thread_nowait(tid) panic("thread_kill_thread_nowait at %s:%d",__FILE__,__LINE__)

#define thread_snooze(nsec) hal_sleep_msec((nsec)/1000)






#define uint32 u_int32_t
#define uint16 u_int16_t
#define uint8 u_int8_t




#define CHECK_BIT(a, b) ((a) & (1 << (b)))
#define SET_BIT(a, b) ((a) | (1 << (b)))
#define CLEAR_BIT(a, b) ((a) & (~(1 << (b))))

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))


#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))
#define ROUNDOWN(a, b) (((a) / (b)) * (b))









#ifndef __ASSEMBLY__

#if (__GNUC__ == 3) || (__GNUC__ == 4)
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define _PACKED __attribute__((packed))
#define _ALIGNED(x) __attribute__((aligned(x)))

#endif // #ifndef __ASSEMBLY__


#include <newos/err.h>



enum {
	INT_NO_RESCHEDULE,
	INT_RESCHEDULE
};


#endif // _NEWOS

