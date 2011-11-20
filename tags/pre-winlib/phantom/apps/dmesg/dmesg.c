#include <user/sys_misc.h>
#include <user/sys_fio.h>
#include <stdio.h>

// It's better to read kernel buf in one piece
char buf[32*1024];

void
cat(int fd)
{
    int n;

    while((n = read(fd, buf, sizeof(buf))) > 0)
        write(1, buf, n);

    if(n < 0)
    {
        printf("dmesg: read error\n");
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
    printf("kernel messages:\n");
    cat(fd);
    close(fd);
    printf("\n");

    return 0;
}
