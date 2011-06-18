#include <user/sys_misc.h>
#include <user/sys_fio.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int i;

    if(argc < 2){
        printf("Usage: mkdir files...\n");
        _exit(0);
    }

    for(i = 1; i < argc; i++){
        if(mkdir(argv[i]) < 0)
        {
            printf("mkdir: %s failed to create\n", argv[i]);
            break;
        }
    }

    return 0;
}
