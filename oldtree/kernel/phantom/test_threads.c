/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests - network
 *
 *
**/

#define DEBUG_MSG_PREFIX "test"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>
#include <phantom_libc.h>
#include <errno.h>
#include "test.h"

#include <threads.h>
#include <thread_private.h>

#include <hal.h>
#include <kernel/timedcall.h>

#define TEST_CHATTY 0


static volatile int thread_activity_counter = 0;
static volatile int thread_stop_request = 0;

static hal_cond_t c;
static hal_mutex_t m;
static hal_sem_t s;

//static void YIELD();
//#define YIELD() hal_sleep_msec(400)
#define YIELD() hal_sleep_msec(40)

// TODO call kill thread when implemented
//#define FINISH() do { hal_sleep_msec(1000); } while(1)
#define FINISH()


static void pressEnter(char *text)
{
    if(TEST_CHATTY) printf("%s\n", text);
}

static int inmutex = 0;
static void checkEnterMutex()
{
    inmutex++;
    if(inmutex > 1)
    {
        SHOW_ERROR0( 0, "mutex reentered");
        test_fail( -1 );
    }
}

static void checkLeaveMutex() { inmutex--; }


static void t_empty(void *a)
{
    (void) a;
    hal_sleep_msec( random() % 6000 );
    // TODO do some random things, such as sleep, set timer, etc
}


static void t_wait(void *a)
{
    hal_set_current_thread_priority( THREAD_PRIO_HIGH );

    char *name = a;
    while(!thread_stop_request)
    {
        thread_activity_counter++;

        if(TEST_CHATTY) printf("--- thread %s will wait 4 cond ---\n", name);

        hal_mutex_lock(&m);
        checkEnterMutex();
        checkLeaveMutex();
        hal_cond_wait(&c, &m);
        checkEnterMutex();
        checkLeaveMutex();
        hal_mutex_unlock(&m);

        if(TEST_CHATTY) printf("--- thread %s runs ---\n", name);
        //pressEnter("--- thread a runs ---\n");
        YIELD();
    }
    FINISH();
}


static int counter = 0;
static void thread1(void *a)
{
    char *name = a;
    while(!thread_stop_request)
    {
        thread_activity_counter++;

        if(TEST_CHATTY) printf("--- thread %s runs ---\n", name);
        pressEnter("");

        if(TEST_CHATTY) printf("Will lock mutex\n");
        hal_mutex_lock(&m);
        if(TEST_CHATTY) printf("locked mutex\n");
        checkEnterMutex();
        YIELD();
        if(TEST_CHATTY) printf("Will unlock mutex\n");
        checkLeaveMutex();
        hal_mutex_unlock(&m);
        if(TEST_CHATTY) printf("unlocked mutex\n");

        if( random() & 1 )
            hal_sem_acquire( &s );

        counter++;
        if(counter >7)
        {
            counter = 0;
            if(TEST_CHATTY) printf("Will signal cond\n");
            hal_cond_signal(&c);
            if(TEST_CHATTY) printf("Signalled cond\n");
        }
        YIELD();
        
    }
    FINISH();
}

/*
static void t_sem_signal(void *a)
{
    (void) a;
    while( !thread_stop_request )
    {
        hal_sleep_msec( 350 );
        hal_sem_release( &s );
    }
}
*/

static errno_t threads_test()
{

    hal_cond_init(&c, "threadTest");
    hal_mutex_init(&m, "threadTest");
    hal_sem_init(&s, "threadTest");

    int i = 40;
    while(i-- > 0)
        phantom_create_thread( t_empty, "Empty", 0 );

    pressEnter("will create thread");
    phantom_create_thread( thread1, "__T1__", 0 );
    phantom_create_thread( thread1, "__T2__", 0 );
    //phantom_create_thread( thread1, "__T3__" );

    //phantom_create_thread( t_wait, "__TW__" );
    int tid = hal_start_kernel_thread_arg( t_wait, "__TW__" );

    i = 40;
    while(i-- > 0)
    {
        if(TEST_CHATTY) pressEnter("will yield");
        YIELD();

        if(TEST_CHATTY) printf("!! back in main\n");
    }

    t_kill_thread( tid );
    hal_sleep_msec( 30 );

    thread_stop_request = 1;
    hal_sleep_msec( 10 );

    thread_activity_counter = 0;
    hal_sleep_msec( 1000 );
    if( thread_activity_counter )
    {
        SHOW_ERROR0( 0, "Can't stop thread" );
        return -1;
    }

    return 0;
}

// TODO check that we passed after cond wait on permission
// TODO timeouts, incl manual awake of timed cond and sema
// TODO semas

int do_test_threads(const char *test_parm)
{
    (void) test_parm;

    return threads_test();
}



// -----------------------------------------------------------------------
// DPC
// -----------------------------------------------------------------------





#include "dpc.h"


static dpc_request         dpc1;
static dpc_request         dpc2;

#define DPC_ARG1 "xyz_1"
#define DPC_ARG2 "2_xyz"

static int      dpc1_triggered = 0;
static int      dpc2_triggered = 0;

static void dpc_serve1( void *arg )
{
    if( strcmp( arg, DPC_ARG1 ) )
    {
        SHOW_ERROR0( 0, "DPC 1 arg is wrong" );
        test_fail( -1 );
    }

    dpc1_triggered = 1;
    hal_sleep_msec(2000);
}

static void dpc_serve2( void *arg )
{
    if( strcmp( arg, DPC_ARG2 ) )
    {
        SHOW_ERROR0( 0, "DPC 2 arg is wrong" );
        test_fail( -1 );
    }
    dpc2_triggered = 1;
}


// TODO need more sleeping DPCs to test that DPC engine runs additional threads as needed


int do_test_dpc(const char *test_parm)
{
    (void) test_parm;

    int rc = 0;

    dpc_request_init( &dpc1, dpc_serve1 );
    dpc_request_init( &dpc2, dpc_serve2 );

    // DPC engine runs next thread only after a second
    hal_sleep_msec(2000);


    dpc_request_trigger( &dpc1, DPC_ARG1 );
    hal_sleep_msec(200);
    dpc_request_trigger( &dpc2, DPC_ARG2 );
    hal_sleep_msec(200);

    if( !dpc1_triggered )
    {
        rc = -1;
        SHOW_ERROR0( 0, "DPC 1 lost" );
    }

    if( !dpc2_triggered )
    {
        rc = -1;
        SHOW_ERROR0( 0, "DPC 2 lost" );
    }

    dpc1_triggered = 0;
    dpc2_triggered = 0;

    hal_sleep_msec(200);

    if( dpc1_triggered )
    {
        rc = -1;
        SHOW_ERROR0( 0, "DPC 1 sporadic" );
    }

    if( dpc2_triggered )
    {
        rc = -1;
        SHOW_ERROR0( 0, "DPC 2 sporadic" );
    }

    dpc_request_trigger( &dpc2, DPC_ARG2 );

    hal_sleep_msec(200);

    if( dpc1_triggered )
    {
        rc = -1;
        SHOW_ERROR0( 0, "DPC 1 sporadic after DPC 2 trigger" );
    }

    if( !dpc2_triggered )
    {
        rc = -1;
        SHOW_ERROR0( 0, "DPC 2 lost 2" );
    }

    return rc;
}










// -----------------------------------------------------------------------
// Timed calls
// -----------------------------------------------------------------------


#define DUMPQ 0



static volatile int called = 0;

static void echo(  void *_a )
{
    called++;
    printf("Echo: '%s'\n", _a);
}

static timedcall_t     t1 = { echo, "hello 5", 		   5,	0, 0, { 0, 0 }, 0 };
static timedcall_t     t2 = { echo, "hello 100",         100,	0, 0, { 0, 0 }, 0 };
static timedcall_t     t3 = { echo, "hello 2000", 	2000,	0, 0, { 0, 0 }, 0 };
static timedcall_t     t4 = { echo, "hello 10 000",    10000,	0, 0, { 0, 0 }, 0 };

static char *msg = "timed func 5000";

int do_test_timed_call(const char *test_parm)
{
    (void) test_parm;


    printf("Testing timed call undo, must be no echoes:\n");
    called = 0;

    phantom_request_timed_call( &t2, 0 );
    phantom_undo_timed_call(&t2);

    hal_sleep_msec(200); // Twice the time
    test_check_eq(called, 0);


#if DUMPQ
    //dump_timed_call_queue();
#endif


    called = 0;

    printf("Testing timed calls, wait for echoes:\n");

    phantom_request_timed_call( &t1, 0 );
    //test_check_false(called); // it is still possible for this test to fail with correct code!
    phantom_request_timed_call( &t2, 0 );
#if DUMPQ
    //dump_timed_call_queue();
#endif
    phantom_request_timed_call( &t3, 0 );
    phantom_request_timed_call( &t4, 0 );

    phantom_request_timed_func( echo, msg, 5000, 0 );

#if DUMPQ
    dump_timed_call_queue();
#endif

    // We check for >= (ge) because hal_sleep... can cleep for more than asked, and timed
    // calls are usually quite on time

    hal_sleep_msec(6);
    test_check_ge(called, 1);

    hal_sleep_msec(200); // Have lag of 106 msec, OK?
    test_check_ge(called, 2);

    hal_sleep_msec(2000-100); // Still have lag of 106 msec
    test_check_ge(called, 3);

    hal_sleep_msec(5000-2000); // Have lag of 206 msec
    test_check_ge(called, 4);

    hal_sleep_msec(2500+10000-5000-2000); // Strange - need quite big lag here...
    test_check_ge(called, 5);



    printf("Done testing timed calls\n");
    return 0;
}







// -----------------------------------------------------------------------
// Semaphores
// -----------------------------------------------------------------------

static hal_sem_t 	ts;
static volatile int 	stop_sem_test = 0;
static volatile int 	sem_released = 0;
static int 		softirq = -1;

static void sem_rel(void *a)
{
    (void) a;
    hal_sleep_msec( 300 );
    printf("sema release 1 (direct)\n");
    sem_released = 1;
    hal_sem_release( &ts );

    hal_sleep_msec( 300 );
    printf("sema release 2 (softirq %d)\n", softirq );
    sem_released = 1;
    hal_request_softirq( softirq );

    /*
    while(!stop_sem_test)
    {
        hal_sleep_msec( 500 );
    }

    */
}


static void sem_softirq(void *a)
{
    (void) a;
    printf("sema softirq\n");
    hal_sleep_msec( 10 );
    sem_released = 1;
    hal_sem_release( &ts );
}


int do_test_sem(const char *test_parm)
{
    (void) test_parm;
    printf("Testing semaphores\n");

    hal_sem_init(&ts, "semTest");



    softirq = hal_alloc_softirq();
    if( softirq < 0 )
        test_fail_msg( 1, "Unable to get softirq" );

    hal_set_softirq_handler( softirq, sem_softirq, 0 );

    //int tid =
    hal_start_kernel_thread_arg( sem_rel, 0 );

    printf("sema wait 1\n");

    // Direct
    sem_released = 0;
    hal_sem_acquire( &ts );
    test_check_eq(sem_released,1);

    hal_sleep_msec( 100 );

    printf("sema wait 2\n");

    // Softirq
    sem_released = 0;
    hal_sem_acquire( &ts );
    test_check_eq(sem_released,1);

    stop_sem_test = 1;

    printf("Done testing semaphores\n");
    return 0;

}














