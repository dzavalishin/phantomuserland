#include <sys/unistd.h>

extern void exit(int);

int
main(int ac, char **av, char **env)
{
	int pid = getpid();
	int tid = gettid();

	printf("printf: test module runs with pid %d tid %d\n", pid, tid );

	char buf[1024];
	snprintf(buf, sizeof(buf), "syslog: test module runs with pid %d tid %d", pid, tid );
    ssyslog( 0, buf );

    while(1)
    {
        ssyslog( 0, "module test is running" );
        sleepmsec(4000);
    }

    exit(0);
    asm("int $3");
}