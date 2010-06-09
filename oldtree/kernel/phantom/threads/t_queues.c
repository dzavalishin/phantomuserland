/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Run queues handling.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include <queue.h>
#include <hal.h>
#include <malloc.h>

#include "thread_private.h"


phantom_thread_t * t_dequeue_highest_prio(queue_head_t *queue)
{
    phantom_thread_t *nextt = 0;

#if 0
    queue_remove_first(queue, nextt, phantom_thread_t *, chain);
#else
    phantom_thread_t	*it;
    unsigned int  	max = 0;

    queue_iterate(queue, it, phantom_thread_t *, chain)
    {
        assert(it != GET_CURRENT_THREAD());

        if (it->priority > max)
        {
            max = it->priority;
            nextt  = it;
        }
    }

    if(nextt != 0)
        queue_remove(queue, nextt, phantom_thread_t *, chain);
#endif

    return nextt;
}


void
t_queue_check(queue_head_t *queue, phantom_thread_t *test)
{
    phantom_thread_t *next;

    queue_iterate(queue, next, phantom_thread_t *, chain)
        assert(next != test);
}

