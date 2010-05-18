/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Thread switch and helpers.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/


#define DEBUG_MSG_PREFIX "thread"
#include "../debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10



#include "../config.h"
#include "thread_private.h"

#include <queue.h>
#include <phantom_libc.h>

// TSS - TODO - move to machdep thread switch code
#include <i386/tss.h>

hal_spinlock_t schedlock;

static hal_spinlock_t unused_lock;

//static int preemption_disabled = 0;

#if 1

// Must be called from softint handler, actually switches context
void phantom_thread_switch()
{
    int ie = hal_save_cli();
    hal_spinlock_t *toUnlock;

    if(GET_CURRENT_THREAD()->sw_unlock == &schedlock)
    {
        SHOW_ERROR0(0, "schedlock passed as sw_unlock!");
        GET_CURRENT_THREAD()->sw_unlock = 0;
        toUnlock = &unused_lock;
    }
    else
    {
        hal_spin_lock(&schedlock);

        toUnlock = GET_CURRENT_THREAD()->sw_unlock;
        GET_CURRENT_THREAD()->sw_unlock = 0;

        if(toUnlock == 0) toUnlock = &unused_lock;

        assert(toUnlock != &schedlock);
        // FIXME This is SMP-wrong! It has to beunlocked AFTER thread is swithed off
        //if(toUnlock) hal_spin_unlock(toUnlock);
    }

    // Eat rest of tick
    GET_CURRENT_THREAD()->ticks_left--;

    phantom_thread_t *next = phantom_scheduler_select_thread_to_run();
    phantom_thread_t *old = GET_CURRENT_THREAD();

    if(next == old)
    {
        //printf("same thread selected\n");
        // threads_stat.samethread++;

        if(old->sleep_flags)
            panic("blocked thread selected");

        goto exit;
    }

    t_dequeue_runq(next);

    if(!old->sleep_flags)
        t_enqueue_runq(old);

    // do it before - after we will have stack switched and can't access
    // correct 'next'
    //GET_CURRENT_THREAD() = next;
    SET_CURRENT_THREAD(next);

    hal_enable_softirq();
    phantom_switch_context(old, next, toUnlock );
    // threads_stat.switches++;
    hal_disable_softirq();

    {
        // TODO machdep, header
        extern struct i386_tss	       	tss;

        phantom_thread_t *t = GET_CURRENT_THREAD();
        // TODO 64 bug
        tss.esp0 = (int)t->kstack_top;
    }

exit:
    hal_spin_unlock(&schedlock);

    if(ie) hal_sti();

}


#else
// Must be called from softint handler, actually switches context
void phantom_thread_switch()
{
    int ie = hal_save_cli();

    if(GET_CURRENT_THREAD()->sw_unlock == &schedlock)
    {
        GET_CURRENT_THREAD()->sw_unlock = 0;
    }
    else
    {
        hal_spin_lock(&schedlock);

        hal_spinlock_t *toUnlock = GET_CURRENT_THREAD()->sw_unlock;
        GET_CURRENT_THREAD()->sw_unlock = 0;

        assert(toUnlock != &schedlock);
        // FIXME This is SMP-wrong! It has to beunlocked AFTER thread is swithed off
        if(toUnlock) hal_spin_unlock(toUnlock);
    }

    // Eat rest of tick
    GET_CURRENT_THREAD()->ticks_left--;

    // If we got here, lets do it anyway.
    //if(!phantom_scheduler_is_reschedule_needed())        goto exit;


    phantom_thread_t *next = phantom_scheduler_select_thread_to_run();
    phantom_thread_t *old = GET_CURRENT_THREAD();

    if(next == old)
    {
        //printf("same thread selected\n");
        // threads_stat.samethread++;

        if(old->sleep_flags)
            panic("blocked thread selected");

        goto exit;
    }

    t_dequeue_runq(next);

    if(!old->sleep_flags)
        t_enqueue_runq(old);

    // do it before - after we will have stack switched and can't access
    // correct 'next'
    GET_CURRENT_THREAD() = next;

    hal_enable_softirq();
    phantom_switch_context(old, next, &schedlock );
    // threads_stat.switches++;
    hal_disable_softirq();

    {
        // TODO machdep, header
        extern struct i386_tss	       	tss;

        phantom_thread_t *t = GET_CURRENT_THREAD();
        tss.esp0 = t->kstack_top;
    }

exit:
    hal_spin_unlock(&schedlock);

    if(ie) hal_sti();

}
#endif



void hal_disable_preemption(void)
{
    //int ret = !GET_CURRENT_THREAD()->preemption_disabled;
    GET_CURRENT_THREAD()->preemption_disabled = 1;
    //return ret;
}

int hal_disable_preemption_r(void)
{
    int ret = !GET_CURRENT_THREAD()->preemption_disabled;
    GET_CURRENT_THREAD()->preemption_disabled = 1;
    return ret;
}

void hal_enable_preemption(void)
{
    GET_CURRENT_THREAD()->preemption_disabled = 0;
}

int hal_is_preemption_disabled()
{
    return GET_CURRENT_THREAD()->preemption_disabled;
}
