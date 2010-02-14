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
};

typedef struct uuprocess uuprocess_t;

uuprocess_t *uu_create_process(int ppid);

