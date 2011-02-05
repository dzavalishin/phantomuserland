#include <i386/trap.h>
#include <i386/eflags.h>
#include <phantom_assert.h>
#include <phantom_libc.h>
#include <threads.h>
#include <thread_private.h>

#include "misc.h"

void
phantom_check_user_trap( struct trap_state *ts )
{
    phantom_thread_t *t = GET_CURRENT_THREAD();

    if(t->thread_flags & THREAD_FLAG_USER)
    {
        int tid = t->tid;
        printf("Usermode thread %d killed due to unexpected trap\n", tid);
        t_kill_thread( tid );
        // Will panic below if returned
        printf("Usermode trap panic in thread %d\n", tid);
        trap_panic(ts);
    }
    // Not user mode, return
}



void
phantom_kernel_trap( struct trap_state *ts )
{
    int trapno = ts->trapno;

    check_global_lock_entry_count(); // Debug

    int (*handler)(struct trap_state *ts)
        = phantom_trap_handlers[trapno];

    if(handler == 0)
    {
        phantom_check_user_trap( ts ); // returns only if not an user mode thread
        trap_panic(ts);
    }

    if(handler(ts))
    {
        dump_ss(ts);
        panic("Trap handler failed for trap %s\n", trap_name(trapno));
    }
}

