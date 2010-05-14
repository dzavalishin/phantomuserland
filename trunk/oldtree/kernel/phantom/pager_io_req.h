/**
 *
 * $Log: pager_io_req.h,v $
 * Revision 1.4  2005/01/06 21:27:18  dz
 * cleanup before first vm run
 *
 *
**/

#ifndef PAGER_IO_REQUEST_H
#define PAGER_IO_REQUEST_H

#include <phantom_types.h>
#include <queue.h>
#include <errno.h>

#define IO_RQ_SLEEP 0

typedef struct pager_io_request
{
    //void *              virt_addr;        // virtual add or 0 if not mapped
    phys_page_t         phys_page;        // prev state copy for save to make_page. used on snap make
    disk_page_no_t      disk_page;        // where on disk is this

    // TODO mustdie - together w. pager's queue
    struct pager_io_request *  next_page;        // used for pager or some other queue

    // Used internally by disk partitions support/driver code
    long                blockNo;         	// disk sector (usually 512-byte) no
    int                 nSect;                 	// no of disk sectors

    unsigned char       flag_pagein; // : 1;
    unsigned char       flag_pageout; // : 1;

    unsigned char       flag_ioerror; // : 1; // BUG - not used yet

    unsigned char       flag_urgent; // : 1; // BUG - not used yet

#if IO_RQ_SLEEP
    unsigned char       flag_sleep; // calling thread will be put onsleep until IO is done // BUG - not used yet
    int                 sleep_tid; // thread which is put asleep due to flag_sleep
#endif

    void                (*pager_callback)( struct pager_io_request *req, int write );

    queue_chain_t       disk_chain; // disk io q chain
    //struct queue_entry	disk_chain; // disk io q chain

    errno_t 		rc; // driver return code
} pager_io_request;


static __inline__ void
pager_io_request_init( pager_io_request *me)
{ 
	me->flag_pagein 	= 0;
	me->flag_pageout 	= 0;
        me->flag_ioerror 	= 0;
        me->flag_urgent 	= 0;

        me->rc                  = 0;

        me->blockNo             = 0;
        me->nSect               = 0;

        me->pager_callback 	= 0;
#if IO_RQ_SLEEP
        me->flag_sleep 		= 0;
        me->sleep_tid 		= 0;
#endif
}


void pager_io_request_done( pager_io_request *me );


#endif // PAGER_IO_REQUEST_H

