// This one is compiled with compiler's headers

#if (defined(__MINGW64__) || defined(__MINGW32__))
#  define NO_NETWORK
#endif

#  define NO_NETWORK

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <setjmp.h>

//#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef NO_NETWORK
#  include <sys/socket.h> // breaks Travis CI compile
#  include <netinet/in.h>
#  include <arpa/inet.h> 
#endif

#include <pthread.h>


#include "winhal.h"



// -----------------------------------------------------------
// output
// -----------------------------------------------------------

static FILE *klog_f = 0;
static FILE *kout_f = 0;

void win_hal_open_kernel_log_file( const char *fn )
{
    klog_f = fopen( fn, "w" );
    if( 0 == klog_f )
    {
        fprintf( stderr, "Can't open kernel log file '%s'\n", fn );
        return;
    }

    FILE *rc = freopen( fn, "w", stderr );
    if( 0 == rc )
    {
        fprintf( stderr, "Can't reopen stdout for kernel log file '%s'\n", fn );
        return;
    }

}

void win_hal_open_kernel_out_file( const char *fn )
{
    kout_f = fopen( fn, "w" );
    if( 0 == kout_f )
        fprintf( stderr, "Can't open kernel console output file '%s'\n", fn );

    FILE *rc = freopen( fn, "w", stdout );
    if( 0 == rc )
    {
        fprintf( stderr, "Can't reopen stdout for kernel console output file '%s'\n", fn );
        return;
    }

}




static jmp_buf finish_gdb_socket;

int hal_printf(const char *fmt, ...)
{
    va_list ap;
    int retval;

    va_start(ap, fmt);

    if( 0 == klog_f )
        retval = vprintf(fmt, ap);
    else
        retval = vfprintf( klog_f, fmt, ap);

    va_end(ap);

    fflush(stdout);

    return (retval);
}


void debug_console_putc(int c)
{
    if( kout_f ) fputc( c, kout_f );
    else putchar(c);
}




//#include <kernel/debug.h>


// Print to log file only
void lprintf(char const *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if( klog_f )
        vfprintf( klog_f, fmt, ap);
    else
        vprintf(fmt, ap);
    va_end(ap);
}




// -----------------------------------------------------------
// 
// -----------------------------------------------------------



static void winhal_setport( int sock, int port )
{
#ifndef NO_NETWORK
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int rc = bind( sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in) );
    if( rc )
        perror("! bind");
#endif
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
    //printf("Debug server running\n");
#ifndef NO_NETWORK

    int ls = socket( PF_INET, SOCK_STREAM, 0);
    //int ls = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if( ls < 0 )
    {
        printf("! Debug server socket error\n");
        return;
    }

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

        //printf("Debug server accept\n");
        cfd = accept( ls, (struct sockaddr *) &client_addr, &clinetLen );
        if (cfd == -1)
        {
            perror("! Debug server accept error\n");
			close(ls);
            return;
        }
        printf("Debug server accepted from %s\n", inet_ntoa(client_addr.sin_addr) );

        //while(1)
        {
            //printf("Debug server read cmds\n");
            if(!setjmp(finish_gdb_socket))
                gdb_stub_handle_cmds(0, 0);
            printf("Debug server close connection\n");
        }

        close(cfd);

    }
#endif
}



int get_current_tid()
{
#if (defined(__MINGW64__) || defined(__MINGW32__))
    return (int)(pthread_self().p);
#else
    return (int)pthread_self();
    //return -1;
#endif
}

// TODO FIXME errno.h breaks compile on Linux :(
// EPIPE
#define errno 32

int k_open( int *fd, const char *name, int flags, int mode )
{
    int rc = open( name, O_RDWR, 0666 );
    if( rc < 0 )
        return errno;

    *fd = rc;
    return 0;
}


int k_read( int *nread, int fd, void *addr, int count )
{
    int rc = read( fd, addr, count );
    if( rc < 0 )
        return errno;
    if(nread) *nread = rc;
    return 0;
}


int k_write( int *nwritten, int fd, const void *addr, int count )
{
    int rc = write( fd, addr, count );
    if( rc < 0 )
        return errno;
    if(nwritten) *nwritten = rc;
    return 0;
}



int k_seek( int *pos, int fd, int offset, int whence )
{
    off_t rc = lseek( fd, offset, whence );
    if( rc < 0 )
        return errno;
    if( pos ) *pos = rc; // TODO var size?
    return 0;
}


/*

  can't be implemmnted here - we're unable to bring both Phantom and host OS struct stat in one C file
  and move fields by hand.

int k_stat( const char *path, struct stat *data, int statlink )
{

}
*/

int k_stat_size( const char *path, unsigned int *size ) // just get file size
{
    struct stat sb;
    int rc = stat( path, &sb );
    if( rc ) return rc;

    *size = sb.st_size;

    return 0;
}


int k_close( int fd )
{
    int rc = close( fd );
    if( rc ) return errno;
    return 0;
}



//void machdep_longjmp () __attribute__ ((weak, alias ("longjmp")));
//void machdep_setjmp () __attribute__ ((weak, alias ("setjmp")));


// freetype lib wants 'em in user mode too, provide hacks to call GCC libc ones

//#warning untested

asm(".globl _longjmp_machdep;\
    _longjmp_machdep: jmp _longjmp \
    ");

asm(".globl _setjmp_machdep;\
    _setjmp_machdep: jmp _setjmp \
    ");


//void longjmp_machdep ( void *jb, int a ){    __asm__("jmp __longjmp");}
//int setjmp_machdep ( void *jb ) {    __asm__("jmp __setjmp");}








