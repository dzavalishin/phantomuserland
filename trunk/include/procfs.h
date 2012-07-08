/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * /proc fs io structs.
 *
 *
**/

typedef struct proc_fs_process
{
    int                 pid; 
    int                 ppid;
    int                 pgrp_pid;
    int                 sess_pid;

    int                 uid, euid;
    int                 gid, egid;

    char                cmd[MAX_UU_CMD];

    int                 ntids;

    char                cwd_path[FS_MAX_PATH_LEN];

    int                 umask;

    int                 capas; // Capabilities == rights

    size_t		mem_size;

} proc_fs_process_t;


// Not used yet!
typedef struct proc_fs_thread
{
    int                 tid;

} proc_fs_thread_t;

