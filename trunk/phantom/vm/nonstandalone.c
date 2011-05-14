// This one is compiled with compiler's headers

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <setjmp.h>

#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "winhal.h"


static jmp_buf finish_gdb_socket;

int hal_printf(const char *fmt, ...)
{
    va_list ap;
    int retval;

    va_start(ap, fmt);
    retval = vprintf(fmt, ap);
    va_end(ap);

    fflush(stdout);

    return (retval);
}

static void winhal_setport( int sock, int port )
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int rc = bind( sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in) );
    if( rc )
        perror("! bind");
}

static int cfd = -1;

int putDebugChar(char c)    /* write a single character      */
{
    //putchar(c);
    int rc = write( cfd, &c, 1 );
    return rc;
}

extern char getDebugChar(void)     /* read and return a single char */
{
    char c;
    int rc = read( cfd, &c, 1 );

    //if( rc > 0 ) putchar(c);

    if( rc < 0 ) 
        perror("gdb sock read");

    if( rc < 0 )
    {
        longjmp(finish_gdb_socket, 1);
    }

    return (rc < 1) ? -1 : c;
}

void gdb_stub_handle_cmds(void *da, int signal);


void winhal_debug_srv_thread(int *arg)
{
    (void) arg;
    printf("Debug server running\n");

    int ls = socket( PF_INET, SOCK_STREAM, 0);
    //int ls = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);

    winhal_setport( ls, 1256 );

    printf("Debug server listen\n");
    if(listen(ls,1))
    {
        printf("! Debug server listen error\n");
        return;
    }

    while(1)
    {
        struct sockaddr_in client_addr;
        unsigned int clinetLen = sizeof(client_addr);

        printf("Debug server accept\n");
        cfd = accept( ls, (struct sockaddr *) &client_addr, &clinetLen );
        if (cfd == -1)
        {
            perror("! Debug server accept error\n");
            return;
        }
        printf("Debug server accepted from %s\n", inet_ntoa(client_addr.sin_addr) );

        //while(1)
        {
            printf("Debug server read cmds\n");
            if(!setjmp(finish_gdb_socket))
                gdb_stub_handle_cmds(0, 0);
            printf("Debug server finished read cmds\n");
        }

        close(cfd);

    }

}



