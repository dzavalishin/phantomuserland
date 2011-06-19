#include <setjmp.h>

#include <sys/unistd.h>
#include <sys/utsname.h>
#include <user/sys_port.h>
#include <user/sys_misc.h>
#include <user/sys_getset.h>

#include <testenv.h>


extern void exit(int);

static void test_connect();
static int do_test_usermode();


void report( int rc, const char *test_name )
{
    if( !rc )
    {
        printf("USERMODE TEST PASSED: %s\n", test_name );
        return;
    }

    printf("!!! USERMODE TEST FAILED: %s -> %d\n", test_name, rc );
    // todo strerror(rc)
}


static jmp_buf jb;

void test_fail(errno_t rc)
{
    longjmp( jb, rc );
}

void test_fail_msg(errno_t rc, const char *msg)
{
    printf( "Test fail: %s\n", msg );
    longjmp( jb, rc );
}

#define TEST(name) \
    ({                                  		\
    int rc;                                             \
    if( ( rc = setjmp( jb )) )				\
    {                                                   \
        report( rc, #name );     			\
    }                                                   \
    else                                                \
    {                                                   \
        report( do_test_##name(), #name );     \
    }                                                   \
    })





#define GET "GET /\n"

int
main(int ac, char **av, char **env)
{
    (void) ac;
    (void) av;
    (void) env;

    int pid = getpid();
    int tid = gettid();

    printf("UNIX userland regression test module runs with pid %d tid %d\n", pid, tid );

    char buf[1024];
    snprintf(buf, sizeof(buf), "UNIX userland regression test module runs with pid %d tid %d", pid, tid );
    ssyslog( 0, buf );


    // TODO turn back on
    // we hang on simult sleep from usermode and kernel!
    exit(0);

    /*
     int tcpfd = open("tcp://213.180.204.8:80", 0, 0 );

     printf("tcp fd = %d\n", tcpfd);

     write(tcpfd, GET, sizeof(GET));
     sleepmsec(4000);
     read(tcpfd, buf, 512);
     buf[512] = 0;
     printf("ya.ru: '%s'\n", buf );
     close(tcpfd);
     */
    // wait for kernel to be ready to test userland
    sleepmsec(20000);

    while(1)
    {
        test_connect();
        //ssyslog( 0, "module test is running" );
        sleepmsec(60000);
    }

    exit(0);
    asm("int $3");
}

#define BSIZE 256

void test_connect()
{
    int	id = find_port("__regress_test_port");
    if( id < 0 )
        return;

    ssyslog( 0, "userland regress test port found" );

    int rc;
    int32_t msg_code;
    char buf[BSIZE];

    rc = read_port(id, &msg_code, &buf, BSIZE);
    if( rc < 0 )
        return;

    rc = write_port(id, 0xAA, &buf, BSIZE);

    TEST(usermode);

    rc = write_port(id, 0x55, &buf, BSIZE);


    if( close_port( id ) )
    {
        ssyslog( 0, "can't close regress test port" );
    }
}




int do_test_usermode()
{
    char buf[BSIZE];
    int rc;

    test_check_eq(getpagesize(),4096);

    test_check_eq(personality(0xffffffff), 0);

    {
    struct utsname phantom_uname;
    test_check_eq(uname(&phantom_uname), 0);

    printf("uland: uname sys %s\n\tnode %s\n\trel %s\n\tver %s\n\tmachine %s\n\tdomain %s\n",
           phantom_uname.sysname,
           phantom_uname.nodename,
           phantom_uname.release,
           phantom_uname.version,
           phantom_uname.machine,
           phantom_uname.domainname
          );
    }

    rc = gethostname(buf, BSIZE);
    test_check_ge(rc,0);
    printf("uland: hostname %s\n", buf );


    return 0;
}



