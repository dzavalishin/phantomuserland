#include <sys/unistd.h>

extern void exit(int);

int
main(int ac, char **av, char **env)
{
    while(1)
    {
        ssyslog( 0, "module test is running" );
        sleepmsec(4000);
    }

    exit(0);
    asm("int $3");
}