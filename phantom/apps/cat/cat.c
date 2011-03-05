#include <user/sys_misc.h>
#include <user/sys_fio.h>
#include <stdio.h>

char buf[512];

void
cat(int fd)
{
    int n;

    while((n = read(fd, buf, sizeof(buf))) > 0)
        write(1, buf, n);
    if(n < 0){
        printf("cat: read error\n");
        _exit(0);
    }
}

int
main(int argc, char *argv[])
{
    int fd, i;

    if(argc <= 1)
    {
        cat(0);
        _exit(0);
    }

    for(i = 1; i < argc; i++){
        if((fd = open(argv[i], 0)) < 0){
            printf("cat: cannot open %s\n", argv[i]);
            _exit(0);
        }
        cat(fd);
        close(fd);
    }

    return 0;
}
