// This one is compiled with compiler's headers

#if (defined(__MINGW64__) || defined(__MINGW32__))
#  define NO_NETWORK
#endif

//#  define NO_NETWORK

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <setjmp.h>

#include <errno.h>

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
//#define errno 32

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


#if defined(__CYGWIN__)

// for some reason .weak does not work in cygwin

asm("\
.globl _longjmp_machdep;\
    _longjmp_machdep: jmp _longjmp \
    ");

asm("\
.globl _setjmp_machdep;\
    _setjmp_machdep: jmp _setjmp \
    ");


#else

//void machdep_longjmp () __attribute__ ((weak, alias ("longjmp")));
//void machdep_setjmp () __attribute__ ((weak, alias ("setjmp")));


// freetype lib wants 'em in user mode too, provide hacks to call GCC libc ones

//#warning untested

asm("\
.weak _longjmp_machdep;\
.globl _longjmp_machdep;\
    _longjmp_machdep: jmp _longjmp \
    ");

asm("\
.weak _setjmp_machdep;\
.globl _setjmp_machdep;\
    _setjmp_machdep: jmp _setjmp \
    ");


//void longjmp_machdep ( void *jb, int a ){    __asm__("jmp __longjmp");}
//int setjmp_machdep ( void *jb ) {    __asm__("jmp __setjmp");}


#endif


#if 1

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define CURL_MAXBUF 512

/*
#define	ENOENT 2
#define	EIO    5
#define EINVAL 22
#define	ENOMEM 12
*/

int net_curl( const char *url, char *obuf, size_t obufsize, const char *headers )
{
    int nread = 0;

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons( 80 ); // HTTP

    int rc;
    char host[CURL_MAXBUF+1];
    char path[CURL_MAXBUF+1];

    int sock;

    printf( "\n\ncurl '%s'\n", url );

    if( strlen(url) > CURL_MAXBUF )
        return EINVAL;

    if( strncmp( url, "http://", 7 ) )
        return EINVAL;

    url += 7;

    char *pos = strchr( url, '/' );
    if( pos == 0 )
        return EINVAL;

    strlcpy( host, url, pos-url+1 );
    strlcpy( path, pos+1, CURL_MAXBUF );

    pos = strchr( host, ':' );
    if( pos != 0 )
    {
        *pos = '\0';
        int port = atoi( pos+1 );
        if( port )
            addr.sin_port = port;
    }

    printf( "curl host '%s' path '%s'\n", host, path );

    in_addr_t out;

    //rc = name2ip( &out, host, 0 );
    struct hostent *he = gethostbyname(host);
    if( he == 0 )
    {
        rc == ENOENT; //?
        printf( "can't resolve '%s', %d\n", host, rc );
        return rc;
    }

    //addr.sin_addr.s_addr = htonl( he->h_addr_list[0] );
    //addr.sin_addr.s_addr = he->h_addr_list[0];
    addr.sin_addr.s_addr = *(long *)(he->h_addr_list[0]);


    sock = socket(AF_INET , SOCK_STREAM , 0);
    if( sock < 0 )
    {
        printf( "can't open TCP socket\n" );
        return ENOMEM;
    }

/*
    struct timeval timeout;      
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        error("setsockopt failed\n");

    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        error("setsockopt failed\n");
*/
    //SHOW_FLOW( 0, "TCP - create socket to %d.%d.%d.%d port %d", ip0, ip1, ip2, ip3, port);

    printf( "TCP - will connect\n" );
    if( connect( sock, (void *)&addr, sizeof(addr)) )
    {
        printf( "can't connect to %s\n", host );
        goto err;
    }
    printf( "TCP - connected\n");

    char buf[1024*10];

    memset( buf, 0, sizeof(buf) );
    strlcpy( buf, "GET /", sizeof(buf) );
    strlcat( buf, path, sizeof(buf) );
    //rlcat( buf, "\r\n\r\n", sizeof(buf) );
    strlcat( buf, " HTTP/1.1\r\nHost: ", sizeof(buf) );
    strlcat( buf, host, sizeof(buf) );
    strlcat( buf, "\r\nUser-Agent: PhantomOSNetTest/0.1 (PhantomOS i686; ru)\r\nAccept: text/html,text/plain\r\nConnection: close\r\n", sizeof(buf) );

    if(headers)
        strlcat( buf, headers, sizeof(buf) );

    strlcat( buf, "\r\n", sizeof(buf) );

    //snprintf( buf, sizeof(buf), "GET / HTTP/1.1\r\nHost: ya.ru\r\nUser-Agent: PhantomOSNetTest/0.1 (PhanomOS i686; ru)\r\nAccept: text/html\r\nConnection: close\r\n\r\n" );
    int len = strlen(buf);
    int nwrite = write( sock, buf, len );
    printf( "TCP - write = %d, requested %d (%s)\n", nwrite, len, buf );
    if( nwrite != len ) goto err;

    memset( obuf, 0, obufsize );
    int bytes_recvd = 0;
    while(1)
    {
        nread = read( sock, buf, sizeof(buf)-1 ); // , &addr, SOCK_FLAG_TIMEOUT, 1000L*1000*50
        buf[sizeof(buf)-1] = 0;
        
        printf("TCP - read = %d\n", nread );

        if( nread == 0 ) break;
        if( nread < 0 )
        {
            printf("TCP - err = %d\n", nread );
            close(sock);
            return nread;
        }
        
        buf[nread] = 0;
        strlcat( obuf, buf, obufsize );
        bytes_recvd += nread;
    }

    //printf("TCP - recvd = %d (%s)\n", bytes_recvd, obuf );
err:
    close(sock);
/*
    if( nread <= 0 )
        return EIO;

    memset( obuf, 0, obufsize );
    len = nread;
    if( len > obufsize-1 ) len = obufsize-1;
    strncpy( obuf, buf, len );
*/
    return 0;
}

#endif

