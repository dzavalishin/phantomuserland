#include <user/sys_misc.h>
#include <user/sys_fio.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
    int i;

    if(argc < 1) {
        printf("usage: kill pid...\n");
        exit(0);
    }

    for(i=1; i<argc; i++)
        kill((int)atol(argv[i]));

    return 0;
}
