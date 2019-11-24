

struct phantom_mutex_impl
{
    pthread_mutex_t m;
    const char *name;
    int lock;
};
