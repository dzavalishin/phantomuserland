#include <kernel/config.h>

// used elsewhere
//#if HAVE_NET

/*
 ** Copyright 2002, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */

#include <threads.h>
#include <kernel/net_timer.h>
#include "misc.h"
#include <time.h>
#include <compat/newos.h>

#define NET_TIMER_INTERVAL 100 // 100 ms

//#define NET_TIMER_INTERVAL 2000000 // 2000 ms

typedef struct {
    net_timer_event *next;
    net_timer_event *prev;

    hal_mutex_t      lock;
    sem_id           wait_sem;

    thread_id        runner_thread;
} net_timer_queue;

static net_timer_queue net_q;

static void add_to_queue(net_timer_event *e)
{
    net_timer_event *tmp = (net_timer_event *)net_q.next;
    net_timer_event *last = (net_timer_event *)&net_q;

    while(tmp != (net_timer_event *)&net_q) {
        if(tmp->sched_time > e->sched_time)
            break;
        last = tmp;
        tmp = tmp->next;
    }

    // add it to the list here
    e->next = tmp;
    e->prev = last;
    last->next = e;
    tmp->prev = e;
}

static void remove_from_queue(net_timer_event *e)
{
    e->prev->next = e->next;
    e->next->prev = e->prev;
    e->prev = e->next = NULL;
}

static net_timer_event *peek_queue_head(void)
{
    net_timer_event *e = NULL;

    if(net_q.next != (net_timer_event *)&net_q)
        e = net_q.next;

    return e;
}

static int _cancel_net_timer(net_timer_event *e)
{
    if(!e->pending) {
        return ERR_GENERAL;
    }

    remove_from_queue(e);
    e->pending = false;

    return NO_ERROR;
}

int set_net_timer(net_timer_event *e, unsigned int delay_ms, net_timer_callback callback, void *args, int flags)
{
    int err = NO_ERROR;

    mutex_lock(&net_q.lock);

    if(e->pending) {
        if(flags & NET_TIMER_PENDING_IGNORE) {
            err = ERR_GENERAL;
            goto out;
        }
        _cancel_net_timer(e);
    }

    // set up the timer
    e->func = callback;
    e->args = args;
    e->sched_time = system_time() + delay_ms * 1000;
    e->pending = true;

    add_to_queue(e);

out:
    mutex_unlock(&net_q.lock);

    return err;
}

int cancel_net_timer(net_timer_event *e)
{
    int err = NO_ERROR;

    mutex_lock(&net_q.lock);
    err = _cancel_net_timer(e);
    mutex_unlock(&net_q.lock);

    return err;
}

static void net_timer_runner(void *arg)
{
    (void) arg;

    net_timer_event *e;
    bigtime_t now;

    t_current_set_name("Net Timer");

    for(;;) {
        //sem_acquire_etc(net_q.wait_sem, 1, SEM_FLAG_TIMEOUT, NET_TIMER_INTERVAL, NULL);
        hal_sleep_msec(NET_TIMER_INTERVAL);

        now = system_time();
//printf("(^)");
    retry:
        mutex_lock(&net_q.lock);

        // pull off the head of the list and run it, if it timed out
        if((e = peek_queue_head()) != NULL && e->sched_time <= now) {

            remove_from_queue(e);
            e->pending = false;

            mutex_unlock(&net_q.lock);

            e->func(e->args);

            // Since we ran an event, loop back and check the head of
            // the list again, because the list may have changed while
            // inside the callback.
            goto retry;

        } else {
            mutex_unlock(&net_q.lock);
        }
    }

}

int net_timer_init(void)
{
    int err;

    net_q.next = net_q.prev = (net_timer_event *)&net_q;

    err = hal_mutex_init(&net_q.lock, "net timer mutex");
    if(err < 0)
        return err;

    //net_q.wait_sem = sem_create(0, "net timer wait sem");

    if( hal_sem_init(&(net_q.wait_sem), "NetQ") < 0 ) {
        mutex_destroy(&net_q.lock);
        return -1; //net_q.wait_sem;
    }


    //net_q.runner_thread = thread_create_kernel_thread("net timer runner", &net_timer_runner, NULL);
    net_q.runner_thread = hal_start_kernel_thread_arg( &net_timer_runner, NULL );

    if(net_q.runner_thread < 0) {
        sem_delete(net_q.wait_sem);
        mutex_destroy(&net_q.lock);
        return -1; //net_q.runner_thread;
    }
    //thread_resume_thread(net_q.runner_thread);


    return 0;
}

//#endif // HAVE_NET

