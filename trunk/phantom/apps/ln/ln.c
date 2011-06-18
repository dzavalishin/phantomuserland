#include <user/sys_misc.h>
#include <user/sys_fio.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    if(argc != 3){
        printf( "Usage: ln old new\n");
        exit(0);
    }
    if(link(argv[1], argv[2]) < 0)
        printf( "link %s %s: failed\n", argv[1], argv[2]);

    return 0;
}
