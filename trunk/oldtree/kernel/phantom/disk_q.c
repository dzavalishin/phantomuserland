#include "disk_q.h"
#include <malloc.h>

// really needs private - thread_unblock!
#include <thread_private.h>
//#include <threads.h>


#if IO_RQ_SLEEP
//static void awake(int tid);
// returns tid
//static int putAsleep();
#endif


#define LOCK()  int ie = hal_save_cli(); hal_spin_lock(&(q->lock))
#define UNLOCK() ({ hal_spin_unlock(&(q->lock)); if(ie) hal_sti(); })




static void start_io(struct disk_q *q)
{
    assert( q->struct_id == DISK_Q_STRUCT_ID );
    LOCK();

    assert(q->current == 0);

    if(queue_empty(&(q->requests)))
    {
        UNLOCK();
        return;
    }

    queue_remove_first(&(q->requests), q->current, pager_io_request *, disk_chain);

    UNLOCK();
    q->startIo(q);
}


void
pager_io_request_done( pager_io_request *rq )
{
    // NB! Callback can overwrite our request, so take what we need now
#if IO_RQ_SLEEP
    //char sleep = me->flag_sleep;
    //int tid = me->sleep_tid;
#endif

    if(rq->pager_callback)
        rq->pager_callback( rq, rq->flag_pageout );

#if IO_RQ_SLEEP
    //if(sleep)        awake(tid);

    // Prelim check
    if(rq->flag_sleep)
    {
        int ei = hal_save_cli();
        hal_spin_lock(&(rq->lock));

        // Locked check
        if(rq->flag_sleep)
            thread_unblock( get_thread( rq->sleep_tid ), THREAD_SLEEP_IO );

        rq->flag_sleep = 0;
        hal_spin_unlock(&(rq->lock));
        if( ei ) hal_sti();
    }

#endif

}


static void interrupt(struct disk_q *q, errno_t rc)
{
    if(rc)
    {
        q->current->flag_ioerror = 1;
        q->current->rc = rc;
    }

    pager_io_request *last = q->current;
    q->current = 0; // Atomic, I hope?

    start_io(q);
    pager_io_request_done( last );

/*
    // NB! Callback can overwrite our request, so take what we need now
#if IO_RQ_SLEEP
    char sleep = last->flag_sleep;
    int tid = last->sleep_tid;
#endif

    if(last->pager_callback)
        last->pager_callback( last, last->flag_pageout );

#if IO_RQ_SLEEP
    if(sleep)
        awake(tid);
#endif
*/
}


static errno_t queueAsyncIo( struct phantom_disk_partition *p, pager_io_request *rq )
{
    struct disk_q *q = (struct disk_q*)p->specific;

    assert( q != 0 );
    assert( q->struct_id == DISK_Q_STRUCT_ID );

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

#if IO_RQ_SLEEP
    //if(rq->flag_sleep)        rq->sleep_tid = putAsleep();
#endif

    start_io(q);
    return 0;
}

void phantom_init_disk_q(struct disk_q *q, void (*startIo)( struct disk_q *q ))
{
    q->struct_id = DISK_Q_STRUCT_ID;
    q->ioDone = interrupt;
    queue_init(&(q->requests));
    q->current = 0;
    q->startIo = startIo;
    hal_spin_init(&(q->lock));
}




phantom_disk_partition_t *phantom_create_disk_partition_struct(long size, void *private, int unit, void (*startIoFunc)( struct disk_q *q ) )
{
    phantom_disk_partition_t * ret = phantom_create_partition_struct( 0, 0, size);

    ret->asyncIo = queueAsyncIo;
    ret->flags |= PART_FLAG_IS_WHOLE_DISK;


    struct disk_q *q = calloc( 1, sizeof(struct disk_q) );
    phantom_init_disk_q( q, startIoFunc );

    ret->specific = q;

    q->device = private;
    q->unit = unit; // if this is multi-unit device, let 'em distinguish

    // errno_t phantom_register_disk_drive(ret);


    return ret;
}





/*
#if IO_RQ_SLEEP

static void awake(int tid)
{
    phantom_thread_t *t = get_thread(tid);
    thread_unblock( t, THREAD_SLEEP_IO );
}


// returns tid
static int putAsleep()
{
    thread_block( THREAD_SLEEP_IO, 0 );
}
#endif
*/





