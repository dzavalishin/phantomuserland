/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests. TODO!
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include "thread_private.h"
#include <hal.h>
#include <phantom_libc.h>

hal_cond_t c;
hal_mutex_t m;

void YIELD();
void pressEnter(char *text);

int inmutex = 0;
void checkEnterMutex()
{
    inmutex++;
    if(inmutex > 1)
        panic("reentered mutex");
}

void checkLeaveMutex() { inmutex--; }


void t_wait(void *a)
{
    char *name = a;
    while(1)
    {

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
}


int counter = 0;
void t1(void *a)
{
    char *name = a;
    while(1)
    {
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
}



void threads_test()
{

    hal_cond_init(&c, "test");
    hal_mutex_init(&m, "test");

    pressEnter("will create thread");
    phantom_create_thread( t1, "__T1__", 0 );
    phantom_create_thread( t1, "__T2__", 0 );
    //phantom_create_thread( t1, "__T3__" );

    //phantom_create_thread( t_wait, "__TW__" );
    int tid = hal_start_kernel_thread_arg( t_wait, "__TW__" );
    hal_set_thread_priority( tid, THREAD_PRIO_HIGH );

    while(1)
    {
        pressEnter("will yield");
        YIELD();

        printf("!! back in main\n");
    }

}


void alarm_to_intr()
{
    printf("\n(alarm do time)\n");
    phantom_scheduler_time_interrupt();

#if USE_FORK_LUKE
    if( GET_CURRENT_THREAD()->thread_flags & THREAD_FLAG_PREFORK )
    {
        printf("\n(alarm do fork)\n");
        phantom_thread_in_interrupt_fork();
        return;
    }
#endif

    if( phantom_scheduler_is_reschedule_needed() )
    {
        printf("\n(alarm do switch)\n");
        phantom_scheduler_soft_interrupt();
    }
}



