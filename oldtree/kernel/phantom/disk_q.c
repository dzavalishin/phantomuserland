/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Disk IO queue
 *
 * TODO Q sort?
 *
**/

#define DEBUG_MSG_PREFIX "disk_q"
#include <debug_ext.h>
#define debug_level_flow 1
#define debug_level_error 10
#define debug_level_info 10

#include <disk_q.h>
#include <malloc.h>
#include <stdio.h>

// really needs private - thread_unblock!
#include <thread_private.h>
//#include <threads.h>

#include <kernel/stats.h>


//static void dump_q(struct disk_q *q);

#define dump_q(___q)                                             \
    SHOW_FLOW( 9, "q %p magic %8x, dev %p unit %d, curr rq %p",  \
           (___q), q->struct_id, (___q)->device,                 \
           (___q)->unit,         (___q)->current                 \
          )



#define LOCK()  int ie = hal_save_cli(); hal_spin_lock(&(q->lock))
#define UNLOCK() ({ hal_spin_unlock(&(q->lock)); if(ie) hal_sti(); })




static void start_io(struct disk_q *q)
{
    SHOW_FLOW( 7, "requested to start io on q %p", q );
    dump_q(q);

    assert( q->struct_id == DISK_Q_STRUCT_ID );
    LOCK();

    //assert(q->current == 0);
    if(q->current != 0)
    {
        UNLOCK();
        return;
    }

    if(queue_empty(&(q->requests)))
    {
        UNLOCK();
        return;
    }

    queue_remove_first(&(q->requests), q->current, pager_io_request *, disk_chain);

    UNLOCK();

    SHOW_FLOW( 6, "really start io on q %p", q );
    q->startIo(q);
}


void
pager_io_request_done( pager_io_request *rq )
{
    // NB! Callback can overwrite our request, so take what we need now - CAN'T!
    char sleep = rq->flag_sleep;
    tid_t tid = rq->sleep_tid;
    pool_handle_t phandle = rq->phandle;

    int isWrite = rq->flag_pageout;

    STAT_INC_CNT_N( STAT_CNT_DISK_Q_SIZE, -1 );

#if PAGING_PARTITION
    rq->flag_pageout = 0;
    rq->flag_pagein  = 0;
#endif

    if(rq->pager_callback)
        rq->pager_callback( rq, isWrite );

    if( phandle >= 0 )
        dpart_release_async( phandle );


    // Prelim check
    if(sleep)
    {
        int ei = hal_save_cli();
        hal_spin_lock(&(rq->lock));

        // Locked check
        if(sleep)
            thread_unblock( get_thread( tid ), THREAD_SLEEP_IO );

        rq->flag_sleep = 0;
        hal_spin_unlock(&(rq->lock));
        if( ei ) hal_sti();
    }

}


static void interrupt(struct disk_q *q, errno_t rc)
{
    assert(q->current);

    SHOW_FLOW( 8, "interrupt on q %p, rc %d", q, rc );
    dump_q(q);
    if(rc)
    {
        q->current->flag_ioerror = 1;
        q->current->rc = rc;
    }

    pager_io_request *last = q->current;
    q->current = 0; // Atomic, I hope?

    start_io(q);
    pager_io_request_done( last );
}


static errno_t queueAsyncIo( struct phantom_disk_partition *p, pager_io_request *rq )
{
    struct disk_q *q = (struct disk_q*)p->specific;
    SHOW_FLOW( 7, "start io on q %p part %p, rq %p", q, p, rq );

    assert( q != 0 );
    assert( q->struct_id == DISK_Q_STRUCT_ID );

    dump_q(q);

    LOCK();

    if(rq->flag_urgent)
    {
        queue_enter_first(&(q->requests), rq, pager_io_request *, disk_chain);
    }
    else
    {
        queue_enter(&(q->requests), rq, pager_io_request *, disk_chain);
    }

    UNLOCK();

    start_io(q);
    return 0;
}

static errno_t queueDequeue( struct phantom_disk_partition *p, pager_io_request *rq )
{
    SHOW_FLOW( 7, "dequeue rq %p", rq );
    struct disk_q *q = (struct disk_q*)p->specific;

    assert( q != 0 );
    assert( q->struct_id == DISK_Q_STRUCT_ID );

    errno_t ret = 0;

    LOCK();
    dump_q(q);

    if( rq == q->current )
        ret = EBUSY;
    else
    {
        assert(!queue_empty(&(q->requests)));
        // TODO assert that block is really in q
        queue_remove( &(q->requests), rq, pager_io_request *, disk_chain);
        rq->flag_pageout = 0;
        rq->flag_pagein = 0;
    }

    UNLOCK();

    return ret;

}

static errno_t queueRaisePrio( struct phantom_disk_partition *p, pager_io_request *rq )
{
    SHOW_FLOW( 7, "dequeue rq %p", rq );
    struct disk_q *q = (struct disk_q*)p->specific;

    assert( q != 0 );
    assert( q->struct_id == DISK_Q_STRUCT_ID );

    errno_t ret = 0;

    LOCK();
    dump_q(q);

    if( rq == q->current )
        ret = EBUSY;
    else
    {
        assert(!queue_empty(&(q->requests)));
        // TODO assert that block is really in q
        queue_remove( &(q->requests), rq, pager_io_request *, disk_chain);
        queue_enter_first( &(q->requests), rq, pager_io_request *, disk_chain);
    }

    UNLOCK();

    return ret;
}

// TODO start timer (timed call) on start io, call driver's reset entry point on timeout

void phantom_init_disk_q(struct disk_q *q, void (*startIo)( struct disk_q *q ))
{
    q->struct_id = DISK_Q_STRUCT_ID;
    q->ioDone = interrupt;
    queue_init(&(q->requests));
    q->current = 0;
    q->startIo = startIo;
    hal_spin_init(&(q->lock));
    dump_q(q);
}




phantom_disk_partition_t *phantom_create_disk_partition_struct(long size, void *private, int unit, void (*startIoFunc)( struct disk_q *q ) )
{
    phantom_disk_partition_t * ret = phantom_create_partition_struct( 0, 0, size );

    ret->asyncIo = queueAsyncIo;
    ret->dequeue = queueDequeue;
    ret->raise = queueRaisePrio;
    ret->flags |= PART_FLAG_IS_WHOLE_DISK;

    struct disk_q *q = calloc( 1, sizeof(struct disk_q) );
    phantom_init_disk_q( q, startIoFunc );

    ret->specific = q;

    q->device = private;
    q->unit = unit; // if this is multi-unit device, let 'em distinguish

    // errno_t phantom_register_disk_drive(ret);

    dump_q(q);

    return ret;
}

#ifndef dump_q
static void dump_q(struct disk_q *q)
{
    SHOW_FLOW( 9, "q %p magic %8x, dev %p unit %d, curr rq %p",
           q, q->struct_id,
           q->device,
           q->unit,
           q->current
          );
}
#endif

