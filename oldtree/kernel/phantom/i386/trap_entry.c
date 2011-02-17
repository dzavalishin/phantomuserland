#include <i386/trap.h>
#include <i386/eflags.h>

#include <phantom_assert.h>
#include <phantom_libc.h>
#include <threads.h>
#include <thread_private.h>

#include <mach/debug_reg.h>
#include <i386/proc_reg.h>

#include <signal.h>

#include "misc.h"

static int trap2sig( struct trap_state *ts );

void
phantom_check_user_trap( struct trap_state *ts )
{
    phantom_thread_t *t = GET_CURRENT_THREAD();

    if(t->thread_flags & THREAD_FLAG_USER)
    {
        // Try thread trap handler first. If it returns zero - trap
        // is supposed to be handled
        if(t->trap_handler)
        {
            int sig_no = trap2sig(ts);
            if( 0 == ( t->trap_handler( sig_no, ts ) ) )
                return;
        }

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


static int trap2sig( struct trap_state *ts )
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


