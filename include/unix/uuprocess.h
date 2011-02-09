#ifndef UUPROCESS_H
#define UUPROCESS_H


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
	physaddr_t			pa;
	size_t				mem_size;

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
    struct uufile *     fd[MAX_UU_FD];
    int                 tids[MAX_UU_TID];
    int                 ntids;

    struct uufile *     cwd;

    int                 umask;

    int					capas; // Capabilities == rights

    // Process address space is mapped to our main linear adrr space
    // as follows. These adresses are used in syscall processing.
    // Any pointer from user process is first has mem_start added and
    // then checked to be in mem_start-mem_end bounds.

    void *				mem_start;		// where it starts in linear addr space
    void *				mem_end;		// first address above proc mem

	struct exe_module * em;
};

typedef struct uuprocess uuprocess_t;

uuprocess_t *uu_create_process(int ppid, struct exe_module *em);
void uu_proc_add_thread( uuprocess_t *p, int tid );


#endif // UUPROCESS_H

