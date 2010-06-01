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


#include "config.h"
#include <phantom_libc.h>
#include <errno.h>
#include "test.h"

#include "threads/thread_private.h"
#include <hal.h>

static volatile int thread_activity_counter = 0;
static volatile int thread_stop_request = 0;

static hal_cond_t c;
static hal_mutex_t m;

//static void YIELD();
#define YIELD() hal_sleep_msec(400)

// TODO call kill thread when implemented
#define FINISH() do { hal_sleep_msec(1000); } while(1)


static void pressEnter(char *text)
{
    printf("%s\n", text);
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


static void t_wait(void *a)
{
    char *name = a;
    while(!thread_stop_request)
    {
        thread_activity_counter++;

        printf("--- thread %s will wait 4 cond ---\n", name);

        hal_mutex_lock(&m);
        checkEnterMutex();
        checkLeaveMutex();
        hal_cond_wait(&c, &m);
        checkEnterMutex();
        checkLeaveMutex();
        hal_mutex_unlock(&m);

        printf("--- thread %s runs ---\n", name);
        //pressEnter("--- thread a runs ---\n");
        YIELD();
    }
    FINISH();
}


static int counter = 0;
void t1(void *a)
{
    char *name = a;
    while(!thread_stop_request)
    {
        thread_activity_counter++;

        printf("--- thread %s runs ---\n", name);
        pressEnter("");

        printf("Will lock mutex\n");
        hal_mutex_lock(&m);
        printf("locked mutex\n");
        checkEnterMutex();
        YIELD();
        YIELD();
        printf("Will unlock mutex\n");
        checkLeaveMutex();
        hal_mutex_unlock(&m);
        printf("unlocked mutex\n");


        counter++;
        if(counter >7)
        {
            counter = 0;
            printf("Will signal cond\n");
            hal_cond_signal(&c);
            printf("Signalled cond\n");
        }
        YIELD();
        
    }
    FINISH();
}



static errno_t threads_test()
{

    hal_cond_init(&c, "threadTest");
    hal_mutex_init(&m, "threadTest");

    pressEnter("will create thread");
    phantom_create_thread( t1, "__T1__", 0 );
    phantom_create_thread( t1, "__T2__", 0 );
    //phantom_create_thread( t1, "__T3__" );

    //phantom_create_thread( t_wait, "__TW__" );
    int tid = hal_start_kernel_thread_arg( t_wait, "__TW__" );
    hal_set_thread_priority( tid, THREAD_PRIO_HIGH );

    int i = 40;
    while(i-- > 0)
    {
        pressEnter("will yield");
        YIELD();

        printf("!! back in main\n");
    }


    // TODO use thread kill func
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

