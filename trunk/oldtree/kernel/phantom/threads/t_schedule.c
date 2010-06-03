/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Scheduler and helpers.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include "thread_private.h"

#include <queue.h>
#include <phantom_libc.h>

#define CHECK_SLEEP_FLAGS 1


static volatile int reschedule_request = 0;

/** Idle prio threads */
static queue_head_t	runq_idle = {0,0};

/** Normal prio threads */
static queue_head_t	runq_norm = {0,0};

/** Realtime prio threads */
static queue_head_t	runq_rt = {0,0};


void phantom_scheduler_init(void)
{
    queue_init(&runq_idle);
    queue_init(&runq_norm);
    queue_init(&runq_rt);

    hal_spin_init(&schedlock);
}




void phantom_scheduler_request_reschedule( /* reason? */ void )
{
    reschedule_request = 1;
    phantom_scheduler_schedule_soft_irq();
}



void phantom_scheduler_yield( void )
{
    phantom_scheduler_request_reschedule( /* YIELD? */  );

    //assert(!hal_is_preemption_disabled()); // why?

    // BUG!! Race here. Softint might happen and switch us off!

    //hal_spinlock_t dummy;
    //phantom_thread_switch(&dummy);
    //phantom_thread_switch();

    // TODO in fact, possibly, it is ok just to call phantom_scheduler_soft_interrupt() from here?
    phantom_scheduler_request_soft_irq();

    // assert that cur thread is runnable
}


void phantom_scheduler_yield_locked( hal_spinlock_t *lock )
{
    GET_CURRENT_THREAD()->sw_unlock = lock;
    phantom_scheduler_request_reschedule( /* YIELD? */  );

    //assert(!hal_is_preemption_disabled()); // why?

    // BUG!! Race here. Softint might happen and switch us off!


    //phantom_thread_switch();
    phantom_scheduler_request_soft_irq();

    // assert that cur thread is runnable
}





static volatile int phantom_scheduler_soft_interrupt_reenter = 0;

void phantom_scheduler_soft_interrupt(void)
{
    //#warning dummy?

    // if curr thread disables preemption or scheduler lock is here,
    // don't interfere! BUT - if thread has sleep_flags, then we are
    // called to switch it off, so do it!
#if 0
    if(
       (GET_CURRENT_THREAD()->sleep_flags == 0 )
        &&
       (hal_is_preemption_disabled() || schedlock.lock)
      )
        return;
#else
    if(schedlock.lock)
        return;

    if( (GET_CURRENT_THREAD()->sleep_flags == 0) && hal_is_preemption_disabled() )
        return;
#endif

    //if(phantom_scheduler_soft_interrupt_reenter)        panic("phantom_scheduler_soft_interrupt_reenter");

    phantom_scheduler_soft_interrupt_reenter++;

    phantom_thread_switch();

    phantom_scheduler_soft_interrupt_reenter--;

}



int phantom_scheduler_is_reschedule_needed(void)
{
    // Has to check:

    // 1. if there is a reschedule request exists
    if(reschedule_request)
        return 1;

    // 2. if curr thread is used out it's time slot and there is
    //    someone else ready to run.
    if( GET_CURRENT_THREAD()->ticks_left <= 0 )
        return 1;

    // 3. Check if current thread is blocked!
    //    We're not locking, as no race here. if someone unblocks us
    //    later, scheduler will take it in account.
    if( GET_CURRENT_THREAD()->sleep_flags )
        return 1;

    return 0;
}

// Called from timer interrupt 100 times per sec.
// todo add call to timer_int_handler() in ../i386/pit.c
void phantom_scheduler_time_interrupt(void)
{
    if(GET_CURRENT_THREAD()->priority & THREAD_PRIO_MOD_REALTIME)
        return; // Realtime thread will run until it blocks or reschedule requested

    if( (GET_CURRENT_THREAD()->ticks_left--) <= 0 )
        phantom_scheduler_request_reschedule();
}







int t_is_on_runq(phantom_thread_t *t, queue_head_t *h)
{
    phantom_thread_t *it = 0;
    queue_iterate(h, it, phantom_thread_t *, runq_chain)
        if(t == it)
            return 1;
    return 0;

}


void t_dequeue_runq(phantom_thread_t *t)
{
    if(t->priority & THREAD_PRIO_MOD_REALTIME )
    {
        assert(t_is_on_runq(t,&runq_rt));
        queue_remove(&runq_rt, t, phantom_thread_t *, runq_chain);
    }
    else if(t->priority == THREAD_PRIO_IDLE )
    {
        assert(t_is_on_runq(t,&runq_idle));
        queue_remove(&runq_idle, t, phantom_thread_t *, runq_chain);
    }
    else
    {
        assert(t_is_on_runq(t,&runq_norm));
        queue_remove(&runq_norm, t, phantom_thread_t *, runq_chain);
    }

    // Can be used to check if we're on runq
    t->runq_chain.next = 0;
    t->runq_chain.prev = 0;
}

void t_enqueue_runq(phantom_thread_t *t)
{
    assert(!t_is_on_runq(t,&runq_rt));
    assert(!t_is_on_runq(t,&runq_idle));
    assert(!t_is_on_runq(t,&runq_norm));

    if(t->priority & THREAD_PRIO_MOD_REALTIME )
    {
        queue_enter(&runq_rt, t, phantom_thread_t *, runq_chain);
    }
    else if(t->priority == THREAD_PRIO_IDLE )
    {
        queue_enter(&runq_idle, t, phantom_thread_t *, runq_chain);
    }
    else
    {
        queue_enter(&runq_norm, t, phantom_thread_t *, runq_chain);
    }
}










// Each thread runs for 0.1 sec normally
#define NORM_TICKS 10
//#define NORM_TICKS 1

static int t_assign_time(void);
static int t_is_runnable(phantom_thread_t *t) { return t->sleep_flags == 0; }


/**
 *
 * Scheduler itself. Selects a next thread to be run on this CPU.
 * Not sure if it is SMP ready yet, but written with SMP in mind.
 *
**/


// NB! Must be called under schedlock to make sure thread
// is not stolen by another CPU in SMP system

phantom_thread_t *phantom_scheduler_select_thread_to_run(void)
{
    assert(schedlock.lock);

    // TODO find out if it is time to replentish ticks_left

    phantom_thread_t *ret = 0;

//retry:
    // Have some realtime?
    if( !queue_empty(&runq_rt) )
    {
        int maxprio = -1;
        phantom_thread_t *best = 0;
        phantom_thread_t *it = 0;
        queue_iterate(&runq_rt, it, phantom_thread_t *, runq_chain)
        {
            assert( (it->priority & THREAD_PRIO_MOD_REALTIME) != 0);
#if CHECK_SLEEP_FLAGS
            // hack?
            if( it->sleep_flags ) continue;
#endif
            if( ((int)it->priority) > maxprio )
            {
                maxprio = it->priority;
                best = it;
            }
        }
        if( best )
        {
            assert(t_is_runnable(best));
            return best;
        }
    }


    // Have normal one?
    if( !queue_empty(&runq_norm) )
    {
        // If no normal thread has ticks left, reassign
        // ticks and retry
        do {
            unsigned int maxprio = 0; // NB! not a negative number!
            phantom_thread_t *best = 0;
            phantom_thread_t *it = 0;
            queue_iterate(&runq_norm, it, phantom_thread_t *, runq_chain)
            {
#if DEBUG
                printf("sel th @ tid %d, prio %d, ticks %d\n", it->tid, it->priority, it->ticks_left );
#endif
                //if( it->priority <= maxprio )                    continue;

                //if( it->ticks_left <= 0)                    continue;

#if CHECK_SLEEP_FLAGS
                // hack?
                if( it->sleep_flags ) continue;
#endif
                if( (it->priority > maxprio) && (it->ticks_left > 0) )
                {
                    maxprio = it->priority;
                    best = it;
                }
            }
            if( best )
            {
                assert(t_is_runnable(best));
#if DEBUG
                printf("---\nselected tid %d, prio %d, ticks %d\n", best->tid, best->priority, best->ticks_left );
#endif
                return best;
            }
        } while(t_assign_time());
    }




idle_again:
    if(!queue_empty(&runq_idle))
    {
        // Have idle one?
        ret = (phantom_thread_t *)queue_first(&runq_idle);
#if CHECK_SLEEP_FLAGS
        // hack?
        if( ret->sleep_flags )
        {
            queue_remove(&runq_idle, ret, phantom_thread_t *, runq_chain);
            queue_enter(&runq_idle, ret, phantom_thread_t *, runq_chain);
            goto idle_again;
        }
#endif
        if( ret )
        {
            // Just take first. Switched off thread will become
            // last on runq, so all idle threads will run in cycle
            ret->ticks_left = NORM_TICKS;
            assert(t_is_runnable(ret));
            return ret;
        }
    }


    // NOTHING!
#if 0
    {
        panic("Where's scheduler idle thread?");
        // TODO switch to (idle) thread 0 and move this stuff there.
        // really nothing to do. Stop CPU and wait 4 int
        assert(!hal_is_sti());


        int ie = hal_save_cli();
        hal_spin_unlock(&schedlock);
        hal_sti();
        asm volatile("hlt" : : );
        if( !ie ) hal_cli();
        hal_spin_lock(&schedlock);

        goto retry;
    }
#endif
    return GET_IDLEST_THREAD(); // No real thread is ready to run
}




// NB! Must be called under schedlock 
// Check if all normal threads eaten their time slots, assign new ones
static int t_assign_time(void)
{
    assert(schedlock.lock);

    phantom_thread_t *it = 0;
    int haveTicks = 0;
    int cntRunnable = 0;
    queue_iterate(&runq_norm, it, phantom_thread_t *, runq_chain)
    {
        assert( (it->priority & THREAD_PRIO_MOD_REALTIME) == 0);
        cntRunnable++;
        if( it->ticks_left > 0 )
            haveTicks = 1;
    }

    if(!haveTicks)
    {
#if 0
        // Leth them all rotate in 0.1 sec max
        int giveTicks = 10/cntRunnable;
        if(giveTicks < 2) giveTicks = 2;
#endif
        queue_iterate(&runq_norm, it, phantom_thread_t *, runq_chain)
        {
            // TODO a very dumb priority handling
            it->ticks_left = NORM_TICKS + it->priority; // = giveTicks
        }

        it = GET_CURRENT_THREAD();
        it->ticks_left = NORM_TICKS + it->priority; // = giveTicks


        return 1;
    }
    return 0;
}












void thread_block( int sleep_flag, hal_spinlock_t *lock_to_be_unlocked )
{
    assert(lock_to_be_unlocked != &schedlock);

    int ie = hal_save_cli();
    hal_spin_lock(&schedlock);

#if SPIN_DEBUG
    assert(global_lock_entry_count == 2);
#endif

    phantom_thread_t *t = GET_CURRENT_THREAD();
    //t_dequeue_runq(t);

#if LATENCY_DEBUG
    if (!t->sleep_flags &&
            (sleep_flag & THREAD_SLEEP_MUTEX))
    {
        t->sleep_start = hal_system_time();
    }
#endif

    t->sleep_flags |= sleep_flag;

    hal_spin_unlock(&schedlock);
    if(ie) hal_sti();

    //while(1)
    {
        phantom_scheduler_yield_locked(lock_to_be_unlocked);
        if(t->sleep_flags)
            panic("woken with t->sleep_flags nonzero");
    }

}




void thread_unblock( phantom_thread_t *t, int sleep_flag )
{
    int ie = hal_save_cli();
    hal_spin_lock(&schedlock);

    assert( t->sleep_flags & sleep_flag );

#if LATENCY_DEBUG
    if ((t->sleep_flags & sleep_flag) & THREAD_SLEEP_MUTEX)
    {
        bigtime_t delay = hal_system_time() - t->sleep_start;
        if (delay > t->max_sleep)
        {
            t->max_sleep = delay;
            printf("max latency: %d for ", delay);
            phantom_scheduler_request_soft_irq();dump_thread_info(t);
            dump_thread_stack(t);
            printf("released by ");
            dump_thread_info(GET_CURRENT_THREAD());
            stack_dump();
        }
    }
#endif

    t->sleep_flags &= ~sleep_flag;

    if(!t->sleep_flags)
    {
        t_enqueue_runq(t);
        hal_spin_unlock(&schedlock);
        if(ie) hal_sti();
        phantom_scheduler_request_reschedule( /* UNBLOCK */  );

        return;
    }

    hal_spin_unlock(&schedlock);
    if(ie) hal_sti();
}

