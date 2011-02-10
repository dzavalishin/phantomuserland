#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

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

    printf("Got sig, depth = %d\n", depth++);
    while(1)
    {
        printf("Sleep in catcher\n");
        sleep(10);
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




int main( int ac, char **av )
{

    catch_sig();
    set_timer();
    while(1)
    {
        printf("Sleep in main\n");
        sleep(10);
    }

    return 0;
}



