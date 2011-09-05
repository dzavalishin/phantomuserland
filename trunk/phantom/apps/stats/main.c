#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <user/sys_time.h>
#include <user/tmalloc.h>

#include <kernel/stats.h>


static const int sz = sizeof(struct kernel_stats);

static int gather_info(void)
{
    int fd = open("/proc/stats", 0, 0);
    if( fd < 0 )
    {
        perror("can't open /proc/stats");
        return -1;
    }

    printf("%32s curr/sec avg/sec total\n", "name" );

    // walk through each thread in the system
    for(;;)
    {
        struct kernel_stats ti;
        int rc = read( fd, &ti, sz );
        if( sz != rc )
            break;

        if(ti.name[0] == 0)
            continue;

        printf("%32s  %6d %6d %6d\n",
               ti.name,
               ti.current_per_second,
               ti.average_per_second,
               ti.total
              );
    }

    close(fd);

    return 0;
}

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    // TODO bring in good malloc/free and implement sbrk()!
    static char arena[1024*1024];
    init_malloc( arena, sizeof(arena) );

    int err;

    for(;;)
    {
        // gather data about each of the threads in the system
        err = gather_info();
        if(err < 0)
            return err;

        sleepmsec(1000);
    }
}


