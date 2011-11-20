#include <user/sys_getset.h>
#include <phantom_libc.h>

#include "test.h"

#define TEST_GET(name)  test_get( #name, name() )


#define TEST_SET(name,val) test_set( #name, name(val), val )



void test_get( const char *name, int val )
{
    //test_check_ge(val, 0); // not negative
    printf("%s = %d\n", name, val );
    if( val < 0 )
        test_fail_msg(EINVAL, name);
}


void test_set( const char *name, int ret, int sent )
{
    printf("%s( %2d ) = %d\n", name, sent, ret );
    if( ret < 0 )
        test_fail_msg(EINVAL, name);
}

int do_test_getters(const char *test_parm)
{
	(void)test_parm;

    //SYSCALL(getpagesize);

    //SYSCALL(personality);



    TEST_GET(gettid);
    TEST_GET(getpid);

    TEST_GET(getuid);
    TEST_GET(getuid32);

    TEST_GET(getpgrp);
    TEST_GET(getppid);

    TEST_GET(getegid);
    TEST_GET(getegid32);

    TEST_GET(geteuid);
    TEST_GET(geteuid32);

    TEST_GET(getgid);
    TEST_GET(getgid32);

	test_get( "getpgid", getpgid(getpid()) );


	return 0;
}

int do_test_setters(const char *test_parm)
{
	(void)test_parm;

    int uid = getuid();
    int gid = getgid();

    // Second val is to set to
    TEST_SET(setuid,        uid         );
    TEST_SET(setuid32,      uid         );

    TEST_SET(setgid,        gid         );
    TEST_SET(setgid32,      gid         );
/* unimpl in kernel yet
    TEST_SET(setregid,      gid         );
    TEST_SET(setregid32,    gid         );
    TEST_SET(setresgid,     gid         );
    TEST_SET(setresgid32,   gid         );

    TEST_SET(setresuid,     uid         );
    TEST_SET(setresuid32,   uid         );
    TEST_SET(setreuid,      uid         );
    TEST_SET(setreuid32,    uid         );
*/
    //TEST_SET(setpgid);

    //TEST_SET(setgroups);
    //TEST_SET(setgroups32);

	return 0;
}



