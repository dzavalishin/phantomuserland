#include <sys/unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <user/sys_getset.h>
#include <user/sys_fio.h>
#include <stdio.h>

//extern void exit(int);

#define ECHO_PORT 7

int main(int ac, char **av, char **env)
{
    (void) ac;
    (void) av;
    (void) env;

    int rc;

    int pid = getpid();
    int tid = gettid();

    printf("tcp_echo runs with pid %d tid %d\n", pid, tid );

    char buf[1024];
    //snprintf(buf, sizeof(buf), "syslog: test module runs with pid %d tid %d", pid, tid );
    //ssyslog( 0, buf );

    int tries = 0;
    while(tries++ < 100)
    {
        int lsock = socket( PF_INET, SOCK_DGRAM, 0 );

        printf("lsock = %d\n", lsock );
        if( lsock < 0 )
        {
            continue;
        }

        struct sockaddr_in sa;
        bzero( &sa, sizeof( sa ) );
        sa.sin_len = sizeof( sa );
        sa.sin_family = PF_INET;
        sa.sin_port = htons(ECHO_PORT); // TODO check if we have to convert it here
        sa.sin_addr.s_addr = INADDR_ANY;
        rc = bind( lsock, (const struct sockaddr *)&sa, sizeof(sa) );

        while(1)
        {
            struct sockaddr ra;
            socklen_t alen = sizeof(ra);

            ssize_t nr = recvfrom( lsock, buf, 512, 0, &ra, &alen );
            //printf("read = %d\n", nr );
            if( nr <= 0 )
            {
                break;
            }

            if( nr > 0 )
            {
                //ssize_t nw = 
                sendto( lsock, buf, nr, 0, &ra, alen );
                //if( (nw != nr) || (nw <= 0) )                    printf("write = %d\n", nr );
            }
        }

        close(lsock);
    }

    exit(0);
}
