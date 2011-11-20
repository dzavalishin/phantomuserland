/*
 ** Copyright 2002, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */
#include <compat/newos.h>
#include <newos/err.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <thread_private.h>
#include <user/sys_time.h>
#include <user/tmalloc.h>

typedef struct thread_time {
    struct thread_time *next;

    //struct thread_info info;
    struct phantom_thread info;

    bigtime_t last_user_time;
    bigtime_t last_kernel_time;

    bigtime_t delta_user_time;
    bigtime_t delta_kernel_time;

    bool touched;
} thread_time;

static thread_time *times = NULL;

static thread_time *find_in_list(tid_t tid)
{
    thread_time *tt;

    for(tt = times; tt; tt = tt->next) {
        if(tid == tt->info.tid)
            return tt;
    }
    return NULL;
}

static void insert_in_list(thread_time *tt)
{
    tt->next = times;
    times = tt;
}

static void prune_untouched(void)
{
    thread_time *tt, *last;

    last = NULL;
    tt = times;
    while(tt) {
        thread_time *temp;
        if(!tt->touched) {
            // prune this entry
            if(!last)
                times = tt->next;
            else
                last->next = tt->next;
            temp = tt;
            tt = tt->next;
            free(temp);
        } else {
            tt->touched = false;
            last = tt;
            tt = tt->next;
        }
    }
}

// unused
#if 0
static void sort_times(void)
{
    thread_time *curr, *next, *last;
    bool done;

    do {
        done = true;
        last = NULL;
        curr = times;
        while(curr) {
            next = curr->next;
            if(!next)
                break;

            if((curr->delta_user_time + curr->delta_kernel_time) > (next->delta_user_time + next->delta_kernel_time)) {
                done = false;

                // swap the two
                if(last)
                    times = next;
                else
                    last->next = next;
                curr->next = next->next;
                next->next = curr;
            }
            last = curr;
            curr = curr->next;
        }
    } while(!done);
}
#endif

static const int sz = sizeof(struct phantom_thread);

static int gather_info(void)
{
    //struct proc_info pi;
    //struct thread_info ti;
    thread_time *tt;
    //int err;

    int fd = open("/proc/threads", 0, 0);
    if( fd < 0 )
        return ERR_GENERAL;

    // walk through each thread in the system
    for(;;) {
        struct phantom_thread ti;
        int rc = read( fd, &ti, sz );
        if( sz != rc )
        {
            break;
        }

        tt = find_in_list(ti.tid);
        if(!tt) {
            // create a new one
            tt = malloc(sizeof(thread_time));
            if(!tt)
                return ERR_NO_MEMORY;

            memset(&tt->info, 0, sizeof(struct phantom_thread));

            insert_in_list(tt);
        }

        // save the last user and kernel time
        //tt->last_user_time = tt->info.user_time;
        //tt->last_kernel_time = tt->info.kernel_time;

        // save the new data
        memcpy(&tt->info, &ti, sizeof(ti));

        // calculate the delta time
        //tt->delta_user_time = tt->info.user_time - tt->last_user_time;
        //tt->delta_kernel_time = tt->info.kernel_time - tt->last_kernel_time;

        tt->touched = true;
    }

    close(fd);

    // prune any untouched entries
    prune_untouched();

    // sort the entries
    //	sort_times();

    return 0;
}

static int display_info(void)
{
    thread_time *tt;
    bigtime_t total_user = 0;
    bigtime_t total_kernel = 0;

    // clear and home the screen
    //printf("%c[2J%c[H", 0x1b, 0x1b);

    // print the thread dump
    printf("  tid   cpu   pri    usertime  kerneltime                             name\n");
    for(tt = times; tt; tt = tt->next) {
        printf("%5d %5d %5d %11d %11d %32s\n",
               tt->info.tid, tt->info.cpu_id, tt->info.priority, 0, 0, /*tt->info.name*/ "" );
        total_user += tt->delta_user_time;
        total_kernel += tt->delta_kernel_time;
    }

    //printf("%18s%12Ld%12Ld\n", "total:", total_user, total_kernel);

    return 0;
}

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    // TODO bring in good malloc/free and implement sbrk()!
    //static char arena[1024*1024];
    //init_malloc( arena, sizeof(arena) );

    int err;

    for(;;) {
        // gather data about each of the threads in the system
        err = gather_info();
        if(err < 0)
            return err;

        err = display_info();
        if(err < 0)
            return err;

        sleepmsec(1000);
    }
}


