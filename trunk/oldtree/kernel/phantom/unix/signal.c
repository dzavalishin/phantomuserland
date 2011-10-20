/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Signals
 *
**/

#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "sig"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_types.h>
#include <phantom_libc.h>

#include <kernel/trap.h>

#include <signal.h>
#include <threads.h>

#include <unix/uuprocess.h>
#include <unix/uusignal.h>



#define SIG_PUSH(__v_) *((u_int32_t *)(mina+(--(st->esp)))) = (__v_)


errno_t sig_deliver( uuprocess_t *u, struct trap_state *st, int nsig, void *handler )
{
#ifdef ARCH_ia32
    u_int32_t   old_eip = st->eip;
    //u_int32_t   old_esp = st->esp;

    addr_t mina = (addr_t)u->mem_start;

    // Push args backwards
    SIG_PUSH(0); // one more zero arg for any case :)
    SIG_PUSH(nsig);

    // Push cur IP
    SIG_PUSH(old_eip);
    // Change IP to handler address
    st->eip = (addr_t)handler;
#endif

#warning unimpl

    return 0;
}








void sig_init(signal_handling_t *sh)
{
    memset( sh, 0, sizeof(signal_handling_t) );
    sh->signal_mask 	= SIG_DEF_IGNORE;
    sh->signal_stop     = SIG_DEF_STOP;
    sh->signal_cont     = SIG_DEF_CONT;
}

// Translate signals to user
void sig_exec(uuprocess_t *u, signal_handling_t *sh, struct trap_state *st)
{

    // Mask off ignored ones
    u_int32_t s = sh->signal_pending & sh->signal_mask;

    // No signals?
    if( !s )
        return;

    SHOW_FLOW( 1, "Signals %x (unmasked), %x (total)", s, sh->signal_pending );

    u_int32_t do_stop = s & sh->signal_stop;
    u_int32_t do_cont = s & sh->signal_cont;

    u_int32_t do_kill = s & ~(do_stop|do_cont);

    if(do_cont)
    {
        do_stop = 0; // Cont overrides

        // We're going to process all of these
        sh->signal_pending &= ~do_cont;

        SHOW_ERROR( 0, "Unimplemented signal cont %x", do_cont );
    }

    if(do_stop)
    {
        // We're going to process all of these
        sh->signal_pending &= ~do_stop;

        SHOW_ERROR( 0, "Unimplemented signal stop %x", do_stop );
    }


    int i;
    for( i = 0; i < NSIGNAL; i++ )
    {
        u_int32_t sel = 1<<i;

        // Not this?
        if( !(sel & s) )
            continue;

        void *uhandler = sh->signal_handler[i];
        if( uhandler == SIG_DFL )
        {
            int killme = do_kill & s;
            if(killme)
            {
                SHOW_ERROR( 0, "Unimplemented signal kill %d", i );
            }
        }
        else if( uhandler == (void *)SIG_IGN )
        {
            // None
        }
        else
        {
            // TODO mask here?
            sig_deliver( u, st, i, uhandler );
        }

    }

}

// Send a signal (turn bit on)
void sig_send(signal_handling_t *sh, int signal )
{
    assert( signal >= 0 && signal < NSIGNAL );
    sh->signal_pending |= (1 << signal);
}

void sig_send_to_current(int signal)
{
    tid_t tid = get_current_tid();
    pid_t pid;

    errno_t rc = t_get_pid( tid, &pid );
    if( rc )
    {
        SHOW_ERROR( 0, "Attempt to send sig to thread %d with no corresponging process", tid );
        return;
    }

    SHOW_FLOW( 3, "Send sig %d to pid %d", signal, pid );

    uuprocess_t *u = proc_by_pid(pid);

    signal_handling_t *sh = &u->signals;
    sig_send( sh, signal );
}


// called on trap state before return to produce signal handling in u a space
void execute_signals(uuprocess_t *u, struct trap_state *st)
{
    signal_handling_t *sh = &u->signals;
    sig_exec(u, sh, st);
}



int usys_sigpending( int *err, uuprocess_t *u, sigset_t * set)
{
    (void) err;

    signal_handling_t *sh = &u->signals;
    *set = sh->signal_pending;
    return 0;
}

sighandler_t usys_signal( int *err, uuprocess_t *u, int signum, sighandler_t handler)
{
//#warning impl
    signal_handling_t *sh = &u->signals;
    if( signum < 0 || signum > _SIG_MAXSIG )
    {
        *err = EINVAL;
        return (void *)SIG_ERR;
    }

    sighandler_t ret = sh->signal_handler[signum];

    sh->signal_handler[signum] = handler;

    if( handler == (void *)SIG_IGN)
        sh->signal_mask |= (1<<signum);
    else
        sh->signal_mask &= ~(1<<signum);

    return ret;
}

#endif // HAVE_UNIX
