#include <spinlock.h>
#include <hal.h>
#include <phantom_libc.h>
#include <kernel/smp.h>

#if SPIN_DEBUG
#include <ia32/proc_reg.h>
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


#if SPIN_DEBUG && !HAVE_SMP
//#warning spin reenter debug on
int global_lock_entry_count[MAX_CPUS] = {};

static void spin_dump(hal_spinlock_t *sl)
{
    printf("spinlock reenter detected, prev enter was here:\n");
    stack_dump_from((void *)(sl->ebp));
    panic("reenter");
}
#endif


__asm (".p2align 4;\n"); // for profiler to distingush funcs


void hal_spin_init(hal_spinlock_t *sl)
{
    sl->lock = 0;
    sl->ebp = 0;
}

__asm (".p2align 4;\n"); // for profiler to distingush funcs

void hal_spin_lock(hal_spinlock_t *sl)
{
    if(hal_is_sti())
        printf("\n!spinlock STI!\n");

#if SPIN_DEBUG && !HAVE_SMP
    if(sl->lock)
        spin_dump(sl);
#endif

    while( !  _spin_try_lock( &(sl->lock)  ) )
        while( sl->lock )
            ;


#if SPIN_DEBUG && !HAVE_SMP
    global_lock_entry_count[GET_CPU_ID()]++;
    sl->ebp = (addr_t)arch_get_frame_pointer();
#endif
}

__asm (".p2align 4;\n"); // for profiler to distingush funcs


void hal_spin_unlock(hal_spinlock_t *sl)
{
    if (sl) // Scheduler sometimes calls us will sl == 0
    {
        assert(sl->lock);
#if SPIN_DEBUG && !HAVE_SMP
        sl->ebp = 0;
        global_lock_entry_count[GET_CPU_ID()]--;
#endif
        _spin_unlock(&(sl->lock));
    }
    if(hal_is_sti())
        printf("\n!spin unlock STI!\n");
}

__asm (".p2align 4;\n"); // for profiler to distingush funcs



void check_global_lock_entry_count()
{
#if SPIN_DEBUG && 1 && !HAVE_SMP
    if(global_lock_entry_count[GET_CPU_ID()] > 1)
    {
        printf("some spinlock locked!");
        stack_dump_from( arch_get_frame_pointer() );
    }
#endif
}
