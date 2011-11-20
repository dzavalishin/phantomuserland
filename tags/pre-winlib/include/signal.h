/**
 *
 * Phantom OS.
 *
 * Copyright (C) 2009-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Unix-like signal definitions. Used by unix emulation
 * and internally, mostly for hardware trap handling.
 *
**/


/*
 * Signal definitions for the Oskit standalone C library
 */
#ifndef SIGNAL_H
#define SIGNAL_H

#include <sys/types.h>
#include <kernel/trap.h>
#include <errno.h>

// supposed to be ia32/machine/signal.h
//#include <machine/signal.h>



// mostly from FreeBSD

#define	SIGHUP		1	/* hangup */
#define	SIGINT		2	/* interrupt */
#define	SIGQUIT		3	/* quit */
#define	SIGILL		4	/* illegal instr. (not reset when caught) */
#define	SIGTRAP		5	/* trace trap (not reset when caught) */
#define	SIGABRT		6	/* abort() */
#define	SIGIOT		SIGABRT	/* compatibility */
#define	SIGEMT		7	/* EMT instruction */
#define	SIGFPE		8	/* floating point exception */
#define	SIGKILL		9	/* kill (cannot be caught or ignored) */
#define	SIGBUS		10	/* bus error */
#define	SIGSEGV		11	/* segmentation violation */
#define	SIGSYS		12	/* non-existent system call invoked */
#define	SIGPIPE		13	/* write on a pipe with no one to read it */
#define	SIGALRM		14	/* alarm clock */
#define	SIGTERM		15	/* software termination signal from kill */

#define	SIGURG		16	/* urgent condition on IO channel */

#define	SIGSTOP		17	/* sendable stop signal not from tty */
#define	SIGTSTP		18	/* stop signal from tty */
#define	SIGCONT		19	/* continue a stopped process */
#define	SIGCHLD		20	/* to parent on child stop or exit */
#define	SIGTTIN		21	/* to readers pgrp upon background tty read */
#define	SIGTTOU		22	/* like TTIN if (tp->t_local&LTOSTOP) */

#define	SIGIO		23	/* input/output possible signal */

#define	SIGXCPU		24	/* exceeded CPU time limit */
#define	SIGXFSZ		25	/* exceeded file size limit */
#define	SIGVTALRM	26	/* virtual time alarm */
#define	SIGPROF		27	/* profiling time alarm */

#define	SIGWINCH	28	/* window size changes */
#define	SIGINFO		29	/* information request */

#define	SIGUSR1		30	/* user defined signal 1 */
#define	SIGUSR2		31	/* user defined signal 2 */

#if 0
#define	SIGTHR		32	/* reserved by thread library. */
#define	SIGLWP		SIGTHR

#define	SIGRTMIN	65
#define	SIGRTMAX	126

#endif




#define	SIG_BLOCK	1	/* block specified signal set */
#define	SIG_UNBLOCK	2	/* unblock specified signal set */
#define	SIG_SETMASK	3	/* set specified signal set */

#define NSIGNAL 32

#define _SIG_MAXSIG NSIGNAL

//typedef unsigned int sigset_t;










typedef struct siginfo {
    int      si_signo;    /* Signal number */
    int      si_errno;    /* An errno value */
    int      si_code;     /* Signal code */
    int      si_trapno;   /* Trap number that caused
    hardware-generated signal
    (unused on most architectures) */
    //pid_t
    int      si_pid;      /* Sending process ID */
    //uid_t
    int      si_uid;      /* Real user ID of sending process */
    int      si_status;   /* Exit value or signal */

    //clock_t  si_utime;    /* User time consumed */
    //clock_t  si_stime;    /* System time consumed */
    //sigval_t si_value;    /* Signal value */

    int      si_int;      /* POSIX.1b signal */
    void    *si_ptr;      /* POSIX.1b signal */
    int      si_overrun;  /* Timer overrun count; POSIX.1b timers */
    int      si_timerid;  /* Timer ID; POSIX.1b timers */
    void    *si_addr;     /* Memory location which caused fault */
    //long     si_band;     /* Band event (was int in glibc 2.3.2 and earlier) */
    int      si_fd;       /* File descriptor */
    //short    si_addr_lsb; /* Least significant bit of address (since kernel 2.6.32) */
} siginfo_t;



typedef void (*sighandler_t)(int);

#define SIG_DFL 0
#define SIG_IGN 1
#define SIG_ERR -1



#ifndef KERNEL


int sigpending(sigset_t *set);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

int raise(int sig);


sighandler_t signal(int signum, sighandler_t handler);


struct sigaction
{
    void     (*sa_handler)(int);
    void     (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t   sa_mask;
    int        sa_flags;
    //void     (*sa_restorer)(void);
};






static inline int sigaddset( sigset_t *set, int signo)
{

    if (signo <= 0 || signo > _SIG_MAXSIG) {
        //errno = EINVAL;
        return (-1);
    }
    *set |= (1<<signo);
    return (0);
}

static inline int
sigdelset(sigset_t *set, int signo)
{

    if (signo <= 0 || signo > _SIG_MAXSIG) {
        //errno = EINVAL;
        return (-1);
    }
    *set &= ~(1<<signo);
    return (0);
}

static inline int
sigemptyset(sigset_t *set)  { *set =  0; return 0; }

static inline int
sigfillset(sigset_t *set)   { *set = ~0; return 0; }

static inline int
sigismember(sigset_t *set, int signo) { return (*set) & (1<<signo); }





#endif







#if 1 //def KERNEL

#define SIG_DEF_IGNORE 	( (1<<SIGWINCH) | (1<<SIGINFO) )
#define SIG_DEF_STOP   	( (1<<SIGSTOP)  | (1<<SIGTSTP) )
#define SIG_DEF_CONT 	( (1<<SIGCONT) )

typedef struct signal_handling
{
    u_int32_t           signal_pending;
    u_int32_t           signal_mask; // 0 for ignored

    // unimpl
    u_int32_t           signal_stop; // 1 = stop process
    u_int32_t           signal_cont; // 1 = resume process
    //u_int32_t           signal_kill; // 1 = kill process

    void *              signal_handler[NSIGNAL]; // 0 = kill

} signal_handling_t;

#include <unix/uusignal.h>


#endif // KERNEL



#endif // SIGNAL_H
