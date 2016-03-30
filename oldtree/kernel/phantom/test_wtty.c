/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests - wtty
 *
 *
**/

#define DEBUG_MSG_PREFIX "test"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>
#include <kernel/sem.h>

#include <phantom_libc.h>
#include <errno.h>
#include "test.h"

#include <wtty.h>
#include <threads.h>


// TODO test sleep on putc/getc
// TODO test loosing char on putc_nowait and full buff

#define MAXTB 1024
static char buf[MAXTB];
static char lastrc = 0;
//static char lastwc = 0;

static hal_sem_t thread_start;


static void test_rd( wtty_t *w, int cnt, bool nowait )
{
    int tries = 0;
    char rbuf;

    while(cnt>0)
    {
        int rc = wtty_read(w, &rbuf, 1, 1);
        if( nowait && rc != 1 )
            test_fail_msg( -1, "no char in wtty" );

        if( rc != 1 )
        {
            if( tries++ > 100 )
                test_fail_msg( -1, "no char in wtty, tired" );
            hal_sleep_msec(1);
            continue;
        }


        cnt--;

        if( rbuf != lastrc )
        {
            printf( "char value error (%d, need %d)", rbuf, lastrc );
            test_fail( -1 );
        }
        lastrc++;
    }
}

static volatile int wtty_w_thread_runs = 0;
static void wtty_w_thread(void *a)
{
    lastrc = 0;
    wtty_w_thread_runs = 1;
    hal_sem_acquire( &thread_start );

    wtty_t *w = a;
    int rc = wtty_write(w, buf, MAXTB, 0);
    test_check_eq( rc, MAXTB );

    wtty_w_thread_runs = 0;
}

static volatile int wtty_r_thread_runs = 0;
static void wtty_r_thread(void *a)
{
    lastrc = 0;
    wtty_r_thread_runs = 1;
    hal_sem_acquire( &thread_start );

    char rbuf[MAXTB];

    wtty_t *w = a;
    int rc = wtty_read(w, rbuf, MAXTB, 0);
    test_check_eq( rc, MAXTB );
    // TODO check data

    wtty_r_thread_runs = 0;
}



static void _wtty_test(wtty_t *w)
{
    int rc = wtty_write(w, buf, 32, 1);
    test_check_eq( rc, 32 );
    test_rd( w, 32, 1 );

    // wr thread

    SHOW_FLOW0( 1, "start write thread" );
    hal_start_kernel_thread_arg( wtty_w_thread, w );

    while(!wtty_w_thread_runs) hal_sleep_msec(10);
    test_check_true(wtty_w_thread_runs);
    hal_sem_release( &thread_start );

    SHOW_FLOW0( 1, "start read check" );
    test_rd( w, MAXTB, 0 );

    hal_sleep_msec(100);
    test_check_false(wtty_w_thread_runs);

    // rd thread

    SHOW_FLOW0( 1, "start read thread" );
    hal_start_kernel_thread_arg( wtty_r_thread, w );

    while(!wtty_r_thread_runs) hal_sleep_msec(10);
    test_check_true(wtty_r_thread_runs);
    hal_sem_release( &thread_start );

    SHOW_FLOW0( 1, "start write check" );
    
    rc = wtty_write(w, buf, MAXTB, 0);
    test_check_eq( rc, MAXTB );

    hal_sleep_msec(100);
    test_check_false(wtty_r_thread_runs);


}


static void fail_dump( void *arg )
{
    wtty_t *w = arg;
    wtty_dump( w );
}

int do_test_wtty(const char *test_parm)
{
    int i;

    (void) test_parm;

    for( i = 0; i < MAXTB; i++ )
        buf[i] = i;

    wtty_t *w = wtty_init();
    hal_sem_init( &thread_start, "wtty_test" );

    on_fail_call( fail_dump, w );
    _wtty_test(w);

    hal_sem_destroy( &thread_start );
    wtty_destroy(w);
    return 0;
}


