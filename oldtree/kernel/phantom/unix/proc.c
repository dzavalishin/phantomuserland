#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "proc"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <unix/uufile.h>
#include <unix/uuprocess.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <kernel/init.h>
#include <kernel/debug.h>
#include <threads.h>
#include <thread_private.h>


uuprocess_t     	proc[MAX_UU_PROC];
static int      	next_pid = 10;
static hal_mutex_t      proc_lock;

static int get_pid();
static uuprocess_t * proc_by_pid(int pid);
void uu_proc_rm_thread( uuprocess_t *p, int tid ); // static?

static void dbg_ps(int argc, char **argv);



void phantom_unix_proc_init(void)
{
    int i;

    hal_mutex_init(&proc_lock, "process");

    for( i = 0; i < MAX_UU_PROC; i++ )
        proc[i].pid = -1;

    // add the debug command
    dbg_add_command(&dbg_ps, "ps", "Process list");
}


// called in proc_lock!
static int getp = 0;
static uuprocess_t *get_proc()
{
    int cnt = MAX_UU_PROC;
    while( cnt-- > 0 )
    {
        if( proc[++getp].pid < 0 )
            return proc+getp;
    }

    return 0;

}





static uuprocess_t * proc_by_pid(int pid)
{
    if(pid < 0) return 0;

    int i;
    for( i = 0; i < MAX_UU_PROC; i++ )
        if( proc[i].pid == pid )
            return proc+i;
    return 0;
}


static int pid_used(int pid)
{
    /*
    int i;
    for( i = 0; i < MAX_UU_PROC; i++ )
        if( proc[i].pid == pid )
            return 1;
            return 0;
            */
    return proc_by_pid(pid) != 0;
}

static int get_pid()
{
    int pid;
    int tries = MAX_UU_PROC+10;

    while(1)
    {
        if(tries-- < 0 )
            panic("out of pids");

        pid = next_pid++;
        if(pid_used(pid))
            continue;
        return pid;
    }
}

// Called on proc death in proc mutex, so - interlocked
static void uu_unlink_exe_module( struct exe_module *em )
{
    assert(em);
    assert(em->refcount > 0);

    em->refcount--;
    if(em->refcount > 0)
        return;

    hal_pv_free( em->pa, em->mem_start, em->mem_size );
    free(em);
}


// Process death handler. called on no threads left.
static void uu_proc_death(uuprocess_t *p)
{
    SHOW_FLOW( 1, "Process %d dies", p->pid );

    // TODO cleanup!

    struct exe_module * em = p->em;

    uu_unlink_exe_module(em);
}

// Called in thread death callback.
static void uu_proc_thread_kill( phantom_thread_t *t )
{
    assert(t);

    uuprocess_t *p = t->u;

    assert(p);

    SHOW_FLOW( 1, "Thread %d of process %d dies", t->tid, p->pid );

    uu_proc_rm_thread( p, t->tid );
}



// -----------------------------------------------------------------------
// Public
// -----------------------------------------------------------------------


uuprocess_t *uu_create_process(int ppid, struct exe_module *em)
{
    hal_mutex_lock(&proc_lock);
    //uuprocess_t *p = calloc( 1, sizeof(uuprocess_t) );
    uuprocess_t *p = get_proc();
    assert(p);
    assert(em);

    // Caller must increment
    assert( em->refcount > 0 );

    memset( p, 0, sizeof(uuprocess_t) );

    p->em = em;

    p->mem_start = em->mem_start;
    p->mem_end = em->mem_end;

    strncpy( p->cmd, em->name, MAX_UU_CMD );

    p->pid = get_pid();

    p->ppid = p->pid;
    p->pgrp_pid = p->pid;
    p->sess_pid = p->pid;

    p->uid = p->euid = p->gid = p->egid = -1;
    p->umask = 0664;

    int i;
    for( i = 0; i < MAX_UU_TID; i++ )
        p->tids[i] = -1;

    uuprocess_t * parent = proc_by_pid(ppid);
    if( parent )
    {
        p->ppid = ppid;
        p->pgrp_pid = parent->pgrp_pid;
        p->sess_pid = parent->sess_pid;
        p->ctty = parent->ctty;
        p->cwd = copy_uufile( parent->cwd );
        p->umask = parent->umask;
    }


    hal_mutex_unlock(&proc_lock);
    return p;
};




// Add thread to process.
void uu_proc_add_thread( uuprocess_t *p, int tid )
{
    assert( tid >= 0 );
    assert( p );
    assert( p->pid >= 0 );

    phantom_thread_t *t = get_thread(tid);
    assert( t );
    assert( t->u == 0 );

    hal_set_thread_death_handler(uu_proc_thread_kill);

    hal_mutex_lock(&proc_lock);

    int done = 0;

    int i;
    for( i = 0; i < MAX_UU_TID; i++ )
    {
        if( p->tids[i] >= 0 )
            continue;

        p->tids[i] = tid;
        p->ntids++;
        done = 1;
        break;
    }

    t->u = p;

    if(!done) panic("out of thread slots for proc");

    hal_mutex_unlock(&proc_lock);
}

// remove (dead) thread from process.
void uu_proc_rm_thread( uuprocess_t *p, int tid )
{
    assert( tid >= 0 );
    assert( p );
    assert(p->ntids > 0);

    // Not sure we can
    //phantom_thread_t *t get_thread(tid);
    //assert( t );

    hal_mutex_lock(&proc_lock);

    int done = 0;

    int i;
    for( i = 0; i < MAX_UU_TID; i++ )
    {
        if( p->tids[i] != tid )
            continue;

        p->tids[i] = -1;
        p->ntids--;
        done = 1;
        break;
    }

    if(!done) panic("not proc's thread");

    if( p->ntids <= 0 )
        uu_proc_death(p);

    hal_mutex_unlock(&proc_lock);

}



// -----------------------------------------------------------------------
// Debug
// -----------------------------------------------------------------------



static void dbg_ps(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    printf("    PID    PPID     UID     MEM  CMD\n");

    int i;
    for( i = 0; i < MAX_UU_PROC; i++ )
    {
        if( proc[i].pid < 0 )
            continue;

        printf("%7d %7d %7d %7d  %s\n",
               proc[i].pid, proc[i].ppid,
               proc[i].uid, proc[i].mem_end-proc[i].mem_start,
               proc[i].cmd
              );

    }


}



// -----------------------------------------------------------------------
// unix syscalls
// -----------------------------------------------------------------------



int usys_kill(int *err, uuprocess_t *u, int pid, int sig)
{
    hal_mutex_lock(&proc_lock);
    panic("kill is not implemented");
    hal_mutex_unlock(&proc_lock);
}



int usys_waitpid(int *err, uuprocess_t *u, int pid, int *status, int options)
{
    int retpid = -1;

    hal_mutex_lock(&proc_lock);

    if( pid <= 0 )
    {
        *err = EINVAL;
            retpid = -1;
    }
    else
    {
        uuprocess_t * p = proc_by_pid(pid);
        if( p == 0 || (p->ppid != u->pid ) )
        {
            *err = ECHILD;
            retpid = -1;
            goto finish;
        }

    }

finish:

    hal_mutex_unlock(&proc_lock);


    return retpid;
}





#endif // HAVE_UNIX

