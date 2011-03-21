
#include <kernel/trap.h>

//#include <i386/trap.h>
#include <i386/eflags.h>

#include <phantom_assert.h>
#include <phantom_libc.h>
#include <threads.h>
#include <thread_private.h>

#include <mach/debug_reg.h>
#include <i386/proc_reg.h>

#include <signal.h>

#include "misc.h"




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


int trap2signo( struct trap_state *ts )
{
    int		sig_no = 0;

    /*
     * It probably makes no sense to pass all these signals to the
     * application program.
     */
    switch (ts->trapno)
    {

    case -1: 	sig_no = SIGINT;  break;	/* hardware interrupt */
    case 0: 	sig_no = SIGFPE;  break;	/* divide by zero */
    case 1: 	sig_no = SIGTRAP; break;	/* debug exception */
    case 3: 	sig_no = SIGTRAP; break;	/* breakpoint */
    case 4: 	sig_no = SIGFPE;  break;	/* overflow */
    case 5: 	sig_no = SIGFPE;  break;	/* bound instruction */
    case 6: 	sig_no = SIGILL;  break;	/* Invalid opcode */
    case 7: 	sig_no = SIGFPE;  break;	/* coprocessor not available */
    case 9: 	sig_no = SIGBUS;  break;	/* coproc segment overrun*/
    case 10: 	sig_no = SIGBUS;  break;	/* Invalid TSS */
    case 11: 	sig_no = SIGSEGV; break;	/* Segment not present */
    case 12: 	sig_no = SIGSEGV; break;	/* stack exception */
    case 13: 	sig_no = SIGSEGV; break;	/* general protection */
    case 14: 	sig_no = SIGSEGV; break;	/* page fault */
    case 16: 	sig_no = SIGFPE;  break;	/* coprocessor error */

    default:
        panic("No signal mapping for trap number: %d", ts->trapno);
    }

    /*
     * Look for null pointer exceptions ...
     */
    if(ts->trapno == 1)
    {
        unsigned dr6 = rdr6();

        if (dr6 & (DR6_B0|DR6_B1))
            sig_no = SIGSEGV;
        else
            sig_no = SIGTRAP;
    }

    return sig_no;
}


