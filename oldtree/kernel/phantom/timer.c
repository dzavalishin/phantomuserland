#define OWN_TIMER 1

#include <kernel/timedcall.h>
#include <vm/syscall_tools.h>
#include "snap_sync.h"

#include <phantom_libc.h>


static void
wakeThread(void *arg)
{
    printf("thread awake");
    struct data_area_4_thread *tc = (struct data_area_4_thread*)arg;
    SYSCALL_WAKE_THREAD_UP(tc);
}



// wakeup thread after a timeout

void phantom_wakeup_after_msec(int msec, struct data_area_4_thread *tc)
{
    printf("phantom_wakeup_after_msec %d\n", msec);
    phantom_request_timed_func( wakeThread, (void *)tc, msec, 0 );
}

