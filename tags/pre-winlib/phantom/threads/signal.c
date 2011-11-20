#include <thread_private.h>


typedef void (*sig_func)(int sig_no);


sig_func signal( int sig, sig_func );

//void (*signal(int sig, void (*action)(int)))(int);


sig_func signal( int sig, sig_func f )
{

	return 0;
}












