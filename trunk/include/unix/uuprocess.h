#ifndef UUPROCESS_H
#define UUPROCESS_H

#include <signal.h>


#define MAX_UU_FD       256
#define MAX_UU_TID      128
#define MAX_UU_CMD      256

#define MAX_UU_PROC     2048

// pids before this are reserved
#define MIN_PID 10




// This structure is pointed by processes that are running it
// and describes loadable execution unit

// In fact, currently this struct can't be reused because
// CS and DS are allocated at once
struct exe_module
{
    int                 refcount;

    char                name[MAX_UU_CMD]; // debug only

    u_int32_t           start; // code entry point
    u_int32_t           esp;
	u_int32_t			stack_bottom; // user addr

    // possibly here we have to have syscall service pointer
    //physaddr_t  	phys_cs;
    u_int16_t           cs_seg;
    linaddr_t           cs_linear;
    size_t              cs_pages;

    //physaddr_t  	phys_ds;
    u_int16_t           ds_seg;
    linaddr_t           ds_linear;
    size_t              ds_pages;

    // for pv_free
    physaddr_t          pa;
    size_t              mem_size;

    void *              mem_start; // va
    void *              mem_end; // above last addr
};






struct uufile;
struct uutty;


struct uuprocess
{
	//hal_mutex_t		lock;

    int                 pid; // < 0 for unused struct
    int                 ppid;
    int                 pgrp_pid;
    int                 sess_pid;

    int                 uid, euid;
    int                 gid, egid;

    struct uutty *      ctty;

    char                cmd[MAX_UU_CMD];
    const char **       argv;
    int                 argc;
    const char **       envp;

    struct uufile *     fd[MAX_UU_FD];
    int                 tids[MAX_UU_TID];
    int                 ntids;

    struct uufile *     cwd;

    int                 umask;

    int                 capas; // Capabilities == rights

    // Process address space is mapped to our main linear adrr space
    // as follows. These adresses are used in syscall processing.
    // Any pointer from user process is first has mem_start added and
    // then checked to be in mem_start-mem_end bounds.

    void *		mem_start;		// where it starts in linear addr space
    void *		mem_end;		// first address above proc mem

    struct exe_module * em;

    signal_handling_t   signals;
};

typedef struct uuprocess uuprocess_t;

//uuprocess_t *uu_create_process(int ppid, struct exe_module *em);
//void uu_proc_add_thread( uuprocess_t *p, int tid );
//uuprocess_t *uu_create_process( int ppid );

int uu_create_process( int ppid );

errno_t uu_proc_add_thread( int pid, int tid );
errno_t uu_proc_rm_thread( int pid, int tid );
errno_t uu_proc_setargs( int pid, const char **av, const char **env );
errno_t uu_proc_set_exec( int pid, struct exe_module *em);


errno_t uu_run_file( int pid, const char *fname );
errno_t uu_run_binary( int pid, void *_elf, size_t elf_size );

uuprocess_t * 	proc_by_pid(int pid);


#endif // UUPROCESS_H

