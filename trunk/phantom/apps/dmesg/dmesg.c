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

#define DMESG_F "/proc/dmesg"

int
main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    int fd;

    if((fd = open( DMESG_F, 0)) < 0)
    {
        printf("dmesg: cannot open %s\n", DMESG_F );
        _exit(0);
    }
    cat(fd);
    close(fd);

    return 0;
}
