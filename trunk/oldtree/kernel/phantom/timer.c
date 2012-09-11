#include <kernel/timedcall.h>
#include <vm/syscall_tools.h>
#include <kernel/snap_sync.h>

#include <phantom_libc.h>

#if OLD_VM_SLEEP
#warning i am obsolete

#define CN_TM_NET 1


static void
wakeThread(void *arg)
{
    printf("thread awake");
    struct data_area_4_thread *tc = (struct data_area_4_thread*)arg;
    SYSCALL_WAKE_THREAD_UP(tc);
}


// TODO use spinlock to make sure wakeup won't happen until sleep
// wakeup thread after a timeout

void phantom_wakeup_after_msec(int msec, struct data_area_4_thread *tc)
{
    printf("phantom_wakeup_after_msec %d\n", msec);
    //phantom_request_timed_func( wakeThread, (void *)tc, msec, 0 );

    // TODO check sleep_flag in os restart and restart timer as well?

#if CN_TM_NET
    net_timer_event *e = &(tc->timer);

    int rc = set_net_timer(e, msec, wakeThread, tc, 0);
    if( rc )
    {
        printf("phantom_wakeup_after_msec %d failed\n", msec);
        SYSCALL_WAKE_THREAD_UP(tc);
    }
#else
    timedcall_t *e = &(tc->timer);

    e->arg = tc;
    e->f = wakeThread;
    e->msecLater = msec;

    phantom_request_timed_call( e, 0 );
#endif
}

#endif

