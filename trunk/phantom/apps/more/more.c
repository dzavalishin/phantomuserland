#include <user/sys_misc.h>
#include <user/sys_fio.h>
#include <stdio.h>
#include <sys/fcntl.h>

char msg[] = "more:";
char msg1[] = "\r      \r";

char buf[512];
int confd;

#define MAXL 5

void
more(int fd)
{
    int n, l;

    l = 0;

    while((n = read(fd, buf, sizeof(buf))) > 0)
    {
        char *bp = buf;

        while( n > 0 )
        {
            char *nl = index( bp, '\n' );
            if( nl == 0 )
                break;

            write(1, bp, nl-bp);
            bp = nl;

            l++;
            if( l > MAXL )
            {
                char tmp;
                write( confd, msg, sizeof(msg) );
                read( confd, &tmp, 1);
                write( confd, msg1, sizeof(msg1) );
                l = 0;
            }

        }

        write(1, bp, n);
    }

    if(n < 0)
    {
        printf("more: read error\n");
        _exit(0);
    }
}

int
main(int argc, char *argv[])
{
    int fd, i;

    confd = open( "/dev/tty", O_RDWR );
    if(confd < 0){
        printf("more: cannot open tty\n");
        _exit(0);
    }

    if(argc <= 1)
    {
        more(0);
        _exit(0);
    }

    for(i = 1; i < argc; i++){
        if((fd = open(argv[i], 0)) < 0){
            printf("more: cannot open %s\n", argv[i]);
            _exit(0);
        }
        more(fd);
        close(fd);
    }

    return 0;
}
