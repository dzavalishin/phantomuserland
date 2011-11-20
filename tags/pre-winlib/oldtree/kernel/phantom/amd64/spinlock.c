#if 0

#include <spinlock.h>
#include <hal.h>
#include <phantom_libc.h>
#include <kernel/smp.h>

#if SPIN_DEBUG
#include <amd64/proc_reg.h>
#endif

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

    while( !  _spin_try_lock( &(sl->lock)  ) )
        while( sl->lock )
            ;


#if SPIN_DEBUG
    global_lock_entry_count[GET_CPU_ID()]++;
    sl->ebp = (addr_t)arch_get_frame_pointer();
#endif
}

void hal_spin_unlock(hal_spinlock_t *sl)
{
    if (sl) // Scheduler sometimes calls us will sl == 0
    {
    assert(sl->lock);
#if SPIN_DEBUG
    sl->ebp = 0;
    global_lock_entry_count[GET_CPU_ID()]--;
#endif
    _spin_unlock(&(sl->lock));
	}
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

#endif
