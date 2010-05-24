#include <i386/trap.h>
#include <i386/eflags.h>
#include <phantom_assert.h>
//#include <phantom_libc.h>

#include "misc.h"

void
phantom_kernel_trap( struct trap_state *ts )
{
    int trapno = ts->trapno;

    check_global_lock_entry_count(); // Debug

    int (*handler)(struct trap_state *ts)
        = phantom_trap_handlers[trapno];

    if(handler == 0)
        trap_panic(ts);

    if(handler(ts))
    {
        dump_ss(ts);
        panic("Trap handler failed for trap %s\n", trap_name(trapno));
    }
}

