#ifndef UUPROCESS_H
#define UUPROCESS_H

struct uufile;
struct uutty;

#define MAX_UU_FD       256
#define MAX_UU_TID      128
#define MAX_UU_CMD      256

#define MAX_UU_PROC     2048

// pids before this are reserved
#define MIN_PID 10

struct uuprocess
{
	//hal_mutex_t		lock;

    int                 pid;
    int                 ppid;
    int                 pgrp_pid;
    int                 sess_pid;

    int                 uid, euid;
    int                 gid, egid;

    struct uutty *      ctty;

    char                cmd[MAX_UU_CMD];
    struct uufile *     fd[MAX_UU_FD];
    int                 tids[MAX_UU_TID];

    struct uufile *     cwd;

	// Process address space is mapped to our main linear adrr space
	// as follows. These adresses are used in syscall processing.
	// Any pointer from user process is first has mem_start added and
	// then checked to be in mem_start-mem_end bounds. 

	void *				mem_start;		// where it starts in linear addr space
	void *				mem_end;		// first address above proc mem
};

typedef struct uuprocess uuprocess_t;

uuprocess_t *uu_create_process(int ppid);


#endif // UUPROCESS_H

