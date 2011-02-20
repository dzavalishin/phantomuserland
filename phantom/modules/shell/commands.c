/*
 ** Copyright 2001, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <user/sys_fio.h>
#include <user/sys_phantom.h>
#include <user/sys_getset.h>
#include <user/sys_misc.h>

#include <phantom_types.h>
//#include <newos/tty_priv.h>
#include <sys/stat.h>

#include "commands.h"
#include "file_utils.h"
#include "shell_defs.h"

struct command cmds[] = {
    {"exec", &cmd_exec},
    {"stat", &cmd_stat},
    {"mkdir", &cmd_mkdir},
    {"cat", &cmd_cat},
    {"cd", &cmd_cd},
    {"pwd", &cmd_pwd},
    {"help", &cmd_help},
    {NULL, NULL}
};

int cmd_exec(int argc, const char *argv[])
{
    return cmd_create_proc(argc - 1, argv+1 );
}

int cmd_create_proc(int argc, const char *argv[])
{
    bool must_wait=1;
    pid_t pid;

    int  arg_len;
    char *tmp;
    char filename[SCAN_SIZE+1];

    if(argc  <1){
        printf("not enough args to exec\n");
        return 0;
    }

    tmp =  argv[argc - 1];

    if( !find_file_in_path(argv[0],filename,SCAN_SIZE)){
        printf("can't find '%s' \n",argv[0]);
        return 0;
    }


    // a hack to support the unix '&'
    if(argc >= 1) {
        arg_len = strlen(tmp);
        if(arg_len > 0){
            tmp += arg_len -1;
            if(*tmp == '&'){
                if(arg_len == 1){
                    argc --;
                } else {
                    *tmp = 0;
                }
                must_wait = 0;
            }
        }
    }


#if 1

	// HACK! unsure!
	argv[argc] = 0;

	printf("sh will run '%s'\n", filename);
    pid = phantom_run( filename, argv, 0, P_RUN_NEW_PGROUP | (must_wait ? P_RUN_WAIT : 0) );
    //pid = _kern_proc_create_proc(filename,filename, argv, argc, 5, PROC_FLAG_SUSPENDED|PROC_FLAG_NEW_PGROUP);

    if(pid >= 0) {
/*
        int retcode;

        if(must_wait) {
            //ioctl(0, _TTY_IOCTL_SET_PGRP, &pid, sizeof(pgrp_id));
            //setpgrp(pid);
            tcsetpgrp( 0, pid );

            //_kern_send_proc_signal(pid, SIGCONT);
            waitpid(pid, &retcode, 0);
            //pid = -1;
            //ioctl(0, _TTY_IOCTL_SET_PGRP, &pid, sizeof(pgrp_id));
            //setpgrp(-1);
            tcsetpgrp( 0, -1 );

        } else {
            //_kern_send_proc_signal(pid, SIGCONT);
        }
*/
    } else
#endif
    {
        printf("Error: cannot execute '%s', ret %d\n", filename, pid );
        return 0; // should be -1, but the shell would exit
    }

    return 0;
}

int cmd_mkdir(int argc, char *argv[])
{
    int rc;

    if(argc < 2) {
        printf("not enough arguments to mkdir\n");
        return 0;
    }

    //rc = _kern_mkdir(argv[1]);
    rc = mkdir(argv[1]);
    if (rc < 0) {
        printf("_kern_mkdir() returned error: %s\n", strerror(rc));
    } else {
        printf("%s successfully created.\n", argv[1]);
    }

    return 0;
}

int cmd_cat(int argc, char *argv[])
{
    int rc;
    int fd;
    char buf[257];

    if(argc < 2) {
        printf("not enough arguments to cat\n");
        return 0;
    }

    fd = open(argv[1], 0);
    if(fd < 0) {
        printf("cat: open() returned error: %s!\n", strerror(fd));
        goto done_cat;
    }

    for(;;) {
        rc = read(fd, buf, sizeof(buf) -1);
        if(rc <= 0)
            break;

        write(1, buf, rc);
    }
    close(fd);

done_cat:
    return 0;
}

int cmd_cd(int argc, char *argv[])
{
    int rc;

    if(argc < 2) {
        printf("not enough arguments to cd\n");
        return 0;
    }

    rc = chdir(argv[1]);
    if (rc < 0) {
        printf("cd: chdir() returned error: %s!\n", strerror(rc));
    }

    return 0;
}

int cmd_pwd(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    char buf[128];
    char *cwd;

    cwd= getcwd(buf, 128);
    if (cwd)
    {
        printf("pwd: cwd=\'%s\'\n", cwd);
    }
    else
    {
        printf("cd: getcwd() returned error: %s!\n", "xx"); //strerror(rc));
    }

    return 0;
}

int cmd_stat(int argc, char *argv[])
{
    int rc;
    struct stat istat;

    if(argc < 2) {
        printf("not enough arguments to stat\n");
        return 0;
    }

    rc = stat(argv[1], &istat);
    if(rc >= 0) {
        printf("stat of file '%s': \n", argv[1]);
        //printf("vnid 0x%x\n", (unsigned int)istat.vnid);
        printf("mode %o\n", istat.st_mode);
        printf("size %d\n", (int)istat.st_size);
    } else {
        printf("stat failed for file '%s'\n", argv[1]);
    }
    return 0;

}

int cmd_help(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    printf("command list:\n\n");
    printf("exit : quits this copy of the shell\n");
    printf("exec <file> : load this file as a binary and run it\n");
    printf("mkdir <path> : makes a directory at <path>\n");
    printf("cd <path> : sets the current working directory at <path>\n");
    printf("ls <path> : directory list of <path>. If no path given it lists the current dir\n");
    printf("stat <file> : gives detailed file statistics of <file>\n");
    printf("help : this command\n");
    printf("cat <file> : dumps the file to stdout\n");
    printf("mount <path> <device> <fsname> : tries to mount <device> at <path>\n");
    printf("unmount <path> : tries to unmount at <path>\n");

    return 0;
}


