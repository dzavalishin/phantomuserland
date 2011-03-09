/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Stack dump.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include "thread_private.h"
#include <phantom_libc.h>




void dump_thread_stack(phantom_thread_t *t)
{
    void *ebp = (void *)t->cpu.ebp;
    printf("Thread %d EIP 0x%08X, ", t->tid, t->cpu.eip);
    stack_dump_from(ebp);
}

void dump_thread_info(phantom_thread_t *t)
{
    printf("T%2d pri %02d blk %08X ",
           t->tid,
           t->priority,
           t->sleep_flags
          );

    if(t->sleep_flags & THREAD_SLEEP_MUTEX)
        printf("mutx %8p", t->waitmutex );

    if(t->sleep_flags & THREAD_SLEEP_COND)
        printf("cond %8p", t->waitcond );

    if(t->sleep_flags & THREAD_SLEEP_SEM)
        printf("sema %8p", t->waitsem );





    printf("\n");
}

void dump_thread_stacks()
{
    int i;
    for( i = 0; i < MAX_THREADS; i++ )
    {
        if( phantom_kernel_threads[i] == 0 )
            continue;
        phantom_thread_t *t = phantom_kernel_threads[i];

        dump_thread_info(t);
        dump_thread_stack(t);
    }
}


void phantom_dump_threads()
{
    int i;
    for( i = 0; i < MAX_THREADS; i++ )
    {
        if( phantom_kernel_threads[i] == 0 )
            continue;

        phantom_thread_t *t = phantom_kernel_threads[i];
        dump_thread_info(t);
    }
}








static int dump_thread_info_buf(char *buf, int len, phantom_thread_t *t)
{
    int rc, ret = 0;

    *buf = '\0';

    const char *slp = "?";
    const char *scol = "";

    switch(t->sleep_flags)
    {
    case 0:                     slp = " RUN "; scol = "\x1b[32m"; break;
    case THREAD_SLEEP_USER:     slp = "user"; break;
    case THREAD_SLEEP_SLEEP:    slp = "sleep"; scol = "\x1b[34m"; break;
    case THREAD_SLEEP_COND:     slp = "cond"; break;
    case THREAD_SLEEP_MUTEX:    slp = "mutex"; break;
    case THREAD_SLEEP_SEM:      slp = "sema"; break;
    case THREAD_SLEEP_LOCKED:   slp = "lock"; break;
    case THREAD_SLEEP_IO:       slp = "io"; scol = "\x1b[33m"; break;
    };

    rc = snprintf(buf, len, " %2d %02d %s%-5.5s %d %-10.10s \x1b[37m",
                  t->tid,
                  t->priority,
                  scol, slp,
                  t->cpu_id,
                  t->name ? t->name : "--"
          );

    len -= rc;
    ret += rc;
    buf += rc;

    if(t->sleep_flags & THREAD_SLEEP_MUTEX)
    {
        rc = snprintf(buf, len, "%8p %s",
                      t->waitmutex, t->waitmutex->impl->name );
        len -= rc;
        ret += rc;
        buf += rc;
    }

    if(t->sleep_flags & THREAD_SLEEP_COND)
    {
        rc = snprintf(buf, len, "%8p %s",
                      t->waitcond, t->waitcond->impl->name );
        len -= rc;
        ret += rc;
        buf += rc;
    }

#if USE_NEW_SEMAS
    if(t->sleep_flags & THREAD_SLEEP_SEM)
    {
        rc = snprintf(buf, len, "%8p %s",
                      t->waitsem, t->waitsem->impl->name );
        len -= rc;
        ret += rc;
        buf += rc;
    }
#endif

    if( len > 0 )
    {
        *buf++ = '\n';
        len--;
        ret++;
    }

    return ret;
}

void phantom_dump_threads_buf(char *buf, int len)
{
    int rc;
    rc = snprintf(buf, len, " Id Pr State CPU Name      Locked\n");
    buf += rc;
    len -= rc;


    int i;
    for( i = 0; i < MAX_THREADS; i++ )
    {
        // BUG! Races with thread kill code
        if( phantom_kernel_threads[i] == 0 )
            continue;

        phantom_thread_t t = *phantom_kernel_threads[i];

        if( t.sleep_flags & THREAD_SLEEP_ZOMBIE )
            continue;

        rc = dump_thread_info_buf(buf,len,&t);
        buf += rc;
        len -= rc;
    }

    if(len--)
        *buf++ = 0;
}

