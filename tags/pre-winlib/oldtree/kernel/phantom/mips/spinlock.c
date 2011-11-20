/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS spinlocks
 *
**/

#define DEBUG_MSG_PREFIX "spinlock"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <spinlock.h>
#include <hal.h>
#include <phantom_libc.h>
#include <kernel/smp.h>
#include <kernel/atomic.h>

#if HAVE_SMP
#error no SMP on arm yet
#endif

#if SPIN_DEBUG
//#include <i386/proc_reg.h>
#endif

// -----------------------------------------------------------------------
// Spinlocks



#if SPIN_DEBUG
int global_lock_entry_count[MAX_CPUS] = {};
#endif


void hal_spin_init(hal_spinlock_t *sl)
{
    sl->lock = 0;
    sl->ebp = 0;
}

void hal_spin_lock(hal_spinlock_t *sl)
{
    if(hal_is_sti())
        printf("\n!spinlock STI!\n");

#if SPIN_DEBUG
    if(sl->lock)
    {
        printf("spinlock reenter detected, prev enter was here:\n");
        stack_dump_from((void *)sl->ebp);
        panic("reenter");
    }
#endif

#if HAVE_SMP
    while(1)
    {
        while(sl->lock != 0)
        {
            //smp_process_pending_ici(curr_cpu);
        }
        //if(atomic_set( &sl->lock, 1) == 0)
        if(__sync_lock_test_and_set( &sl->lock, 1) == 0)
            break;
    }
#else
    sl->lock = 1;
#endif

#if SPIN_DEBUG
    global_lock_entry_count[GET_CPU_ID()]++;
    sl->ebp = (addr_t)arch_get_frame_pointer();
#endif
}

void hal_spin_unlock(hal_spinlock_t *sl)
{
    if(!sl) // Scheduler sometimes calls us will sl == 0
        return;

    assert(sl->lock);
#if SPIN_DEBUG
    sl->ebp = 0;
    global_lock_entry_count[GET_CPU_ID()]--;
#endif

    sl->lock = 0;

    if(hal_is_sti())
        printf("\n!spinunlock STI!\n");
}


void check_global_lock_entry_count()
{
#if SPIN_DEBUG && 0
    if(global_lock_entry_count)
    {
        printf("some spinunlock locked!");
        stack_dump_ebp( arch_get_frame_pointer() );
    }
#endif
}
