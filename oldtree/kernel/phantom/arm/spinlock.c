#include <spinlock.h>
#include <hal.h>
#include <phantom_libc.h>
#include <kernel/smp.h>
#include <kernel/atomic.h>

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

    while(1)
    {
        while(sl->lock != 0)
        {
            //smp_process_pending_ici(curr_cpu);
        }
        if(atomic_set( &sl->lock, 1) == 0)
            break;
    }

#if SPIN_DEBUG
    global_lock_entry_count[GET_CPU_ID()]++;
    sl->ebp = get_ebp();
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
        stack_dump_ebp( get_ebp() );
    }
#endif
}
