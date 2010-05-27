#ifndef pagerH
#define pagerH

//---------------------------------------------------------------------------
#include <phantom_disk.h>

#include "paging_device.h"
#include "pager_io_req.h"
#include "pagelist.h"
#include "spinlock.h"
#include "hal.h"
//---------------------------------------------------------------------------

void page_device_io_final_callback(void);




// phantom virtual memory/snapshot engine general interface

/* TODO: need queue sort to optimize head movement */

/* TODO: 
common r/w queue for usual prio and special high prio q?
common q will let us sort rads and writes together and
optimize head movement better */

/* TODO: need queue management class here */


    void 			pager_stop_io(); // called after io is complete
    void 			pager_free_io_resources(pager_io_request *req);

    void                        pager_get_superblock(void);
    void                        pager_update_superblock(void);

    void                        pager_fix_incomplete_format(void);
    int                         pager_fast_fsck(void);
    int                         pager_long_fsck(void);


    // DO NOT use superblock free_start!
    void                        pager_put_to_free_list( disk_page_no_t );
    void                        pager_refill_free_reserve(void);
    void                        pager_format_empty_free_list_block( disk_page_no_t );
    void                        pager_init_free_list(void);


    // NB! On init pagefile must be good and healthy,
    // so if one needs some check and fix, it must be done
    // before.
    void                pager_init(paging_device * device); // {  }
    void                pager_finish(void);


    int                 pager_interrupt_alloc_page(disk_page_no_t *out);
    int                 pager_alloc_page(disk_page_no_t *out);
    void                pager_free_page( disk_page_no_t );

    int        		pager_can_grow(); // can I grow pagespace


    void                pager_enqueue_for_pagein ( pager_io_request *p );
    void                pager_enqueue_for_pagein_fast ( pager_io_request *p );
    void                pager_enqueue_for_pageout( pager_io_request *p );
    void                pager_enqueue_for_pageout_fast( pager_io_request *p );

    void                pager_io_done(void);

    long                pager_object_space_address(void);

    phantom_disk_superblock *        pager_superblock_ptr();




    void                pager_refill_free(void);

    // called under sema!
    void 		pager_start_io(); // called to start new io






    void 		phantom_fsck(int do_rebuild );


    void 		phantom_free_snap(
                                          disk_page_no_t old_snap_start,
                                          disk_page_no_t curr_snap_start,
                                          disk_page_no_t new_snap_start
                                         );


#endif
