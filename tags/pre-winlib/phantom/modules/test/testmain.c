#include <sys/unistd.h>
#include <user/sys_getset.h>
#include <phantom_libc.h>

#include "test.h"

extern void exit(int);


int main(int ac, char **av, char **env)
{
    (void) ac;
    (void) av;
    (void) env;

    ac--;
    av++;

    const char *test_name = "all";
    const char *test_parm = 0;

    if(ac)
    {
        test_name = *av++;
        ac--;
    }

    if(ac)
    {
        test_parm = *av++;
        ac--;
    }

    int pid = getpid();
    int tid = gettid();

    printf("Usermode test suite runs with pid %d tid %d\n", pid, tid );

    run_test( test_name, test_parm );


    exit(0);
}

