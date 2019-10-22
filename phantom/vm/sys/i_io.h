#ifndef _I_IO
#define _I_IO


#define MAX_FILENAME_LEN 128

struct data_area_4_io
{
    int32_t        fd;
    pvm_object_t   name; // name we got for open() - for restart
};

/*

#define IO_DA_BUFSIZE 4


struct data_area_4_io
{
    u_int32_t                           in_count;       // num of objects waiting for get
    u_int32_t                           out_count;      // num of objects put and not processed by kernel

    // Buffers are small and we don't bother with cycle, just
    // move contents. Input is on the right (higher index) side.
    pvm_object_t                        ibuf[IO_DA_BUFSIZE];
    pvm_object_t                        obuf[IO_DA_BUFSIZE];

    pvm_object_t                        in_sleep_chain;  // Threads sleeping for input
    pvm_object_t                        out_sleep_chain; // Threads sleeping for output

    u_int32_t                           in_sleep_count;  // n of threads sleeping for input
    u_int32_t                           out_sleep_count; // n of threads sleeping for output

#if WEAKREF_SPIN
    hal_spinlock_t                      lock;
#else
#error mutex?
    hal_mutex_t                         mutex;
    //pvm_object_t              mutex;          // persistence-compatible mutex
#endif

    u_int32_t                           reset;          // not operational, unblock waiting threads
};


 */



#endif // _I_IO
