/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Panic.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include <assert.h>
#include <stdarg.h>
#include <hal.h>
#include "thread_private.h"


int panic_reenter = 0;


void panic(const char *fmt, ...)
{
    if(panic_reenter)
        _exit(33);

    hal_cli();
    panic_reenter++;

    printf("Panic: ");
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    printf("\n");

    stack_dump();

    dump_thread_stacks();

    exit(33);
}





#if 0


void panic(const char *fmt, ...)
{
    if(panic_reenter)
        _exit(33);

    hal_cli();
    panic_reenter = 1;

    {
        va_list va;

        va_start(va, fmt);
        vprintf(fmt, va);
        va_end(va);
    }

    phantom_thread_t *t = GET_CURRENT_THREAD();

    int tid;
    for( tid = 0; tid < MAX_THREADS; tid++ )
    {
        phantom_thread_t *dt = phantom_kernel_threads[tid];
        if(dt != 0 && dt != t)
            dump_thread_stack(dt);
    }

    printf("Paniced tid:%p, ", t->tid);
    stack_dump();
    exit(1);
}

#endif
