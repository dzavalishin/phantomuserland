#if HAVE_UNIX

#include <unix/uufile.h>
#include <unix/uuprocess.h>
#include <malloc.h>


uuprocess_t     	proc[MAX_UU_PROC];
static int      	next_pid = 10;
static hal_mutex_t      proc_lock;

static int get_pid();
static uuprocess_t * proc_by_pid(int pid);

uuprocess_t *uu_create_process(int ppid)
{
    hal_mutex_lock(&proc_lock);
    uuprocess_t *p = calloc( 1, sizeof(uuprocess_t) );

    p->pid = get_pid();

    p->ppid = p->pid;
    p->pgrp_pid = p->pid;
    p->sess_pid = p->pid;

    uuprocess_t * parent = proc_by_pid(ppid);
    if( parent )
    {
        p->ppid = ppid;
        p->pgrp_pid = parent->pgrp_pid;
        p->sess_pid = parent->sess_pid;
        p->ctty = parent->ctty;
        p->cwd = copy_uufile( parent->cwd );
    }


    hal_mutex_unlock(&proc_lock);
    return p;
};


static uuprocess_t * proc_by_pid(int pid)
{
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

#endif // HAVE_UNIX

