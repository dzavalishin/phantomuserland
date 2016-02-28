#include <phantom_libc.h>
#include <sys/unistd.h>
#include <user/sys_fio.h>
#include <stdio.h>

#include "test.h"

#define GET "GET /\n"

static const char ya_addr[] = "tcp://87.250.250.3:80";
static const char ya_name[] = "tcp://ya.ru:80";


static void http_request( const char *host )
{
    printf("tcp test %s\n", host);

    char buf[1024];

    int tcpfd = open(host, 0, 0 );
    test_check_ge(tcpfd,0);

    printf("tcp fd (%s) = %d\n", host, tcpfd);
    if( tcpfd < 0 )
        exit(33);

    write(tcpfd, GET, sizeof(GET));
    sleepmsec(4000);

    int rc = read(tcpfd, buf, 512);
    if( rc <= 0 )
        printf("read rc = %d\n", rc );


    buf[512] = 0;
    printf("%s: '%s'\n", host, buf );
    close(tcpfd);
}


int do_test_tcpfs(const char *test_parm)
{
    (void) test_parm;

    http_request( ya_addr );
    http_request( ya_name );

    return 0;
}







