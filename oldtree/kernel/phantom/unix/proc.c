/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Unix processes
 *
 *
**/


#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "proc"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include <sys/fcntl.h>

#include <kernel/init.h>
#include <kernel/debug.h>
#include <kernel/unix.h>

#ifdef ARCH_ia32
#include <ia32/selector.h>
#endif

#include <unix/uufile.h>
#include <unix/uuprocess.h>
#include <unix/uusignal.h>

#include <threads.h>
#include <thread_private.h>


uuprocess_t     	proc[MAX_UU_PROC];
static int      	next_pid = 10;
static hal_mutex_t      proc_lock;

static int 		get_pid();
//static uuprocess_t * 	proc_by_pid(int pid);

static void 		reopen_stdioe(uuprocess_t *u, const char *fname);

static const char**	dup_argv(int *oac, const char**, const char *replace_exe);
static void 		free_argv(const char**);
static void 		flatten_argv( char* out, int len, const char** argv );

static void 		dbg_ps(int argc, char **argv);



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





uuprocess_t * proc_by_pid(int pid)
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

#ifdef ARCH_ia32
    free_ldt_selector(em->cs_seg);
    free_ldt_selector(em->ds_seg);
#endif

    uu_unlink_exe_module(em);
}

// Called in thread death callback.
static void uu_proc_thread_kill( phantom_thread_t *t )
{
    assert(t);

    //uuprocess_t *p = t->u;
    tid_t tid = t->tid;
    pid_t pid;

    if( t_get_pid( tid, &pid ) )
    {
        SHOW_ERROR( 1, "Thread %d must have pid but doesnt", tid );
        return;
    }

    //assert(p);

    SHOW_FLOW( 1, "Thread %d of process %d dies", tid, pid );

    uu_proc_rm_thread( pid, tid );
}



// -----------------------------------------------------------------------
// Public
// -----------------------------------------------------------------------


int uu_create_process( int ppid )
{
    hal_mutex_lock(&proc_lock);
    uuprocess_t *p = get_proc();
    assert(p);

    memset( p, 0, sizeof(uuprocess_t) );

    p->pid = get_pid();

    p->ppid = p->pid;
    p->pgrp_pid = p->pid;
    p->sess_pid = p->pid;

    p->uid = p->euid = p->gid = p->egid = 0; // Let it be root at start
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
        p->cwd_file = copy_uufile( parent->cwd_file );
        memcpy( p->cwd_path, parent->cwd_path, FS_MAX_PATH_LEN );
        p->umask = parent->umask;
    }
    else
    {
        //reopen_stdioe( p, "/dev/tty" );
        strlcpy( p->cwd_path, "/", FS_MAX_PATH_LEN );
    }

    sig_init( &(p->signals) );


    // Mostly created, do final things


    SHOW_FLOW( 11, "ctty %p", p->ctty );

    // allways, while there is no files inherited
    reopen_stdioe( p, "/dev/tty" );



    hal_mutex_unlock(&proc_lock);
    return p->pid;
};




// Add thread to process.
errno_t uu_proc_add_thread( int pid, int tid )
{
    errno_t rc = 0;

    assert( tid >= 0 );
    assert( pid > 0 );

/*
    // TODO t_set_pid
    phantom_thread_t *t = get_thread(tid);
    assert( t );
    assert( t->u == 0 );
*/

    hal_mutex_lock(&proc_lock);

    rc = t_set_pid( tid, pid );
    if( rc ) goto finish;

    uuprocess_t * p = proc_by_pid(pid);
    if( !p )
    {
        hal_mutex_unlock(&proc_lock);
        return ESRCH;
    }

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

    //t->u = p;
    hal_set_thread_death_handler(uu_proc_thread_kill);

    if(!done) panic("out of thread slots for proc");

finish:
    hal_mutex_unlock(&proc_lock);
    return rc;
}

// remove (dead) thread from process.
errno_t uu_proc_rm_thread( int pid, int tid )
{
    assert( tid >= 0 );

    hal_mutex_lock(&proc_lock);

    uuprocess_t * p = proc_by_pid(pid);
    if( !p )
    {
        hal_mutex_unlock(&proc_lock);
        return ESRCH;
    }

    assert(p->ntids > 0);

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
    return 0;
}

errno_t uu_proc_setargs( int pid, const char **av, const char **env )
{
    errno_t r = 0;
    hal_mutex_lock(&proc_lock);

    uuprocess_t * u = proc_by_pid(pid);
    if( u )
    {
        if(u->argv) free_argv(u->argv);
        if(u->envp) free_argv(u->envp);

        u->argv = dup_argv( &u->argc, av, 0 );
        u->envp = dup_argv( 0, env, 0 );

        flatten_argv( u->cmd, MAX_UU_CMD, u->argv );
    }
    else
        r = ESRCH;

    hal_mutex_unlock(&proc_lock);
    return r;
}

errno_t uu_proc_set_exec( int pid, struct exe_module *em)
{
    assert(em);
    // Caller must increment
    assert( em->refcount > 0 );

    errno_t r = 0;
    hal_mutex_lock(&proc_lock);

    uuprocess_t * p = proc_by_pid(pid);
    if( p )
    {
        if( p->em )
        {
            // todo unlink em
            SHOW_ERROR( 0, "Process %d already has em", pid );
        }

        p->em = em;

        p->mem_start = em->mem_start;
        p->mem_end = em->mem_end;

        const char *name = "?";
        if( p->argv[0] )
            name = p->argv[0];

        strncpy( em->name, name, MAX_UU_CMD );

    }
    else
        r = ESRCH;

    hal_mutex_unlock(&proc_lock);
    return r;
}


// -----------------------------------------------------------------------
// Tools
// -----------------------------------------------------------------------


static void reopen_stdioe(uuprocess_t *u, const char *fname)
{
    int err;
    usys_close( &err, u, 0 );
    usys_close( &err, u, 1 );
    usys_close( &err, u, 2 );

    if( usys_open( &err, u, fname, O_RDONLY, 0 ) != 0 )
        SHOW_ERROR( 0, "can't open %s for stdin, %d", fname, err );

    if( usys_open( &err, u, fname, O_WRONLY, 0 ) != 1 )
        SHOW_ERROR( 0, "can't open %s for stdout, %d", fname, err );

    if( usys_open( &err, u, fname, O_WRONLY, 0 ) != 2 )
        SHOW_ERROR( 0, "can't open %s for stderr, %d", fname, err );
}


static const char**dup_argv( int *oac, const char**av, const char *replace_exe)
{
    const char *def = 0;
    if( av == 0 )
        av = &def;

    const char ** avc = av;
    int ac = 0;

    assert(av);

    while(*av++)
        ac++;

    if( oac ) *oac = ac;

    if( replace_exe == 0 )
        replace_exe = avc[0];

    const char ** avr = calloc( ac+1, sizeof(char *) );

    while( ac-- )
    {
        avr[ac] = strdup( ac ? avc[ac] : replace_exe );
        SHOW_FLOW( 7, "argv[%d] added '%s'", ac, avr[ac] );
    }

    return avr;
}


// TODO use it in killing u
static void free_argv(const char**av)
{
    const char ** avc = av;
    assert(av);

    while(*av)
    {
        free((void *)(*av++));
    }
    free(avc);
}


static void flatten_argv( char* out, int len, const char** av )
{
    assert(av);
    assert(out);
    assert(len > 0);

    *out = 0;

    while(*av)
    {
        SHOW_FLOW( 8, "argv add '%s'", *av );
        strlcat( out, *av++, len );
        if(*av) strlcat( out, " ", len );
    }

    SHOW_FLOW( 7, "argv flattenned to '%s'", out );

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
    (void) u;

    hal_mutex_lock(&proc_lock);

    //panic("kill is not implemented");
    uuprocess_t * p = proc_by_pid(pid);

    if(!p)
    {
        *err = ESRCH;
        return -1;
    }

    int ret = 0;
    sig_send( &p->signals, sig );

    hal_mutex_unlock(&proc_lock);

    if(ret)
        *err = EINVAL;

    return ret;
}



int usys_waitpid(int *err, uuprocess_t *u, int pid, int *status, int options)
{
    int retpid = -1;

    (void) status;
    (void) options;

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
#warning impl
finish:

    hal_mutex_unlock(&proc_lock);


    return retpid;
}


// to get flags. it will be better to get 'em to separate include
#include <user/sys_phantom.h>

int usys_run( int *err, uuprocess_t *u,  const char *fname, const char **uav, const char **uep, int flags )
{
    if(flags)
        SHOW_ERROR( 0, "flags not impl: %x", flags );

    int pid = uu_create_process(u->pid);

    if( pid < 0 )
    {
        SHOW_ERROR( 0, "out of processes running %s", fname );
        *err = EPROCLIM; // TODO is it?
        return -1;
    }

    SHOW_FLOW( 7, "run '%s' setargs", fname );
    uu_proc_setargs( pid, uav, uep );
    *err= uu_run_file( pid, fname );

    // TODO if run file failed, process still exists!

    return *err ? -1 : pid;
}




#endif // HAVE_UNIX

