/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Test suit test selector
 *
 *
**/

#define DEBUG_MSG_PREFIX "test"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>

#include <phantom_libc.h>
#include <errno.h>
#include <setjmp.h>

#include <kernel/init.h>

#include "misc.h"
#include "test.h"

#include "svn_version.h"


static jmp_buf jb;
static int nFailed = 0;

void test_fail(errno_t rc)
{
    longjmp( jb, rc );
}

void test_fail_msg(errno_t rc, const char *msg)
{
    printf( "Test fail: %s\n", msg );
    longjmp( jb, rc );
}



/**
 *
 * Each test must report into the stdout one of:
 *
 * FAILED
 * PASSED
 * SKIPPED - this usually follows by PASSED, though.
 *
**/




#define TEST(name) \
    ({                                  		\
    int rc;                                             \
    if( ( rc = setjmp( jb )) )				\
    {                                                   \
        report( rc, #name );     			\
    }                                                   \
    else                                                \
    {                                                   \
        if( all || (0 == strcmp( test_name, #name )) )  \
        report( do_test_##name(test_parm), #name );     \
    }                                                   \
    })


void report( int rc, const char *test_name )
{
    if( !rc )
    {
        printf("KERNEL TEST PASSED: %s\n", test_name );
        return;
    }

    char rcs[128];
    strerror_r(rc, rcs, sizeof(rcs));

    nFailed++;
    printf("!!! KERNEL TEST FAILED: %s -> %d (%s)\n", test_name, rc, rcs );
}












void run_test( const char *test_name, const char *test_parm )
{
    int all = 0 == strcmp(test_name, "all" );

#ifndef ARCH_ia32
    printf("sleeping 20 sec");
    hal_sleep_msec(200000);
#endif

    printf("Phantom ver %s svn %s test suite\n-----\n", PHANTOM_VERSION_STR, svn_version() );

    TEST(rectangles);

    TEST(pool);

#ifdef ARCH_mips
//    TEST(sem);
    TEST(01_threads);
#endif

    TEST(physmem);
    TEST(physalloc_gen);
    TEST(malloc);
    TEST(amap);


    TEST(sem);

    TEST(cbuf);
    TEST(udp_send);
    TEST(udp_syslog);
    TEST(resolver);

    TEST(tftp);


    TEST(tcp_connect);


    // These are long
    TEST(dpc);
    TEST(timed_call);

    // must test after timed calls for it depends on them
    TEST(ports);


    // These are very long, do 'em last
    TEST(threads);

    TEST(absname);

#ifndef ARCH_ia32
//    TEST(sem);
    TEST(01_threads);
#endif


    //TEST(userland);

    printf("\n-----\n" );
    if(nFailed)
        printf("some tests FAILED\n" );
    else
        printf("all tests PASSED\n" );

    printf( "-----\nPhantom test suite FINISHED\n-----\n" );

}

