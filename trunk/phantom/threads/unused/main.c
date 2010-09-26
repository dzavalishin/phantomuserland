/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Test main. won't work, I believe.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include <signal.h>
#include <stdio.h>
#include <sys/time.h>

//#define SIGALRM 14

#define PREEMPT 0

#if PREEMPT
void YIELD() {}
#else
void YIELD() { phantom_scheduler_yield(); }
#endif




void pressEnter(char *text)
{
#if 1
    printf("%s\n", text);
#if !PREEMPT
    printf("press Enter...\n...", text);
    while( getchar() >= ' ' )
        ;
#else
    sleep(1);
#endif
#endif

}




static int ie = 1;

/*
void ararm_handler()
{
    if(ie) {
        //printf("\n(alarm)\n");
#if PREEMPT
        alarm_to_intr();
#endif
    }
    //else        printf("\n(DI alarm)\n");

    signal(SIGALRM, ararm_handler);
    alarm(5);

    //printf("\n(alarm ret)\n");
}
*/


#define SIG SIGALRM
//#define SIG SIGVTALRM

void catch_sig(void);


void unblock_sig()
{
    sigset_t            sigs;

    sigemptyset(&sigs);

    sigaddset(&sigs, SIG);
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);

}

void set_timer()
{
    struct itimerval it;

    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 1000L*500;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 1000L*500;
    //setitimer( ITIMER_VIRTUAL, &it, NULL);
    setitimer( ITIMER_REAL, &it, NULL);

}

void sig_catcher(int sig)
{
    static int depth = 0;
    unblock_sig();

    catch_sig();

    /*
    printf("Got sig, depth = %d\n", depth++);
    while(1)
    {
        printf("Sleep in catcher\n");
        sleep(10);
    }
    */

    if(ie) {
        //printf("\n(alarm)\n");
#if PREEMPT
        alarm_to_intr();
#endif
    }


}


void catch_sig()
{
    struct sigaction a;

    a.sa_handler = sig_catcher;
    a.sa_flags = SA_NODEFER|SA_RESETHAND;

    sigaction( SIG, &a, 0 );
    //signal( SIG, sig_catcher );
    //alarm(2);
}





int main()
{
    setvbuf(stdin, 0, _IONBF, 0 );

    printf("Testing threads lib\n");


    printf("Do phantom_threads_init()\n");
    phantom_threads_init();
    //pressEnter("Done phantom_threads_init()");

#if PREEMPT
    //signal(SIGALRM, ararm_handler);
    //alarm(3);

    catch_sig();
    set_timer();

#endif

    threads_test();

}





void phantom_scheduler_request_soft_irq()
{
#if PREEMPT
    raise(SIGALRM);
#else
    phantom_thread_switch();
#endif
}



void        hal_cli()
{
    ie = 0;
}

void        hal_sti()
{
    ie = 1;
}

int hal_is_sti()
{
    return ie;
}

int hal_save_cli()
{
    int ret = ie;
    ie = 0;
    return ret;
}




void console_set_error_color() {}

void console_set_normal_color() {}



