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
    printf("Thread %d EIP 0x%08X, ", t->cpu.eip);
    stack_dump_ebp(ebp);
}

void dump_thread_info(phantom_thread_t *t)
{
    printf("T%2d pri %02d blk %08X ",
           t->tid,
           t->priority,
           t->sleep_flags
          );

    if(t->sleep_flags & THREAD_SLEEP_MUTEX)
        printf("mutx %08X", t->waitmutex );

    if(t->sleep_flags & THREAD_SLEEP_COND)
        printf("cond %08X", t->waitcond );

    if(t->sleep_flags & THREAD_SLEEP_SEM)
        printf("sema %08X", t->waitsem );





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

    const char *slp = "?";
    switch(t->sleep_flags)
    {
    case 0:                     slp = "_run_"; break;
    case THREAD_SLEEP_USER:     slp = "USER"; break;
    case THREAD_SLEEP_SLEEP:    slp = "SLEEP"; break;
    case THREAD_SLEEP_COND:     slp = "COND"; break;
    case THREAD_SLEEP_MUTEX:    slp = "MUTEX"; break;
    case THREAD_SLEEP_SEM:      slp = "SEMA"; break;
    case THREAD_SLEEP_LOCKED:   slp = "LOCK"; break;
    case THREAD_SLEEP_IO:       slp = "IO"; break;
    };

    rc = snprintf(buf, len, " %2d %02d %-5.5s %-10.10s ",
                  t->tid,
                  t->priority,
                  slp,
                  t->name
          );

    len -= rc;
    ret += rc;
    buf += rc;

    if(t->sleep_flags & THREAD_SLEEP_MUTEX)
    {
        rc = snprintf(buf, len, "%08X %s",
                      t->waitmutex, t->waitmutex->impl->name );
        len -= rc;
        ret += rc;
        buf += rc;
    }

    if(t->sleep_flags & THREAD_SLEEP_COND)
    {
        rc = snprintf(buf, len, "%08X %s",
                      t->waitcond, t->waitcond->impl->name );
        len -= rc;
        ret += rc;
        buf += rc;
    }

#if USE_NEW_SEMAS
    if(t->sleep_flags & THREAD_SLEEP_SEM)
    {
        rc = snprintf(buf, len, "%08X %s",
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
    rc = snprintf(buf, len, " Id Pr State Name         Locked\n");
    buf += rc;
    len -= rc;


    int i;
    for( i = 0; i < MAX_THREADS; i++ )
    {
        if( phantom_kernel_threads[i] == 0 )
            continue;

        phantom_thread_t *t = phantom_kernel_threads[i];
        rc = dump_thread_info_buf(buf,len,t);
        buf += rc;
        len -= rc;
    }

    if(len--)
        *buf++ = 0;
}

