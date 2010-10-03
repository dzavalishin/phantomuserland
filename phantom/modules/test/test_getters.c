#include <user/sys_getset.h>
#include <phantom_libc.h>

#include "test.h"

#define TEST_GET(name)  test_get( #name, name() )


#define TEST_SET(name)



void test_get( const char *name, int val )
{
    test_check_ge(val, 0); // not negative
    printf("%s = %d\n", name, val );
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
    TEST_GET(getpgid);
    TEST_GET(getpgrp);
    TEST_GET(getppid);
    TEST_GET(getegid);
    TEST_GET(getegid32);
    TEST_GET(geteuid);
    TEST_GET(geteuid32);
    TEST_GET(getgid);
    TEST_GET(getgid32);


    // Secod val is to set to
    TEST_SET(setgid);
    TEST_SET(setgid32);
    TEST_SET(setgroups);
    TEST_SET(setgroups32);
    TEST_SET(setpgid);
    TEST_SET(setregid);
    TEST_SET(setregid32);
    TEST_SET(setresgid);
    TEST_SET(setresgid32);
    TEST_SET(setresuid);
    TEST_SET(setresuid32);
    TEST_SET(setreuid);
    TEST_SET(setreuid32);
    TEST_SET(setuid);
    TEST_SET(setuid32);

	return 0;
}


