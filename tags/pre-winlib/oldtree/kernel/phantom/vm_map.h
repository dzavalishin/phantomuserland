/**
 *
 * Phantom OS
 *
 * Copyright (C) 2004-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Virtual memory map.
 * Maps VM page to phys mem page and pagefile page(s).
 *
**/


#ifndef vm_mapH
#define vm_mapH

#include <queue.h>
#include <kernel/vm.h>

#include "spinlock.h"
#include "pager.h"
#include "hal.h"


void vm_enable_regular_snaps( void );

// todo: vm map can be sparse. We can omit (and dynamically load)
// vm page descriptor for page which is not accessed for a long
// time. It's state is well-known without an in-memory descripting object.


//
// Spinlock usage guide:
//
// for really short page ops do under the lock, but check for busy bit. If
// busy found - unlock and wait.
//
// for long - lock, check/set page busy bit, unlock, do op, lock, reset 
// busy bit, unlock.
//
// for scan like ops - divide in parts of about 1000 pages, do part under lock, 
// if busy page happens, unlock and wait
//
hal_spinlock_t    vm_page_lock;

/* unused
extern unsigned char      phantom_vm_generation; // system's current generation number
*/

#define ONEBIT
//#define ONEBIT :1

typedef struct vm_page
{
    // NB!! pager_io_request MUST BE FIRST so that its address is our address too!
    struct pager_io_request pager_io;

    void *              virt_addr;     	// where phys_addr is mapped
    physaddr_t          phys_addr;      // our phys mem page, if any

    // Can't touch pager_io data
    unsigned char       flag_pager_io_busy      ONEBIT;

    // We are o DeFerred Disk Alloc queue
    //unsigned char       flag_dfda_active        ONEBIT;

    // We're backed by phys mem
    unsigned char       flag_phys_mem           ONEBIT;

    // But write protected to receive usage info or snap.
    unsigned char       flag_phys_protect       ONEBIT;

    // dirty - backed, access is rw (if no flag_phys_protect), changed
    // against disk (curr) content
    unsigned char       flag_phys_dirty         ONEBIT;


    unsigned char       flag_have_curr ONEBIT;     // curr_page is actual
    unsigned char       flag_have_make ONEBIT;     // make_page is actual
    unsigned char       flag_have_prev ONEBIT;     // prev_page is actual

    // i am in a pagefile
    disk_page_no_t       curr_page; // changes will go here
    disk_page_no_t       make_page; // page of a snapshot we create(d) now
    disk_page_no_t       prev_page; // page in a previous snapshot


    // Now rules for pagefile pages here.

    // If have_curr is 0, on the next page fault it will
    // be allocated in any state and any conditions, if
    // we have pagefile space to alloc from. If we have not -
    // we're out of memory - see out of mem strategy description
    // elsewhere. (Note deferred allocations...)

    // have_make
    // This page is what we prepared for a new snapshot. It is what our
    // generation number corresponds to. If it's empty and generation is less
    // than phantom_vm_generation - than we have to snap ourselves NOW.

    // NB! - if our generation is less and this block (have_make)
    // is NOT empty - then what?

    // have_prev
    // is what we had on previous (but still last actual) snapshot
    // we'll read from it if no changes were done to the page since then.

    int                 access_count; // Need?

    // How many times in line lazy pageout thread found this page to be unreferenced
    int                 idle_count;

    hal_cond_t      	done;
    hal_mutex_t         lock;

    queue_chain_t       reclaim_q_chain; // Used to put page on memory reclaim list

    int                 max_latency;
//#define PAGE_TOUCH_HISTORY_SIZE 20
#ifdef PAGE_TOUCH_HISTORY_SIZE
    void *              touch_history[PAGE_TOUCH_HISTORY_SIZE];
#endif

    int                 wired_count; // If nonzero, page must be present and can't be moved/paged out. Physical address must not change.

} vm_page;


#undef ONEBIT



void		vm_page_init( vm_page *me, void *my_vaddr);

// These helpers are to be called with CLI!!
void		vm_page_req_deferred_disk_alloc();
void       	vm_page_req_pageout();


// Need access queue(s?) so that accessed pages will go to the beginning of
// q and rarely accessed ones will migrate to the end. Pager will slowly mark
// pages in the end of accessed q to be inaccessible, and page fault will reenable access (or will
// we use hardware access marker?). Unaccessed pages will come to the
// beginning of unaccessed queue. Pages to be freed will be extracted
// from the end of unaccessed queue. Any page access will move page to the
// beginning of accessed q.


    typedef void (*vmem_page_func_t)(  vm_page * );
    typedef int (*bool_vmem_page_func_t)(  vm_page * );


    //static volatile bool last_snap_is_done;

    //void         	vm_map_page_fault_handler( void *address, int is_write );
    //void         	vm_map_lazy_pageout_thread(void);
    //void         	vm_map_deferred_disk_alloc_thread(void);

    void                vm_map_do_for( vmem_page_func_t func, bool_vmem_page_func_t predicate );


    void                vm_map_init(unsigned long page_count);
    void                vm_map_finish(void);


    //void                vm_map_for_all( vmem_page_func_t func );
    //void                vm_map_for_unmapped( vmem_page_func_t func );
    //void                vm_map_for_mapped( vmem_page_func_t func );
    //void                vm_map_for_not_snapped( vmem_page_func_t func );
    //void                vm_map_for_snapped( vmem_page_func_t func );

    void                vm_map_clear_snapped(void);

    void *              vm_map_get_object_address_space_start(); // { return start_of_virtual_address_space; }


    void                vm_map_wait_for_finish(void);

//extern vm_map_impl   vm_map;

/*
 *
 * Well. I'll try to summarize possible virtual memory page states.
 *
 * - Unmapped, clean, no snapshot in progress. Usually has prev_page
 *   (for last good snap), can have or have no curr_page. Usually has,
 *   but of no - will get it on some access attempt
 *   (access violation exception) or with a deferred allocator later.
 *
 * - Mapped, clean. Can be mapped ro or noaccess - latter is to get some
 *   access statistics. Not defined yet. In fact, on 386+ we can get access
 *   stats from memory mapping data structures, so dont we need to bother
 *   with locking the pages.
 *
 *   Access stats are to be done in general.
 *
 * - Mapped, dirty. Can be mapped rw or noaccess - again, for access stats.
 *
 * In process of snapshot:
 *
 * - Mapped, generation is less: current phys page will be copied from
 *   phys_addr to a specially allocated RAM page used for pageout only.
 *   That new page will be mapped and queued for pageout to new snap.
 *
 * Todo:
 *   If access was not occured during that kind of pageout,
 * phys memory will simply return from io_phys_page_no to phys_addr.
 * If access occurs, thread will be stopped if no free pages exist.
 * If pages exist memory will be copied from io_phys_page_no to new
 * page (phys_addr) and process will continue.
 *
 * To make it possible snapper will try to bring in some free memory
 * before starting actual snapshot creation. It will, possibly, be good
 * to do an agressive phys mem alloc in the above scenario - ie,
 * take not only completely free, but even just not dirty pages off.
 *
 * - Not mapped, generation is less. Curr_page will just go to make_page. On
 * write page fault new disk page will be allocated and contents of
 * make_page will come there to be modified. On read page fault page
 * will be just read from make_page - no allocation is needed.
 *
 */



#define foreach_pause_counter_limit 1000
typedef struct foreach_pause_counter
{
    int         now;
} foreach_pause_counter;

void foreach_pause_counter_init(foreach_pause_counter *me);
int  foreach_pause(foreach_pause_counter *me);




// Called from physmem allocator to get some more physram
void physmem_try_to_reclaim_page(void);




#endif
