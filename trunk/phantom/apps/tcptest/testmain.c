#include <sys/unistd.h>
#include <user/sys_getset.h>
#include <user/sys_fio.h>
#include <stdio.h>

extern void exit(int);

#define GET "GET /\n"

int
    main(int ac, char **av, char **env)
{
    (void) ac;
    (void) av;
    (void) env;

    int pid = getpid();
    int tid = gettid();

    printf("printf: tcptest runs with pid %d tid %d\n", pid, tid );

    char buf[1024];
    snprintf(buf, sizeof(buf), "syslog: test module runs with pid %d tid %d", pid, tid );
    ssyslog( 0, buf );

    int tries = 0;
    while(tries++ < 1000)
    {
        snprintf(buf, sizeof(buf), "tcp pid %d try %d", pid, tries );
        ssyslog( 0, buf );

        int tcpfd = open("tcp://87.250.250.3:80", 0, 0 );

        printf("tcp fd = %d\n", tcpfd);

        write(tcpfd, GET, sizeof(GET));
        //sleepmsec(4000);
        //sleepmsec(100);
        read(tcpfd, buf, 512);
        buf[512] = 0;
        printf("ya.ru: '%s'\n", buf );
        close(tcpfd);
    }

    exit(0);
}
