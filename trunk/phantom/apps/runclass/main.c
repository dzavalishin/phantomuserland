#include <user/sys_misc.h>
#include <user/sys_phantom.h>
//#include <user/sys_fio.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        printf("Usage: runclass .phantom.class.name method_no\n");
        return 1;
    }

    int m = atoi(argv[2]);

    return phantom_runclass( argv[1], m, 0 );
}

