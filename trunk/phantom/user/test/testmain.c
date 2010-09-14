#include <sys/unistd.h>

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

	printf("printf: test module runs with pid %d tid %d\n", pid, tid );

	char buf[1024];
	snprintf(buf, sizeof(buf), "syslog: test module runs with pid %d tid %d", pid, tid );
    ssyslog( 0, buf );

	int tcpfd = open("tcp://213.180.204.8:80", 0, 0 );

	printf("tcp fd = %d\n", tcpfd);

	write(tcpfd, GET, sizeof(GET));
    sleepmsec(4000);
	read(tcpfd, buf, 512);
	buf[512] = 0;
	printf("ya.ru: '%s'\n", buf );
	close(tcpfd);


    while(1)
    {
        ssyslog( 0, "module test is running" );
        sleepmsec(4000);
    }

    exit(0);
    asm("int $3");
}