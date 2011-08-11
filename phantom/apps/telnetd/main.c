/*
 ** Copyright 2002, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */
//#include <sys/syscalls.h>
//#include <newos/errors.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
//#include <newos/tty_priv.h>

#include <user/sys_phantom.h>

#ifdef KERNEL
#warning kernel??
#endif

static int debug_fd; // debug spew to the console
static int tty_master_fd;
static int tty_slave_fd;
static int tty_num;
static int socket_fd;
//static sem_id wait_sem;

// XXX fix for big endian (move to libnet or whatever it's gonna be called)
static short ntohs(short value)
{
    return ((value>>8)&0xff) | ((value&0xff)<<8);
}

enum {
    SE = 240,
    SB = 250,
    WILL = 251,
    WONT,
    DO,
    DONT,
    IAC = 255
};

enum {
    OPT_ECHO = 1,
    OPT_SUPPRESS_GO_AHEAD = 3,
    OPT_NAWS = 31,
};

static void process_subblock(int sb_type, unsigned char *buf, int len)
{
    (void) buf;
    char temp[128];

    snprintf(temp, sizeof(temp), "process_subblock: type %d, len %d\r\n", sb_type, len);
    write(debug_fd, temp, strlen(temp));

#if 0
    if(sb_type == OPT_NAWS) {
        struct tty_winsize ws;

        ws.cols = ntohs(*(unsigned short *)&buf[0]);
        ws.rows = ntohs(*(unsigned short *)&buf[2]);
        sprintf(temp, "NAWS %d %d\r\n", ws.cols, ws.rows);
        write(debug_fd, temp, strlen(temp));

        ioctl(tty_master_fd, _TTY_IOCTL_SET_WINSIZE, &ws, sizeof(ws));
    }
#endif
}

static int telnet_reader(void *arg)
{
    (void) arg;

    unsigned char buf[4096];
    unsigned char sb[4096];
    ssize_t len;
    int i;
    enum {
        NORMAL = 0,
        SEEN_IAC,
        SEEN_OPT_NEGOTIATION,
        SEEN_SB,
        IN_SB,
    } state = NORMAL;
    int output_start, output_len;
    int curr_sb_type = 0;
    int sb_len = 0;
    char temp[128];

    for(;;) {
        /* read from the socket */
        len = read(socket_fd, buf, sizeof(buf));
        if(len <= 0)
            break;

        output_start = 0;
        output_len = 0;
        for(i = 0; i < len; i++) {
            // try to remove commands
            switch(state) {
            case NORMAL:
                if(buf[i] == IAC) {
                    state = SEEN_IAC;
                    if(output_len > 0)
                        write(tty_master_fd, &buf[output_start], output_len);
                    output_len = 0;
                    write(debug_fd, "NORMAL: IAC\r\n", strlen("NORMAL: IAC\r\n"));
                } else {
                    output_len++;
                }
                break;
            case SEEN_IAC:
                snprintf(temp, sizeof(temp), "SEEN_IAC: 0x%x\r\n", buf[i]);
                write(debug_fd, temp, strlen(temp));
                if(buf[i] == SB) {
                    state = SEEN_SB;
                } else if(buf[i] == IAC) {
                    output_start = i;
                    output_len = 1;
                    state = NORMAL;
                } else {
                    state = SEEN_OPT_NEGOTIATION;
                }
                break;
            case SEEN_OPT_NEGOTIATION:
                snprintf(temp, sizeof(temp), "SEEN_OPT_NEGOTIATION: 0x%x\r\n", buf[i]);
                write(debug_fd, temp, strlen(temp));
                // we can transition back to normal now, we've eaten this option
                state = NORMAL;
                output_len = 0;
                output_start = i+1;
                break;
            case SEEN_SB:
                snprintf(temp, sizeof(temp), "SEEN_SB: 0x%x\r\n", buf[i]);
                write(debug_fd, temp, strlen(temp));
                if(buf[i] == SE) {
                    state = NORMAL;
                    output_len = 0;
                    output_start = i+1;
                } else {
                    curr_sb_type = buf[i];
                    state = IN_SB;
                    sb_len = 0;
                }
                break;
            case IN_SB:
                snprintf(temp, sizeof(temp), "IN_SB: 0x%x\r\n", buf[i]);
                write(debug_fd, temp, strlen(temp));
                if(buf[i] == SE) {
                    state = NORMAL;
                    output_len = 0;
                    output_start = i+1;
                } else if(buf[i] == IAC) {
                    process_subblock(curr_sb_type, sb, sb_len);
                } else {
                    sb[sb_len++] = buf[i];
                }
                break;
            }
        }
        if(output_len > 0)
            write(tty_master_fd, &buf[output_start], output_len);
    }

    _kern_sem_release(wait_sem, 1);

    exit(0);
    return 0;
}

static int telnet_writer(void *arg)
{
    (void) arg;

    char buf[4096];
    ssize_t len;
    ssize_t write_len;

    for(;;) {
        /* read from the tty's master end */
        len = read(tty_master_fd, buf, sizeof(buf));
        if(len <= 0)
            break;

        write_len = write(socket_fd, buf, len);
        if(write_len < 0)
            break;
    }

    _kern_sem_release(wait_sem, 1);

    exit(0);
    return 0;
}

static int send_opts()
{
    char buf[4096];

    // negotiate the only options I care about
    buf[0] = IAC;
    buf[1] = WILL;
    buf[2] = OPT_ECHO;
    buf[3] = IAC;
    buf[4] = WILL;
    buf[5] = OPT_SUPPRESS_GO_AHEAD;
    buf[6] = IAC;
    buf[7] = DO;
    buf[8] = OPT_NAWS;

    return write(socket_fd, buf, 9);
}

static void sigchld_handler(int signal)
{
    (void) signal;
    _kern_sem_release(wait_sem, 1);
}

int main(int argc, char **argv)
{
    char **spawn_argv;
    int spawn_argc;
    int i;
    int pid;
    int tid;

    if(argc < 2) {
        printf("%s: not enough arguments\n", argv[0]);
        return -1;
    }

    // we're a session leader
    setsid();

    // build an array of args to pass anything we start up
    spawn_argc = argc - 1;
    spawn_argv = (char **)malloc(sizeof(char *) * spawn_argc);
    if(spawn_argv == NULL)
        return -1;
    for(i = 0; i < spawn_argc; i++) {
        spawn_argv[i] = argv[i + 1];
    }

    // register for SIGCHLD signals
    signal(SIGCHLD, &sigchld_handler);

    //	debug_fd = open("/dev/console", 0);
    debug_fd = -1;

    wait_sem = _kern_sem_create(0, "telnetd wait sem");
    if(wait_sem < 0)
        return -1;

    tty_master_fd = open("/dev/tty/master", 0);
    if(tty_master_fd < 0)
        return -2;

    tty_num = ioctl(tty_master_fd, _TTY_IOCTL_GET_TTY_NUM, NULL, 0);
    if(tty_num < 0)
        return -3;

#if 0
    {
        struct tty_flags flags;

        ioctl(tty_master_fd, _TTY_IOCTL_GET_TTY_FLAGS, &flags, sizeof(flags));
        flags.input_flags |= TTY_FLAG_CRNL;
        ioctl(tty_master_fd, _TTY_IOCTL_SET_TTY_FLAGS, &flags, sizeof(flags));
    }
#endif

    {
        char temp[128];
        snprintf(temp, sizeof(temp), "/dev/tty/slave/%d", tty_num);

        tty_slave_fd = open(temp, 0);
        if(tty_slave_fd < 0)
            return -4;
    }

    // move the stdin and stdout out of the way
    socket_fd = dup(0); // assume stdin, stdout, and stderr are the same socket
    close(0);
    close(1);
    close(2);

    // set the endpoints to the tty slave endpoint
    dup2(tty_slave_fd, 0);
    dup2(tty_slave_fd, 1);
    dup2(tty_slave_fd, 2);
    close(tty_slave_fd);

    // send some options over to the other side
    send_opts();

    // now start the app
    //pid = _kern_proc_create_proc(spawn_argv[0], spawn_argv[0], spawn_argv, spawn_argc, 5, PROC_FLAG_NEW_PGROUP);
    pid = phantom_run( spawn_argv[0], spawn_argv, 0, P_RUN_NEW_PGROUP );
    if(pid < 0)
        return -1;

    tid = _kern_thread_create_thread("telnet reader", &telnet_reader, NULL);
    //_kern_thread_set_priority(tid, 30);
    //_kern_thread_resume_thread(tid);

    tid = _kern_thread_create_thread("telnet writer", &telnet_writer, NULL);
    //_kern_thread_set_priority(tid, 30);
    //_kern_thread_resume_thread(tid);

    _kern_sem_acquire(wait_sem, 1);

    return 0;
}

