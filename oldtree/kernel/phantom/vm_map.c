/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Main paging/persistent memory unit.
 *
 *
**/

#define DEBUG_MSG_PREFIX "pager"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/config.h>
#include <sys/syslog.h>
#include <kernel/debug_graphical.h>

#include <assert.h>


//---------------------------------------------------------------------------


#include <phantom_libc.h>


#include <kernel/vm.h>
#include <kernel/stats.h>
#include <kernel/dpc.h>
#include <kernel/snap_sync.h>
#include <kernel/physalloc.h>
#include <kernel/init.h>

#include <threads.h>

#ifdef ARCH_ia32
#include <ia32/proc_reg.h>
#include <ia32/phantom_pmap.h>
#endif

#include <time.h>



#include "vm_map.h"
#include "pager.h"

#include <machdep.h>

//#include <kernel/ia32/cpu.h>
//#include <kernel/ia32/mmx.h>
#include <kernel/trap.h>

//#define volatile /* */

#define USE_SNAP_WAIT 1


#if USE_SNAP_WAIT

static void init_snap_wait( void );
static void signal_snap_snap_passed( void );
static void signal_snap_done_passed( void );

#endif



static int DEBUG_MARK = 0;
static int SNAP_DEBUG = 0;
static int SNAP_STEPS_DEBUG = 1;
static int SNAP_LISTS_DEBUG = 0;
static int FAULT_DEBUG = 0;
static int PAGING_DEBUG = 0;
static int COW_DEBUG = 0;
static int PAGEOUT_DEBUG = 0;








// We do atomic works with this mutex taken.
// If something long happens with ne page we
// set
static hal_mutex_t vm_map_mutex;

//vm_map_impl   vm_map;
//spinlock    vm_page_lock;
//static hal_mutex_t vm_page_mutex;


static void vm_map_deferred_disk_alloc_thread(void);
static void vm_map_lazy_pageout_thread(void);
static void vm_map_snapshot_thread(void);


static void page_clear_engine_init(void);
static void page_clear_engine_clear_page(physaddr_t p);

static void vm_verify_snap(disk_page_no_t head);
static void vm_verify_vm(void);


static hal_cond_t      deferred_alloc_thread_sleep;

/*
 * Lock ordering for both clean and dirty queues:
 * first lock vm_page, second -- *_q_mutex.
 */
// clean physical pages queue (to reclaim from the end)
static hal_mutex_t      clean_q_mutex;
static hal_cond_t       clean_q_nonempty;
static queue_head_t     clean_q;
static size_t           clean_q_size;
// dirty physical pages queue (to pageout from the end)
static hal_mutex_t      dirty_q_mutex;
static queue_head_t     dirty_q;
static size_t           dirty_q_size;



// for each copy of system this address can't change - we
// keep it in a pagefile.

static void *              vm_map_start_of_virtual_address_space;
static unsigned long       vm_map_vm_page_count = 0;         // how many pages VM has, if 0 = we are not running yet

static vm_page *           vm_map_map;                       // array of pages
static vm_page *           vm_map_map_end;                   // a byte after map

static int last_snap_is_done = 0;


void                vm_map_wait_for_finish() { while(!last_snap_is_done) hal_sleep_msec(100); }
void *              vm_map_get_object_address_space_start() { return vm_map_start_of_virtual_address_space; }


//---------------------------------------------------------------------------




//static void process_deferred_allocations(void *arg);
//static volatile pager_io_request *deferred_allocations = 0; // memory
//dpc_request deferred_allocation_dpc;

//static volatile vm_page *deferred_disk_allocations = 0;

/* unused
unsigned char      phantom_vm_generation; // system's current generation number

// generation is cycling, do special comparison
__inline__ static int page_generation_is_less( unsigned char page_gen )
{
    // take wraparound in account
    if( phantom_vm_generation & 0x80 ) // upper bit? Move both down a half
        return (page_gen - 0x80) < (phantom_vm_generation - 0x80);
    else
        return page_gen < phantom_vm_generation;
}
*/

#ifdef PAGE_TOUCH_HISTORY_SIZE
static void page_touch_history(vm_page *p)
{
    void **ebp;
    asm volatile ("movl %%ebp,%0" : "=r" (ebp));

    memmove(p->touch_history + 1, p->touch_history, sizeof(p->touch_history) - sizeof(void*));
    p->touch_history[0] = ebp[1];
}

static void page_touch_history_arg(vm_page *p, int arg)
{
    void **ebp;
    asm volatile ("movl %%ebp,%0" : "=r" (ebp));

    memmove(p->touch_history + 2, p->touch_history, sizeof(p->touch_history) - 2 * sizeof(void*));
    p->touch_history[0] = ebp[1];
    p->touch_history[1] = (void*)arg;
}
#else
static inline void page_touch_history(vm_page *p)
{
    (void)p;
}
static inline void page_touch_history_arg(vm_page *p, int arg)
{
    (void)p;
    (void)arg;
}
#endif


static void    page_fault( vm_page *p, int  is_writing );

static vm_page *addr_to_vm_page(unsigned long addr, struct trap_state *ts)
{
    addr -= (addr_t)vm_map_start_of_virtual_address_space;


    if( addr >= ( ((unsigned long)vm_map_vm_page_count) * __MEM_PAGE) )
    {
        if( ts ) phantom_check_user_trap( ts );
        //dump_ds
        panic("address 0x%X is outside of object space", addr);
    }

    int pageno = addr / __MEM_PAGE;

    if(FAULT_DEBUG) syslog( 0, "fault 0x%lX pgno %d\n", addr, pageno );

    return vm_map_map + pageno;
}


static void
vm_map_page_fault_handler( void *address, int  write, int ip, struct trap_state *ts )
{
    (void) ip;

#if CONF_DUAL_PAGEMAP
	int ola = arch_is_object_land_access_enabled(); //< check if current thread attempts to access object space having access disabled
	if( !ola )
	{
		lprintf("\nObject land access disabled\n");
		trap_panic(ts);
	}
#endif

#if 1
    vm_page *vmp = addr_to_vm_page((addr_t) address, ts);
#else
    // TODO! Stack growth? Object space growth?
    long addr = (unsigned int) address;

    addr -= (unsigned int)vm_map_start_of_virtual_address_space;
    /*
    if( addr < 0 || addr >= (vm_map_vm_page_count*__MEM_PAGE) )
    {
        dump_ds
        panic("fault address 0x%X is outside of object space, IP 0x%X", address, ip);
    }
    */

    int pageno = addr / __MEM_PAGE;

    vm_page *vmp = vm_map_map + pageno;

    if(FAULT_DEBUG) syslog( 0, "fault 0x%X pgno %d\n", addr, pageno );

#endif

    hal_mutex_lock(&vmp->lock);
    page_touch_history_arg(vmp, ip);
    page_fault( vmp, write );
    hal_mutex_unlock(&vmp->lock);

}


/*
 * Trap handler. All we (currently) care about is page faults. Everything
 * else is passed through.
 */
int
vm_map_page_fault_trap_handler(struct trap_state *ts)
{

#ifdef ARCH_ia32
    if (ts->trapno == T_DEBUG)
    {
        //hal_printf("\n\n\n --- debug trap at EIP=0x%X --- \n\n\n", ts->eip );
        return 0;
    }

    if (ts->trapno == T_PAGE_FAULT)
#endif
    {
#ifdef ARCH_e2k
#  warning check me
        addr_t fa = arch_get_fault_address(); // TODO put it to trap_state!
        addr_t ip = TS_PROGRAM_COUNTER;
#  warning find out if it was a write op
        int is_write = 0;
#endif
#ifdef ARCH_ia32
        addr_t fa = arch_get_fault_address();
        ts->cr2 = fa;
        addr_t ip = ts->eip;
        int is_write = ts->err & T_PF_WRITE;
#endif
#ifdef ARCH_amd64
        addr_t fa = arch_get_fault_address(); // TODO put it to trap_state!
#warning is_write
        //ts->cr2 = fa;
        addr_t ip = ts->hw_rip;
        int is_write = ts->hw_err & T_PF_WRITE;
#endif
#ifdef ARCH_arm
        addr_t fa = arch_get_fault_address(); // TODO put it to trap_state!
        addr_t ip = ts->pc;
#  warning find out if it was a write op
        int is_write = 0;
#endif
#ifdef ARCH_mips
        addr_t fa = ts->va;
        addr_t ip = ts->pc;
        int is_write = ts->trapno == T_TLB_STORE;
#endif

#if NEW_SNAP_SYNC
        if( ((addr_t)fa) == ((addr_t)snap_catch_va) )
        {
            // This is a snap trap fired, let it go
            snap_trap();
            return 0; // trap supposed to be open now
        }
#endif

        {
            unsigned long addr = fa;

            addr -= (addr_t)vm_map_start_of_virtual_address_space;

            if( addr >= (vm_map_vm_page_count*__MEM_PAGE) )
            {
                phantom_check_user_trap( ts );
                dump_ss(ts);
                panic("fault address 0x%p is outside of object space, IP 0x%X", fa, ip);
            }
        }


        vm_map_page_fault_handler( (void *)fa, is_write, ip, ts );
        return 0;
    }

    /*
     * Not a page fault. Pass it through to the application as
     * a signal. If signal handling is not enabled, a trap dump
     * will be generated.
     */
    panic("Not a page fault in vm_map_page_fault_trap_handler\n");
}





// vm_map_init is not called for tests, so we leave there just real vm_map init code
// and general init is here

static void vm_map_pre_init(void)
{
    page_clear_engine_init();

    queue_init(&clean_q);
    hal_mutex_init(&clean_q_mutex, "CleanQueue");
    hal_cond_init(&clean_q_nonempty, "CleanQueueNonempty");
    queue_init(&dirty_q);
    hal_mutex_init(&dirty_q_mutex, "DirtyQueue");
    hal_mutex_init(&vm_map_mutex, "VM Map");
    hal_mutex_lock(&vm_map_mutex);

    hal_cond_init(&deferred_alloc_thread_sleep, "Deferred");
    hal_mutex_unlock(&vm_map_mutex);

#if USE_SNAP_WAIT
    init_snap_wait();
#endif

}

INIT_ME(0,vm_map_pre_init,0)




void
vm_map_init(unsigned long page_count)
{
    //page_clear_engine_init();
    SHOW_FLOW0( 1, "Started");

    vm_map_vm_page_count = page_count;

    int mapsize = vm_map_vm_page_count*sizeof(vm_page);

    vm_map_map = (vm_page *)malloc( mapsize );
    memset( vm_map_map, 0, mapsize );
    /*
    unsigned int i;
    for( i = 0; i < page_count; i++ )
    {
        memset( vm_map_map+i, 0, sizeof(vm_page) );
    }*/

    vm_map_map_end = vm_map_map + page_count;

    vm_map_start_of_virtual_address_space = (void *)hal_object_space_address();

    /*
    queue_init(&clean_q);
    hal_mutex_init(&clean_q_mutex, "CleanQueue");
    hal_cond_init(&clean_q_nonempty, "CleanQueueNonempty");
    queue_init(&dirty_q);
    hal_mutex_init(&dirty_q_mutex, "DirtyQueue");
    hal_mutex_init(&vm_map_mutex, "VM Map");
    hal_mutex_lock(&vm_map_mutex);
    */

    unsigned int np;
    for( np = 0; np < page_count; np++ )
        vm_page_init( &vm_map_map[np], ((char *)vm_map_start_of_virtual_address_space) + (__MEM_PAGE * np) );


    disk_page_no_t snap_start = 0;

    if(pager_superblock_ptr()->last_snap != 0 )
    {
        hal_printf("-- Use last snap\n");
        snap_start = pager_superblock_ptr()->last_snap;
    }
    else if(pager_superblock_ptr()->prev_snap != 0 )
    {
        hal_printf("-- Missing last snap, use previous snap\n");
        snap_start = pager_superblock_ptr()->prev_snap;
    }


    if( snap_start == 0 )
    {
        hal_printf("\n!!! No pagelist to load !!!\n");
        //panic("vmem load: no pagelist!");
    }
    else
    {

        hal_printf("Loading pagelist from %d...\n", snap_start);

        pagelist loader;
        pagelist_init( &loader, snap_start, 0, DISK_STRUCT_MAGIC_SNAP_LIST );

        pagelist_seek(&loader);

        for( np = 0; np < page_count; np++ )
        {
            if( !pagelist_read_seq(&loader, &vm_map_map[np].prev_page) )
            {
                printf("\n!!! Incomplete pagelist !!!\n");
                //panic("Incomplete pagelist\n");
                break;
            }

            // Zero page means we have no data fr this block and it must be zero
            vm_map_map[np].flag_have_prev = (vm_map_map[np].prev_page != 0);
        }

        pagelist_finish( &loader );
    }

    //dpc_request_init(&deferred_allocation_dpc,process_deferred_allocations);

    hal_init_object_vmem(vm_map_start_of_virtual_address_space);

    //hal_cond_init(&deferred_alloc_thread_sleep, "Deferred");
    //hal_mutex_unlock(&vm_map_mutex);

#if 1
    hal_start_kernel_thread(vm_map_deferred_disk_alloc_thread);
    hal_start_kernel_thread(vm_map_lazy_pageout_thread);
    hal_start_kernel_thread(vm_map_snapshot_thread);
#endif

    // Ok, everything is ready now. Turn on pagefaults handling
#ifdef ARCH_ia32
    phantom_trap_handlers[T_PAGE_FAULT] = vm_map_page_fault_trap_handler;
#  define HAVE_PGFAULT_HANDLER
#endif

#ifdef ARCH_arm
#endif

#ifndef HAVE_PGFAULT_HANDLER
#  warning no page fault trap handler set
#endif


}


static int  stop_deferred_disk_alloc_thread =  0;
static int  stop_lazy_pageout_thread        =  0;

void vm_map_finish(void)
{
    stop_lazy_pageout_thread = 1;
    // stop_deferred_disk_alloc_thread is asserted in lazy pageout thread
    // after the last snapshot
}



void
vm_page_init( vm_page *me, void *my_vaddr)
{
    memset( me, 0, sizeof(vm_page) );
    me->virt_addr = my_vaddr;
    hal_cond_init(&me->done, "VM PG");
    hal_mutex_init(&me->lock, "VM PG" );
    page_touch_history(me);
    pager_io_request_init( &me->pager_io );
}







// memory reclaiming helpers

static int is_on_reclaim_q( vm_page *p )
{
    return p->reclaim_q_chain.next != 0;
}

static void put_on_clean_q(vm_page *p)
{
    assert(!is_on_reclaim_q(p));
    hal_mutex_lock(&clean_q_mutex);
    queue_enter(&clean_q, p, vm_page *, reclaim_q_chain);
    ++clean_q_size;
    hal_cond_broadcast(&clean_q_nonempty);
    hal_mutex_unlock(&clean_q_mutex);
}

static void put_on_dirty_q(vm_page *p)
{
    assert(!is_on_reclaim_q(p));
    hal_mutex_lock(&dirty_q_mutex);
    queue_enter_first(&dirty_q, p, vm_page *, reclaim_q_chain);
    ++dirty_q_size;
    hal_mutex_unlock(&dirty_q_mutex);
}

static void remove_from_clean_q(vm_page *p)
{
    assert(clean_q_size > 0);
    assert(is_on_reclaim_q(p));
    hal_mutex_lock(&clean_q_mutex);
    queue_remove(&clean_q, p, vm_page *, reclaim_q_chain);
    --clean_q_size;
    p->reclaim_q_chain.next = 0;
    hal_mutex_unlock(&clean_q_mutex);
}

static void remove_from_dirty_q(vm_page *p)
{
    assert(dirty_q_size > 0);
    assert(is_on_reclaim_q(p));
    hal_mutex_lock(&dirty_q_mutex);
    queue_remove(&dirty_q, p, vm_page *, reclaim_q_chain);
    --dirty_q_size;
    p->reclaim_q_chain.next = 0;
    hal_mutex_unlock(&dirty_q_mutex);
}

static void move_to_dirty_q(vm_page *p)
{
    remove_from_clean_q(p);
    put_on_dirty_q(p);
}







// -------------
// Global snapshot related variables
int     is_in_snapshot_process = 0;
//int     snapshot_swapout_queue_size = 0;
// -------------



/*
 // called under the lock!
 static void snap_clean_page( vm_page *p )
 {
 if(p->flag_snapped )        panic("snap_clean got snapped page");
 if(p->flag_phys_dirty)      panic("snap_clean got dirty page");
 if(p->flag_have_make)       panic("snap_clean got flag_have_make set");

 // well, now page can have no current if it did not changed since last snap
 if(!p->flag_have_curr)
 {
 // in this case we do nothing. new snap will get it from previous one
 p->flag_snapped = 1;
 return;
 }

 p->make_page = p->curr_page;
 p->flag_have_make = 1;
 p->flag_have_curr = 0;
 p->flag_snapped = 1;
 }
 */



//---------------------------------------------------------------------------
//       vm_page helpers - CALL UNDER MUTEX!!!
//---------------------------------------------------------------------------

/*
void
vm_page_req_deferred_disk_alloc(vm_page *me)
{
    if(me->flag_dfda_active) return;
    me->flag_dfda_active = 1;

    me->dfda_next = (vm_page *)deferred_disk_allocations;
    deferred_disk_allocations = (volatile vm_page *)me;

    hal_printf("req deferred disk alloc 0x%X\n", me->virt_addr );

    // wake allocator
    //hal_wakeup(&deferred_disk_allocations);
    hal_cond_broadcast( &deferred_alloc_thread_sleep );
}*/

static void
pagein_callback( pager_io_request *p, int  pageout );

// Called under the lock

void
vm_page_req_pageout(vm_page *me)
{
    extern void pageout_callback( pager_io_request *req, int  write );

    if(PAGING_DEBUG) hal_printf("req pageout 0x%X: ", me->virt_addr );

    if (me->flag_pager_io_busy)
    {
        // if it is in the pager, it will return clean. no reason to wait
        if(PAGING_DEBUG) hal_printf("already in pager\n" );
        page_touch_history(me);
        return;
    }

    if(!me->flag_phys_mem)
    {
        if(PAGING_DEBUG) hal_printf("no phys mem\n" );
        page_touch_history(me);
        return;
    }

    if(!me->flag_phys_dirty)
    {
        if(PAGING_DEBUG) hal_printf("not dirty\n" );
        page_touch_history(me);
        return;
    }

    if(!me->flag_have_curr)
    {
        page_touch_history(me);
        if(PAGING_DEBUG) hal_printf("no curr disk page\n" );

        // Ask them to allocate us some disk space.
        if(PAGING_DEBUG) hal_printf("ask disk block 0x%X\n", me->virt_addr );

        if( !pager_alloc_page(&me->curr_page) )
        {
            panic("can't alloc disk page in req pageout");
        }
        me->flag_have_curr = 1;
        if(PAGING_DEBUG) hal_printf("got disk block for 0x%X\n", me->virt_addr );
    }

    hal_page_control(me->phys_addr, me->virt_addr, page_map, page_ro );

    me->flag_phys_protect = 1;
    me->flag_pager_io_busy = 1;
    me->pager_io.phys_page = me->phys_addr;
    me->pager_io.pager_callback = pageout_callback;
    me->pager_io.disk_page = me->curr_page;

    remove_from_dirty_q(me);
    page_touch_history(me);
    hal_mutex_unlock(&me->lock);
    if(PAGEOUT_DEBUG||PAGING_DEBUG) hal_printf("really req pageout\n" );
    pager_enqueue_for_pageout(&me->pager_io);
    hal_mutex_lock(&me->lock);
}


void pageout_callback( pager_io_request *req, int write )
{
    assert(write);

    if(req->rc) panic("pager write error, disk page %d", req->disk_page );

    vm_page *vmp = (vm_page *)req;

    hal_mutex_lock(&vmp->lock);

    if(PAGEOUT_DEBUG||PAGING_DEBUG) hal_printf("pageout callback 0x%X\n", vmp->virt_addr );

    assert(vmp->flag_pager_io_busy);

    req->pager_callback = 0;

    vmp->flag_phys_dirty = 0; // just saved out, we're clean
    put_on_clean_q(vmp);
    vmp->flag_pager_io_busy = 0;
    page_touch_history(vmp);

    hal_cond_broadcast(&vmp->done);

    hal_mutex_unlock(&vmp->lock);
}




//---------------------------------------------------------------------------
//       Page Fault
//---------------------------------------------------------------------------

void snapper_COW_callback( pager_io_request *req, int  write );

// this code will do fault-time snapshot work
// returns 1 if caller must return now
static int
page_fault_snap_aid( vm_page *p, int  is_writing  )
{
    // 1. We are called only for user's write access attempt!
    assert(is_writing);

    // Is it OK?
    if(p->flag_have_make)
        return 0; // No special snap treatment needed, or, maybe, already done

    if(!p->flag_phys_mem || !p->flag_phys_dirty)
    {
        // Since we're not backed, have only actual copy on disk.
        // this copy will go to snapshot, but we'll read it in
        // for process to go on. And later will swap out to a separate
        // disk page

        if(SNAP_DEBUG) hal_printf("snapaid !physmem 0x%X\n", p->virt_addr );

        // after we will return 0 caller will alloc phys page.

        // where precisely on disk?
        if( p->flag_have_curr )
        {
            page_touch_history(p);
            p->make_page = p->curr_page;
            p->flag_have_curr = 0;
            p->flag_have_make = 1;
            return 0; // Do standard write fault processing
        }

        if( p->flag_have_prev )
        {
            page_touch_history(p);
            // do nothing, just mark we're up to date, since
            // page did not change since previous snap. This situation
            // has to be taken care about in snap finalization.
            // Code there must move page to const block, possibly.
            // (A block that keeps pages that do not change for
            // generations.)
            p->make_page = p->prev_page;
            p->flag_have_prev = 0;
            p->flag_have_make = 1;
            return 0; // Do standard write fault processing
        }

        page_touch_history(p);
        // BUG? Can we just let him go? We definitely shall not write him?
        p->make_page = 0;
        p->flag_have_make = 1;
        return 0; // Do standard write fault processing
    }

    // we're in RAM, but still need one more RAM page

    /* TODO: We can start pageout even if no new RAM page is available */

    if(COW_DEBUG||SNAP_DEBUG) hal_printf("snapaid COW 0x%X\n", p->virt_addr );

    physaddr_t  new_phys;
    if( hal_alloc_phys_page(&new_phys) )
        panic("out of phys mem, no deferred alloc");

    hal_copy_page_v2p( new_phys, p->virt_addr );

    assert(!p->flag_pager_io_busy);

    p->flag_pager_io_busy = 1;

    // pass new page to pager
    p->pager_io.phys_page = new_phys;
    p->pager_io.pager_callback = snapper_COW_callback;

    // current disk page will come to snap - activate swapout now

    if( p->flag_have_curr )
    {
        page_touch_history(p);
        p->make_page = p->curr_page;
        p->flag_have_curr = 0;
        p->flag_have_make = 1;
    }
    else
    {
        if( !pager_alloc_page(&(p->make_page)) )
            panic("can't alloc disk page in req pageout");
        p->flag_have_make = 1;
        page_touch_history(p);
    }

    // start pageout
    if(SNAP_DEBUG) hal_printf("req pageout fast");
    p->pager_io.disk_page = p->make_page;

    hal_page_control(p->phys_addr, p->virt_addr, page_map, page_rw );
    p->flag_phys_protect = 0;

    p->access_count++;
    assert(p->flag_phys_dirty);
    p->flag_phys_dirty = 1; // we'll be dirty after return from trap
    page_touch_history(p);

    // release page_fault_write as we've made separate page copy for IO
    hal_cond_broadcast(&p->done);

    hal_mutex_unlock(&p->lock);
    pager_enqueue_for_pageout(&p->pager_io);
    hal_mutex_lock(&p->lock);

    return 1; // Don't do standard write fault processing
}





void snapper_COW_callback( pager_io_request *req, int  write )
{
    // Called after we copied write-attempted page to side-allocated
    // storage and kicked it to be paged out to a snapshot.

    assert(write);

    if(req->rc) panic("pager COW write error, disk page %d", req->disk_page );

    vm_page *vmp = (vm_page *)req;
    hal_mutex_lock(&vmp->lock);
    if(COW_DEBUG||SNAP_DEBUG) hal_printf("COW callback 0x%X\n", vmp->virt_addr );

    assert(vmp->flag_pager_io_busy);
    req->pager_callback = 0;

    // release memory used to hold page.

    hal_free_phys_page(req->phys_page);

    page_touch_history(vmp);
    vmp->flag_pager_io_busy = 0;
    hal_cond_broadcast(&vmp->done);
    hal_mutex_unlock(&vmp->lock);
}







static void
pagein_callback( pager_io_request *p, int  pageout )
{
    vm_page *vmp = (vm_page *)p;

    if(p->rc) panic("pager read error, disk page %d", p->disk_page );

    hal_mutex_lock(&vmp->lock);
    if(PAGING_DEBUG) hal_printf("pagein callback 0x%X\n", vmp->virt_addr );

    assert(vmp->flag_pager_io_busy);
    assert(!pageout);

    p->pager_callback = 0;

    hal_page_control( vmp->phys_addr, vmp->virt_addr, page_map,
           vmp->flag_phys_protect ? page_ro : page_rw);

    page_touch_history(vmp);
    vmp->flag_phys_dirty = 0;
    put_on_clean_q(vmp);
    vmp->flag_pager_io_busy = 0;
    hal_cond_broadcast(&vmp->done); // wakeup threads waiting for page
    hal_mutex_unlock(&vmp->lock);
}



// Mutex is taken!
// Process for memory read faults
static void
page_fault_read( vm_page *p )
{
    page_touch_history(p);
    while (p->flag_pager_io_busy)
    {
        hal_cond_wait(&p->done, &p->lock);
    }

    if (p->flag_phys_mem)
    {
        page_touch_history(p);
        return;
    }

    // read from unmapped page occured
    // it is the same for snapshot and normal operation mode

    if(FAULT_DEBUG) hal_printf("unmapped read 0x%X\n", p->virt_addr );
    // counter strike :)
    p->access_count++; // well, we shouldnt, but what the hell...

    // Allocate phys mem
    {
        physaddr_t newp;

        // TODO: on alloc fail just wake up pageout thread and sleep on
        // some 'mem avail' event, then retry.
        while(hal_alloc_phys_page(&newp))
        {

            panic("deferred alloc not implemented!");
            // TODO request some pageout works, etc
            //request_more_ram = 1;
            //hal_cond_wait(&ramSleepStone,&vm_map_mutex);
        }

        p->phys_addr = newp;
        p->flag_phys_mem = 1;
        p->flag_phys_dirty = 0;
        p->flag_phys_protect = 1; // read access.
    }

    // Allright, decide where to read from
    // if we have current and want just read, we are paging it in
    // in any state
    if     ( p->flag_have_curr )    p->pager_io.disk_page   = p->curr_page;
    else if( p->flag_have_make )    p->pager_io.disk_page   = p->make_page;
    else if( p->flag_have_prev )    p->pager_io.disk_page   = p->prev_page;
    else                            p->pager_io.disk_page   = 0;

    if (p->pager_io.disk_page == 0)
    {
        page_touch_history(p);
        // They're trying to read from unallocated page.
        // BUG! In fact, we are better send them an exception
        // because it looks like an error. But for now we'll just give
        // 'em an empty page.

        page_clear_engine_clear_page(p->phys_addr);
        p->flag_phys_dirty = 0;
        put_on_clean_q(p);
        p->flag_phys_protect = 1; // read access - see below.

        hal_page_control( p->phys_addr, p->virt_addr, page_map, page_ro );
        return;
    }

    assert(!p->flag_pager_io_busy);
    if(FAULT_DEBUG) hal_printf("start pagein 0x%X\n", p->virt_addr );

    p->flag_pager_io_busy = 1;
    p->pager_io.phys_page = p->phys_addr;
    p->pager_io.pager_callback = pagein_callback;
    page_touch_history(p);

    hal_mutex_unlock(&p->lock);
    pager_enqueue_for_pagein(&p->pager_io);
    pager_raise_request_priority(&p->pager_io);
    hal_mutex_lock(&p->lock);

    while (p->flag_pager_io_busy)
    {
        hal_cond_wait(&p->done, &p->lock);
    }
    page_touch_history(p);
}






// Mutex is taken!
// Process for memory write faults
static void
page_fault_write( vm_page *p )
{
    page_touch_history(p);
    if(FAULT_DEBUG) hal_printf("write 0x%X\n", p->virt_addr );
    // we're here if it was write (and, possibly, page is not mapped)

    // don't change page data if it's under IO
    while (p->flag_pager_io_busy && p->pager_io.phys_page == p->phys_addr)
    {
        // if it's snapshot time and this IO is pageout
        // try to dequeue it and reprocess through snap_aid
        if (is_in_snapshot_process && !p->flag_have_make &&
                pager_dequeue_from_pageout(&p->pager_io))
        {
            p->flag_phys_dirty ? put_on_dirty_q(p) : put_on_clean_q(p);
            page_touch_history(p);
            if(FAULT_DEBUG) hal_printf("dequeued 0x%X\n", p->virt_addr );
            p->flag_pager_io_busy = 0;
            break;
        }
        // failed to dequeue, at least try to raise its priority
        pager_raise_request_priority(&p->pager_io);

        if(FAULT_DEBUG) hal_printf("waiting for pager io 0x%X\n", p->virt_addr );
        hal_cond_wait(&p->done, &p->lock);
    }

    if (p->flag_phys_mem && !p->flag_phys_protect)
    {
        page_touch_history(p);
        if(FAULT_DEBUG) hal_printf("solved meanwhile 0x%X\n", p->virt_addr );
        if (!p->flag_phys_dirty)
        {
            page_touch_history(p);
            move_to_dirty_q(p);
            p->flag_phys_dirty = 1; // we'll be dirty after return from trap
        }
        return;
    }

    // we have to aid snapping of this page
    if( is_in_snapshot_process && !p->flag_have_make)
    {
        page_touch_history(p);
        if(FAULT_DEBUG) hal_printf("aiding snap 0x%X\n", p->virt_addr );
        if( page_fault_snap_aid(p, 1) )
        {
            if(FAULT_DEBUG) hal_printf("done 0x%X\n", p->virt_addr );
            return;
        }
    }

    if(FAULT_DEBUG) hal_printf("get page to write 0x%X\n", p->virt_addr );

    // we are not snapping or already did all we need for snap.
    // Now get us a page to write to if we still have no!

    if(p->flag_phys_mem)
    {
        page_touch_history(p);
        if(FAULT_DEBUG) hal_printf("have physmem 0x%X\n", p->virt_addr );
        if(p->flag_phys_protect)
        {
            page_touch_history(p);
            hal_page_control( p->phys_addr, p->virt_addr, page_map, page_rw );
            p->flag_phys_protect = 0;
            p->access_count++;
            p->flag_phys_dirty = 1; // we'll be dirty after return from trap
            move_to_dirty_q(p);
            if(FAULT_DEBUG) hal_printf("unprotect to write 0x%X\n", p->virt_addr );
        }
        return;
    }

    // Get us some phys mem

    if(FAULT_DEBUG) hal_printf("get physmem 0x%X\n", p->virt_addr );

    // try to alloc phys page. if cant - kick allocator to find free space
    // (this will result in swapout, usually), then schedule dpc and put
    // thread to sleep
    physaddr_t newp;

    while(hal_alloc_phys_page(&newp))
    {

        panic("deferred alloc not implemented!");
        // TODO request some pageout works, etc
        //request_more_ram = 1;

        //hal_cond_wait(&ramSleepStone,&vm_map_mutex); // NB! Mutex must be locked!

    }

    // got some - now pagein.

    p->phys_addr = newp;
    p->flag_phys_mem = 1;
    p->flag_phys_dirty = 0; // or set it after pagein?
    p->flag_phys_protect = 0; // pager has to write there - or can it anyway?

    if     ( p->flag_have_curr ) p->pager_io.disk_page = p->curr_page;
    else if( p->flag_have_make ) p->pager_io.disk_page = p->make_page;
    else if( p->flag_have_prev ) p->pager_io.disk_page = p->prev_page;
    else                         p->pager_io.disk_page = 0;

    if (p->pager_io.disk_page == 0)
    {
        page_touch_history(p);
        if(FAULT_DEBUG) hal_printf("zero page 0x%X\n", p->virt_addr );
        // Just clear page here as it is new
        page_clear_engine_clear_page(p->phys_addr);
        hal_page_control( p->phys_addr, p->virt_addr, page_map, page_rw );
        p->flag_phys_dirty = 1;
        put_on_dirty_q(p);
        return;
    }

    assert(!p->flag_pager_io_busy);
    if(FAULT_DEBUG) hal_printf("req pagein 0x%X\n", p->virt_addr );

    p->flag_pager_io_busy = 1;
    p->pager_io.phys_page = p->phys_addr;
    p->pager_io.pager_callback = pagein_callback;

    page_touch_history(p);
    hal_mutex_unlock(&p->lock);
    pager_enqueue_for_pagein(&p->pager_io);
    pager_raise_request_priority(&p->pager_io);
    hal_mutex_lock(&p->lock);

    while (p->flag_pager_io_busy)
    {
        hal_cond_wait(&p->done, &p->lock);
    }
    page_touch_history(p);
    if (p->flag_phys_mem)
    {
        p->flag_phys_dirty = 1;
        move_to_dirty_q(p);
    }
}










#define LATENCY_DEBUG 0

// Mutex is taken!
//
// We will possibly reenable with sti(), or maybe not.
// Caller must enable ints himself. (Or, for USA readers, herself.)
void
page_fault( vm_page *p, int  is_writing )
{
#if LATENCY_DEBUG
    bigtime_t start = hal_system_time();
#endif
    if( is_writing )    page_fault_write( p );
    else                page_fault_read( p );
#if LATENCY_DEBUG
    bigtime_t end = hal_system_time();

    if (end - start > p->max_latency)
    {
        p->max_latency = (int)(end - start);
        if (p->max_latency > 100000)
            printf("page va %p, max latency: %d, %s\n",
                    p->virt_addr, p->max_latency, is_writing ? "w" : "r");
    }
#endif
}


// Used to show progress
int vm_map_do_for_percentage = 0;

static void
vm_map_do_for_all( vmem_page_func_t func, int lock )
{
    size_t total = vm_map_map_end-vm_map_map;
    vm_page *i;
    for( i = vm_map_map; i < vm_map_map_end; i++ )
    {
        if (lock)
            hal_mutex_lock(&i->lock);
        else
            assert(!hal_mutex_is_locked(&i->lock));
        func( i );
        if (lock)
            hal_mutex_unlock(&i->lock);
        vm_map_do_for_percentage = (100L*(i-vm_map_map))/total;
    }
    vm_map_do_for_percentage = 100;
}

static void
vm_map_for_all( vmem_page_func_t func )
{
    vm_map_do_for_all(func, 1);
}

static void
vm_map_for_all_locked( vmem_page_func_t func )
{
    vm_map_do_for_all(func, 0);
}



//---------------------------------------------------------------------------
//       Snapshot
// (runs in pager thread)
//---------------------------------------------------------------------------

//
// called under the lock!
//
// goal is to protect all pages against write - actual snap is just that,
// saving to disk is to be done in page fault code or later in regular snapshot
// code
//
static void mark_for_snap(vm_page *p)
{
    page_touch_history(p);
    if(p->flag_have_make)       panic("mark_for_snap got flag_have_make set");

    if(DEBUG_MARK) hal_printf("mark4snap 0x%x: ", p->virt_addr );

    if( !p->flag_phys_mem )
    {
        page_touch_history(p);
        // since we're not backed, nothing to do - everything will either
        // be done during next page fault or in regular snapshot code
        if(DEBUG_MARK) hal_printf("!phys\n");
        return;
    }

    if( p->flag_phys_protect )
    {
        page_touch_history(p);
        // just the same - page is already protected
        if(DEBUG_MARK) hal_printf("already ro\n");
        return;
    }

    if(DEBUG_MARK) hal_printf("set to ro\n");
    // ok, page is mapped, writeable: the real case.
    hal_page_control( p->phys_addr, p->virt_addr, page_map, page_ro );
    p->flag_phys_protect = 1;
}

//#define KICK_AT_ONCE 16
//static int kick_pageout_sleep_count = 0;
static void kick_pageout(vm_page *p)
{
    page_touch_history(p);
    static int cnt = 0;
    if(p->flag_phys_dirty)
    {
        //if(SNAP_DEBUG) hal_printf("V");
        vm_page_req_pageout(p);
        cnt++;
    }
    if(SNAP_DEBUG && 0 == (0xFFFFF & (addr_t)p->virt_addr) )
    {
        hal_printf("0x%X (%d)\n", p->virt_addr, cnt );
        cnt = 0;
    }
}


// NB! Call with vm_map_mutex taken
static void finalize_snap(vm_page *p)
{
    page_touch_history(p);
    if(SNAP_DEBUG) hal_printf("finalize_snap 0x%X: ", p->virt_addr );

    if(p->flag_have_make)
    {
        page_touch_history(p);
        if(SNAP_DEBUG) hal_printf("has make 1\n" );
        return;
    }

    while (p->flag_pager_io_busy)
    {
        if(SNAP_DEBUG) hal_printf("waiting for pager io\n" );
        hal_cond_wait(&p->done, &p->lock);
    }

    if(p->flag_have_make)
    {
        page_touch_history(p);
        if(SNAP_DEBUG) hal_printf("has make 2\n" );
        return;
    }

    assert(!p->flag_phys_dirty);

    if(p->flag_have_curr)
    {
        page_touch_history(p);
        p->make_page = p->curr_page;
        p->flag_have_curr = 0;
        p->flag_have_make = 1;
        return;
    }

    if(p->flag_have_prev)
    {
        page_touch_history(p);
        p->make_page = p->prev_page;
        p->flag_have_prev = 0;
        p->flag_have_make = 1;
        return;
    }

    page_touch_history(p);

    // either empty or nonexistent. How can we distinguish?
    p->make_page = 0; // NB! means page is not to be written or can be written as zeros
    p->flag_have_make = 1;

    if(SNAP_DEBUG) hal_printf(" done, " );
}


pagelist *snap_saver = 0;

static void save_snap(vm_page *p)
{
    page_touch_history(p);
    // TODO added for safety - remove or do in a more smart way?
    // HACK! We set have make and make_page = 0 on unused page
    if(! (p->flag_have_make && p->make_page == 0 && !p->flag_have_curr && !p->flag_have_prev) )
    {
        page_touch_history(p);
        vm_page_req_pageout(p);
    }

    assert(p->flag_have_make);

    hal_mutex_unlock(&p->lock);
    pagelist_write_seq( snap_saver, p->make_page);
    if(SNAP_LISTS_DEBUG) hal_printf("pg %d, ", p->make_page);
    hal_mutex_lock(&p->lock);

    page_touch_history(p);
    p->prev_page = p->make_page;
    p->flag_have_make = 0;
    p->flag_have_prev = 1;
}


static void wait_commit_snap(vm_page *p)
{
    if (p->flag_pager_io_busy && p->flag_have_curr && p->pager_io.disk_page == p->curr_page)
        return;

    while (p->flag_pager_io_busy)
    {
        hal_cond_wait(&p->done, &p->lock);
    }
}

// TODO if we page out page, which is unchanged since THE SNAP and page fault comes (somebody wants to write to that
// page) we need to do COW too!

void do_snapshot(void)
{
    int			  enabled; // interrupts

    syslog( 0, "snap: started");
    // prerequisites
    //
    // - no pages with flag_have_make can exist! check that?
    //

    // This pageout request is not nesessary, but makes snap to catch a more later state.
    // If we skip this pageout, a lot of pages will go to 'after snap' state.
    // TODO try to find some heuristic to pageout just pages modified long ago?

    // Do it in lowest prio (but not IDLE) or else massive IO will kill world
    int prio;
    t_current_get_priority(&prio);
    t_current_set_priority( THREAD_PRIO_LOWEST );


    vm_map_for_all( kick_pageout ); // Try to pageout all of them - NOT IN LOCK!
    if(SNAP_STEPS_DEBUG) syslog( 0, "snap: wait 4 pgout to settle");

    // Back to orig prio
    t_current_set_priority( prio );

    // commented out to stress the pager
    //hal_sleep_msec(30000); // sleep for 10 sec - why 10?

    if(SNAP_STEPS_DEBUG) syslog( 0, "snap: stop world");


    // MUST BE BEFORE hal_mutex_lock!
    phantom_snapper_wait_4_threads();

    if(SNAP_STEPS_DEBUG) syslog( 0, "snap: threads stopped");

    enabled = hal_save_cli();

    vm_verify_vm();

    // START!
    is_in_snapshot_process = 1;

    // Terrible and mighty step - ALL the pages will be marked
    // as not snapped and access to them will be locked here, so
    // that page faults will bring them to us on write attempts and we'll
    // make a copies (COW).

    // !!!! SnapShot !!!!

    syslog( 0, "snap: hold still, say 'cheese!'...");

    // TODO: we have top do more. such as stop oher CPUS, force VMs into the
    // special snap-friendly state, etc
    //t_smp_enable(0); // make sure other CPUs don't mess here
    t_migrate_to_boot_CPU();
    vm_map_for_all_locked( mark_for_snap );
    t_smp_enable(1);

    syslog( 0, "snap: thank you ladies");

    if(enabled) hal_sti();

    phantom_snapper_reenable_threads();
#if USE_SNAP_WAIT
    signal_snap_snap_passed(); // or before enabling threads?
#endif

    // YES, YES, YES, Snap is nearly done.

    // Here we have to wait a little and start processing pages manually
    // because no one can be sure that all the pages will be accessed for
    // write in a short time.

    // This pageout request is needed - if I skip it, snaps are incomplete
    if(SNAP_STEPS_DEBUG) syslog( 0, "snap: pgout");
    vm_map_for_all( kick_pageout ); // Try to pageout all of them - NOT IN LOCK!

    //if(SNAP_STEPS_DEBUG) syslog( 0, "snap: go kick ass those lazy pages");
    //if(SNAP_DEBUG) getchar();

    syslog( 0, "snap: will finalize_snap");
    // scan nonsnapped pages, snap them manually (or just access to cause
    // page fault?)
    vm_map_for_all( finalize_snap );

    // now all pages must have make_page.
    // will save them now and move make_page -> prev_page,
    // don't want page_fault_write to create them another make_page as we do this.
    is_in_snapshot_process = 0;

    // now all the pages for snapshot are done. Now create
    // the disk data structure for them

    // TODO - free prev snap first!

    //long new_snap_head = 0;
    disk_page_no_t new_snap_head = 0;


    if(SNAP_STEPS_DEBUG) syslog( 0, "snap: creating primary pagelist root");
    if( !pager_alloc_page(&new_snap_head) ) panic("out of disk!");


    if(SNAP_STEPS_DEBUG) syslog( 0, "snap: creating pagelist...");
    //if(SNAP_DEBUG) getchar();

    {
        pagelist saver;
        pagelist_init( &saver, new_snap_head, 1, DISK_STRUCT_MAGIC_SNAP_LIST );

        pagelist_clear(&saver);
        snap_saver = &saver;
        vm_map_for_all( save_snap );
        snap_saver = 0;
        pagelist_flush(&saver);
        pagelist_finish(&saver);
    }

    if(SNAP_STEPS_DEBUG) syslog( 0, "snap: waiting for all pages to be flushed...");
    // make sure page data has been written
    vm_map_for_all( wait_commit_snap );

    vm_verify_snap(new_snap_head);

    // ok, now we have current snap and previous one. come fix the
    // superblock
    disk_page_no_t toFree = pager_superblock_ptr()->prev_snap; // Save list head to be deleted

    pager_superblock_ptr()->prev_snap = pager_superblock_ptr()->last_snap;
    pager_superblock_ptr()->last_snap = new_snap_head;
    pager_update_superblock();


    // TODO free list @toFree excep for pages used in actual two lists
    disk_page_no_t actual1 = pager_superblock_ptr()->prev_snap;
    disk_page_no_t actual2 = pager_superblock_ptr()->last_snap;

    phantom_free_snap ( toFree, actual1, actual2 );

    // Force all io to complete BEFORE updating superblock
    pager_fence();

    pager_update_superblock();

    //#error not impl
    // and free pages of previous-previous snapshot that changed in this
    // snapshot and not used in previous. Release data structure of
    // previous-previous snapshot too

    //#error not impl
    // free journal part, which was created before this snap
    // was started

    // Force all io to complete
    pager_fence();

    // DONE!
    syslog( 0, "Snapshot done!");

    STAT_INC_CNT(STAT_CNT_SNAPSHOT);

#if USE_SNAP_WAIT
    signal_snap_done_passed();
#endif

    hal_sleep_msec(20000);
    syslog( 0, "snap: wait for 10 sec more");
    hal_sleep_msec(10000);

}


//---------------------------------------------------------------------------
// Normal pageout housekeeping activity
// (runs in pager thread on a quite low prio)
//---------------------------------------------------------------------------


static int vm_regular_snaps_enabled = 0;

void vm_enable_regular_snaps() { SHOW_FLOW0( 1, "Enabled"); vm_regular_snaps_enabled = 1; }

#if MEM_RECLAIM

void physmem_try_to_reclaim_page(void)
{
    vm_page *p = NULL;

    hal_mutex_lock(&clean_q_mutex);
    while (!clean_q_size)
        hal_cond_wait(&clean_q_nonempty, &clean_q_mutex);
    p = (vm_page*)queue_last(&clean_q);
    hal_mutex_unlock(&clean_q_mutex);
    if (p)
    {
        hal_mutex_lock(&p->lock);
        if (p->flag_phys_mem && !p->flag_phys_dirty && is_on_reclaim_q(p) && !(p->wired_count))
        {
            page_touch_history(p);
            remove_from_clean_q(p);
            p->flag_phys_mem = 0; // Take it
            physaddr_t paddr = p->phys_addr;
            hal_page_control(paddr, p->virt_addr, page_unmap, page_noaccess);
            hal_free_phys_page(paddr);
        }
        hal_mutex_unlock(&p->lock);
    }
}

static inline int need_pageout(size_t dirty, size_t clean)
{
    if (!dirty)
        return 0;
    /* play with numbers */
    if (clean < 8)
        return dirty >= clean;
    if (clean < 32)
        return dirty > clean * 16;
    return dirty > clean * 4096;
}

static void balance_clean_dirty(void)
{
    size_t clean;
    size_t dirty;
    do
    {
        vm_page *p;

        hal_mutex_lock(&clean_q_mutex);
        clean = clean_q_size;
        hal_mutex_unlock(&clean_q_mutex);
        hal_mutex_lock(&dirty_q_mutex);
        dirty = dirty_q_size;
        p = (vm_page*)queue_last(&dirty_q);
        hal_mutex_unlock(&dirty_q_mutex);
        if (need_pageout(dirty, clean))
        {
            hal_mutex_lock(&p->lock);
            if (p->flag_phys_mem && p->flag_phys_dirty)
            {
                page_touch_history(p);
                vm_page_req_pageout(p);
                --dirty;
            }
            hal_mutex_unlock(&p->lock);
        }
    } while (need_pageout(dirty, clean));
}

#else

static inline void balance_clean_dirty(void)
{
}

#endif

static void vm_map_lazy_pageout_thread(void)
{
    SHOW_FLOW0( 1, "Ready");
    t_current_set_name("LazyPageout");

    while(1)
    {
        if( stop_lazy_pageout_thread )
            hal_exit_kernel_thread();

        hal_sleep_msec( 100 ); // TODO: cond_wait?

        balance_clean_dirty();
    }
}
static int request_snap_flag = 0;
static int seconds_between_snaps = 100;

static void vm_map_snapshot_thread(void)
{
    t_current_set_name("SnapShot");
    t_set_snapper_flag(); // Tell 'em IM THE SNAPPER, don't you ever try to stop me :)

    while(1)
    {
        if( stop_lazy_pageout_thread )
        {
            do_snapshot();
            stop_deferred_disk_alloc_thread = 1;

            hal_cond_broadcast( &deferred_alloc_thread_sleep );
            // so that it will see the stop flag

            hal_exit_kernel_thread();
        }

        if(!vm_regular_snaps_enabled)
        {
            hal_sleep_msec( 100 );
            continue;
        }

        int i = 0;
        while( (!request_snap_flag) && (i++ < seconds_between_snaps) )
        {
            hal_sleep_msec( 1000 );
        }

        if( vm_regular_snaps_enabled || request_snap_flag )
            do_snapshot();

        request_snap_flag = 0;

    }
}

void request_snap(void)
{
    request_snap_flag++;
}

void set_snap_interval( int interval_sec )
{
    seconds_between_snaps = interval_sec;
}

static void vm_map_deferred_disk_alloc_thread(void)
{
    SHOW_FLOW0( 1, "Ready");

    t_current_set_name("DeferAlloc");

    while(1)
    {
        //hal_sleep( &deferred_disk_allocations );
        hal_mutex_lock(&vm_map_mutex); // TODO: this is for cond_wait to have something to unlock only
        hal_cond_wait( &deferred_alloc_thread_sleep, &vm_map_mutex );
        hal_mutex_unlock(&vm_map_mutex);

        // see lazy pageout shutdown
        if(stop_deferred_disk_alloc_thread)
        {
            last_snap_is_done = 1;
            hal_exit_kernel_thread();
        }

        SHOW_FLOW0( 1, "Active");

        //hal_sleep_msec( 100 );
        // sleep is to check for situation where disk mem is not available for a long time

        pager_refill_free();
    }
}





//---------------------------------------------------------------------------
// Page clear code
//---------------------------------------------------------------------------



static void * 		page_clear_vaddr; // Place in address space to map page to clear
static hal_spinlock_t	page_clear_lock;

// TODO idle time pre-clear to some queue
static void page_clear_engine_init(void)
{
    int ie = hal_save_cli();
    hal_spin_init(&page_clear_lock);
    hal_spin_lock(&page_clear_lock);

    if( hal_alloc_vaddress( &page_clear_vaddr, 1 ) )
        panic("page_clear_vaddr alloc failed");

    hal_spin_unlock(&page_clear_lock);
    if(ie) hal_sti();
}


// Clear phys page from some special vaddress
// TODO perCPU vaddress and no spinlock?
static void page_clear_engine_clear_page(physaddr_t p)
{
    int enabled = hal_save_cli();
    hal_spin_lock(&page_clear_lock);

    if(FAULT_DEBUG)
        hal_printf("page_clear_engine_clear_page( 0x%X )\n", p );

    hal_page_control( p, page_clear_vaddr, page_map, page_rw );

    // TODO use MMX clear code
    //memset( page_clear_vaddr, '#', __MEM_PAGE );
    memset( page_clear_vaddr, 0, __MEM_PAGE );

    // TODO Broken!
    //fast_clear_page( page_clear_vaddr );

    hal_page_control( p, page_clear_vaddr, page_unmap, page_ro );

    hal_spin_unlock(&page_clear_lock);
    if (enabled) hal_sti();
}

#if VERIFY_SNAP || VERIFY_VM_SNAP

#include <vm/object.h>
#include <vm/alloc.h>

static size_t vm_verify_object(void *p)
{
    struct pvm_object_storage *curr = p;
    assert(curr->_ah.object_start_marker == PVM_OBJECT_START_MARKER);
    if (pvm_object_is_allocated_light(p))
        pvm_object_is_allocated_assert(p);
    return curr->_ah.exact_size;
}

/**
 * Verify objects in the page.
 *
 * When object's pvm_object_storage crosses page boundary, part of it is stored in
 * the hdr for reassembly with the next page.
 *
 * @param data: page data
 * @param page_offset: relative to the vm_map_start_of_virtual_address_space
 * @param current: current object offset relative to the vm_map_start_of_virtual_address_space
 * @param sz: size of VM, limit for the current
 *
 * @return offset of the object out of the current page, unless current object's
 * pvm_object_storage crosses page boundary
 */
static size_t vm_verify_page(void *data, size_t page_offset, size_t current, size_t sz)
{
    static struct pvm_object_storage hdr;

    if (current < page_offset && page_offset - current < sizeof(hdr))
    {
        memcpy(((void*)&hdr) + (page_offset - current), data,
                sizeof(hdr) - (page_offset - current));
        current += vm_verify_object(&hdr);
    }
    while (current < sz && current - page_offset < PAGE_SIZE)
    {
        if (current + sizeof(hdr) - page_offset <= PAGE_SIZE)
            current += vm_verify_object(data + (current - page_offset));
        else
        {
            memcpy(&hdr, data + (current - page_offset), PAGE_SIZE - (current - page_offset));
            break;
        }
    }
    return current;
}

#endif

#if VERIFY_VM_SNAP

static void vm_verify_vm(void)
{
    size_t current = 0;
    int np;

    if(SNAP_STEPS_DEBUG) hal_printf("Verifying VM before snapshot...\n");
    for (np = 0; np < vm_map_map_end - vm_map_map; np++)
    {
        size_t page_offset = np * PAGE_SIZE;
        current = vm_verify_page(vm_map_start_of_virtual_address_space + page_offset,
                page_offset, current, hal.object_vsize);
    }
    if(SNAP_STEPS_DEBUG) hal_printf("VM verification icomplete\n");
}

#else

static void vm_verify_vm(void)
{
}

#endif

#if VERIFY_SNAP

#define USE_SYNC_IO 1

static void vm_verify_snap(disk_page_no_t head)
{
    int progress = 0;
    int np;
    pagelist loader;
    size_t current = 0;


    if (!head)
        return;

    if (SNAP_STEPS_DEBUG) syslog( 0, "snap: verification started...");

#if !USE_SYNC_IO
    disk_page_io page_io;

    disk_page_io_init(&page_io);
    disk_page_io_allocate(&page_io);
#endif

    pagelist_init(&loader, head, 0, DISK_STRUCT_MAGIC_SNAP_LIST);

    pagelist_seek(&loader);

    for(np = 0; np < vm_map_map_end - vm_map_map; np++)
    {
        size_t page_offset = np * PAGE_SIZE;
        disk_page_no_t block;
	short percentage = np * 100 / (vm_map_map_end - vm_map_map);

        if (progress != percentage)
        {
            progress = percentage;
            if (SNAP_STEPS_DEBUG)
	    {
		if (progress % 10) hal_printf(". ", progress);
		else hal_printf(".%d%%\n", progress);
	    }
        }
        if (!pagelist_read_seq(&loader, &block))
        {
            printf("Incomplete pagelist\n");
            //panic("Incomplete pagelist\n");
            break;
        }

        if (current < page_offset || current - page_offset < PAGE_SIZE)
        {
#if USE_SYNC_IO
            extern phantom_disk_partition_t *pp; // BUG

            char buf[DISK_STRUCT_BS];

            errno_t rc = phantom_sync_read_block( pp, buf, block, 1 );
            if( rc )
            {
                syslog( 0, "snap: verification read err %d", rc );
                return;
            }

            current = vm_verify_page(buf, page_offset, current, hal.object_vsize);
#else
            page_io.req.disk_page = block;
            disk_page_io_load_me_async(&page_io);
            disk_page_io_wait(&page_io);
            current = vm_verify_page(page_io.mem, page_offset, current, hal.object_vsize);
#endif
        }
    }

    pagelist_finish( &loader );
#if !USE_SYNC_IO
    disk_page_io_release(&page_io);
#endif
    if (SNAP_STEPS_DEBUG)
    {
	hal_printf("\n");
	syslog( 0, "snap: verification completed");
    }
}

#else

static inline void vm_verify_snap(disk_page_no_t head)
{
    (void)head;
}

#endif


//---------------------------------------------------------------------------
// Wire/unwire code
//---------------------------------------------------------------------------


void wire_page( vm_page *p )
{
    p->wired_count++;

    STAT_INC_CNT( STAT_CNT_WIRE );

    if(!p->flag_phys_mem)
    {
        STAT_INC_CNT( STAT_CNT_WIRE_PAGEIN );
        /*
        hal_mutex_lock(&p->lock);
        page_touch_history_arg(p, 0);

        pagein somehow

        hal_mutex_unlock(&p->lock);
        */
        volatile int val = *((char *)p->virt_addr); // Just touch it
        (void) val;
    }
}

void unwire_page( vm_page *p )
{
    assert(p->wired_count > 0);
    p->wired_count--;
}



//! Make page wired (fixed in phys mem, allways present)
// It is guaranteed that after return and up to the call
// to unwire_page_for_addr physical addr will be the same
void wire_page_for_addr( void *addr, size_t count )
{
    void *pp = (void *)PREV_PAGE_ALIGN((addr_t)addr);
    ssize_t c = count;
    c += addr-pp;

    do{
        wire_page( addr_to_vm_page((addr_t) pp, 0) );
        c -= PAGE_SIZE;
        pp += PAGE_SIZE;
    } while( c > 0 );
}

void unwire_page_for_addr( void *addr, size_t count )
{
    void *pp = (void *)PREV_PAGE_ALIGN((addr_t)addr);
    ssize_t c = count;
    c += addr-pp;

    do{
        unwire_page( addr_to_vm_page((addr_t) pp, 0) );
        c -= PAGE_SIZE;
        pp += PAGE_SIZE;
    } while( c > 0 );
}


// TODO must unmap and free physmem, and resulting page must not go
// to snap at all (must have 0 in pagelist = no pagein on snap load)
void vm_map_page_mark_unused( addr_t page_start )
{
#if VM_UNMAP_UNUSED_OBJECTS
    //printf("asked to mark page %p unused\n", page_start);
    vm_page *vmp = addr_to_vm_page( page_start, 0 );

    hal_mutex_lock(&vmp->lock);

    page_touch_history(vmp);

    if(vmp->wired_count) goto done; // very strange, but ok, just skip it
    if(vmp->flag_pager_io_busy) goto done; // don't mess
    if(is_in_snapshot_process) goto done; // don't mess

    /*
    page_touch_history(p);
    remove_from_clean_q(p);
    p->flag_phys_mem = 0; // Take it
    physaddr_t paddr = p->phys_addr;
    hal_page_control(paddr, p->virt_addr, page_unmap, page_noaccess);
    hal_free_phys_page(paddr);
    */

    vmp->flag_phys_dirty = 0; // just skip paging out

    //page_fault( vmp, write );
done:
    hal_mutex_unlock(&vmp->lock);
#else
    (void) page_start;
#endif
}






//---------------------------------------------------------------------------
// Wait for snapshot code
//---------------------------------------------------------------------------

#if USE_SNAP_WAIT


static hal_cond_t  wait_snap_snap;
static hal_cond_t  wait_snap_done;
static hal_mutex_t wait_snap_mutex;



static void init_snap_wait( void )
{
    assert ( 0 == hal_mutex_init( &wait_snap_mutex, "SnapWait" ) );

    assert ( 0 == hal_cond_init( &wait_snap_snap, "SnapSnapWait" ) );

    assert ( 0 == hal_cond_init( &wait_snap_done, "SnapDoneWait" ) );
}


void phantom_wait_4_snapshot_snap( void )
{
    //errno_t rc;

    assert ( 0 == hal_mutex_lock( &wait_snap_mutex ) );

    assert ( 0 == hal_cond_wait( &wait_snap_snap, &wait_snap_mutex ) );

    assert ( 0 == hal_mutex_unlock( &wait_snap_mutex ) );
}



void phantom_wait_4_snapshot_done( void )
{
    //errno_t rc;

    assert ( 0 == hal_mutex_lock( &wait_snap_mutex ) );

    assert ( 0 == hal_cond_wait( &wait_snap_done, &wait_snap_mutex ) );

    assert ( 0 == hal_mutex_unlock( &wait_snap_mutex ) );
}



static void signal_snap_snap_passed( void )
{
    assert ( 0 == hal_cond_broadcast( &wait_snap_snap ) );
}

static void signal_snap_done_passed( void )
{
    assert ( 0 == hal_cond_broadcast( &wait_snap_snap ) );
}

#endif





//---------------------------------------------------------------------------
// Debug window - mem map
//---------------------------------------------------------------------------

static rgba_t calc_persistent_pixel_color( int elem, int units_per_pixel );

/**
 * 
 * \brief Generic painter for any allocator using us.
 * 
 * Used in debug window.
 * 
 * \param[in] w Window to draw to
 * \param[in] r Rectangle to paint inside
 * 
**/
void paint_persistent_memory_map(window_handle_t w, rect_t *r )
{
    if(!vm_map_vm_page_count) return;

    int pixels = r->xsize * r->ysize;
    int units_per_pixel =  1 + ((vm_map_vm_page_count-1) / pixels);

    int x, y;
    for( y = 0; y < r->ysize; y++ )
    {
        for( x = 0; x < r->xsize; x++ )
            w_draw_pixel( w, x + r->x, y + r->y, calc_persistent_pixel_color( x + y * r->xsize, units_per_pixel ));
    }

}

static rgba_t calc_persistent_pixel_color( int elem, int units_per_pixel )
{
    vm_page *ep = vm_map_map + elem;

    int state = 0; // 0 = empty, 1 = partial, 2 = used
    int bits = 0;
    int do_io = 0;

    int i;
    for( i = 0; i < units_per_pixel; i++, ep++ )
    {
        if( 0 == ep->flag_phys_mem ) continue; // empty, no change
        state = 2; // full
        bits += 1;

        if( ep->flag_pager_io_busy ) 
        {
            do_io = 1;
            //lprintf("io %d ", elem+i);
        }
    }

    if(do_io) return COLOR_YELLOW;

    switch(state)
    {
        case 0: return COLOR_BLUE;
        
        case 1: 
        {
            //return COLOR_LIGHTGREEN;
            rgba_t c = COLOR_LIGHTGREEN;
            // lighter = less used
            c.g = 0xFF - (bits * 0xFF / (units_per_pixel * sizeof(map_elem_t) * 8));
            return c;
        }

        case 2: return COLOR_LIGHTRED;

        default: return COLOR_BLACK;
    }
}


