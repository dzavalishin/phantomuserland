#ifndef EXCEPTIONS4C
#  error i am just for e4c
#endif

#include <threads.h>
#include <kernel/mutex.h>


typedef tid_t pthread_t; // funny

#define THREAD_TYPE pthread_t

#define pthread_self() get_current_tid()
#define pthread_equal(__t1, __t2) ((__t1)==(__t2))
#define pthread_cancel(__t) t_kill_thread(__t)
#define pthread_exit(_rc) hal_exit_kernel_thread()

#define PTHREAD_MUTEX_INITIALIZER { 0 }
//#define PTHREAD_MUTEX_INITIALIZER { errno_t rc = hal_mutex_init( &m, const char *name);)
 




typedef struct hal_mutex pthread_mutex_t;
#define pthread_mutex_lock(__m) hal_mutex_lock(__m)
#define pthread_mutex_unlock(__m) hal_mutex_unlock(__m)


#define errno 0

#define HAVE_SNPRINTF
