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



//#include <phantom_libc.h>
#include <errno.h>
#include <setjmp.h>

//#include "misc.h"
#include "test.h"

//#include "svn_version.h"


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
    char buf[1024];

    if( !rc )
    {
        snprintf(buf, sizeof(buf), "USERMODE TEST PASSED: %s\n", test_name );
        printf( "%s", buf );
        ssyslog( 0, buf );
        return;
    }

    // todo strerror(rc)

    snprintf(buf, sizeof(buf), "!!! USERMODE TEST FAILED: %s -> %d\n", test_name, rc );
    printf( "%s", buf );
    ssyslog( 0, buf );
}












void run_test( const char *test_name, const char *test_parm )
{
    int all = 0 == strcmp(test_name, "all" );

    //printf("Phantom ver %s svn %s test suite\n-----\n", PHANTOM_VERSION_STR, svn_version() );
    printf("Phantom usermode test suite\n-----\n" );

    TEST(getters);

    //TEST(malloc);

    //TEST(udp_send);
    //TEST(udp_syslog);

    // These are long
    //TEST(timer);

    // must test after timed calls for it depends on them
    //TEST(ports);

    // These are very long, do 'em last
    //TEST(threads);


    printf("-----\nPhantom usermode test suite FINISHED\n" );

}
