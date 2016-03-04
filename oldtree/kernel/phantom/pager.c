/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Paging: queue, FSCK
 *
 *
**/


#define DEBUG_MSG_PREFIX "pager"
#include "debug_ext.h"
#define debug_level_flow 1
#define debug_level_error 10
#define debug_level_info 10

#include <assert.h>
#include <errno.h>

#include <kernel/vm.h>
#include <kernel/stats.h>
#include <kernel/page.h>

#include <phantom_disk.h>

#include "pager.h"
#include "pagelist.h"

#if PAGING_PARTITION
#include <disk.h>
#endif

#define USE_SYNC_IO 0

#define free_reserve_size  32


static hal_mutex_t              pager_freelist_mutex;
static phantom_disk_superblock  superblock;


#if !PAGING_PARTITION
static int _DEBUG = 1;


static hal_mutex_t              pager_mutex;

static void                	pager_io_done(void);
static void 			page_device_io_final_callback(void);


static int                      pagein_is_in_process = 0;
static int                      pageout_is_in_process = 0;

static paging_device            *pdev;

static pager_io_request         *pager_current_request = 0;

static pager_io_request         *pagein_q_start = 0;    // points to first (will go now) page
static pager_io_request         *pagein_q_end = 0;      // points to last page
//static int                        pagein_q_len;

static pager_io_request         *pageout_q_start = 0;
static pager_io_request         *pageout_q_end = 0;
//static int                      pageout_q_len;


#endif

#if !USE_SYNC_IO
static disk_page_io             freelist_head;
static disk_page_io             superblock_io;
#endif

static int                      need_fsck;


static int                      freelist_inited = 0;
static disk_page_no_t           free_reserve[free_reserve_size]; // for interrupt time allocs
static int                      free_reserve_n;

static union
{
    struct phantom_disk_blocklist free_head;
    char __fill[PAGE_SIZE];
} u;


static disk_page_no_t           sb_default_page_numbers[] = DISK_STRUCT_SB_OFFSET_LIST;





#if !PAGING_PARTITION


// called when io is done on page device
static void page_device_io_final_callback()
{
    pager_io_done();
};




//---------------------------------------------------------------------------

int
pager_can_grow() // can I grow pagespace
{
    //if(!pdev)
        return 0;
    //else            return pdev->can_grow();
}

#endif


phantom_disk_superblock *
pager_superblock_ptr()
{
    return &superblock;
}










#if PAGING_PARTITION

// static
phantom_disk_partition_t *pp;

void partition_pager_init(phantom_disk_partition_t *p)
{
    pp = p;
    assert(pp);

    hal_mutex_init(&pager_freelist_mutex, "PagerFree");

#if !USE_SYNC_IO
    disk_page_io_init(&freelist_head);
    disk_page_io_init(&superblock_io);
    disk_page_io_allocate(&superblock_io);
#endif

    SHOW_FLOW0( 1, "Pager get superblock" );
    pager_get_superblock();

    //phantom_dump_superblock(&superblock);    getchar();

    pager_fast_fsck();

    if( need_fsck )
    {
        panic("I don't have any fsck yet. I must die.\n");
        pager_long_fsck();
    }


    //superblock.fs_is_clean = 0; // mark as dirty
    pager_update_superblock();
}


void
pager_finish()
{
    pager_flush_free_list();

    superblock.fs_is_clean = 1; // mark as clean
    pager_update_superblock();
    //disk_page_io_finish(&superblock_io);
}

// TODO implement for partitioning code
int pager_dequeue_from_pageout(pager_io_request *rq)
{
    return !disk_dequeue( pp, rq );
}

// TODO implement for partitioning code
void pager_raise_request_priority(pager_io_request *rq)
{
    disk_raise_priority( pp, rq );
}

void pager_enqueue_for_pagein ( pager_io_request *rq )
{
    assert(rq->phys_page);
    if( rq->flag_pagein ) panic("enqueue_for_pagein: page is already on pager queue (in)");
    if( rq->flag_pageout ) panic("enqueue_for_pagein: page is already on pager queue (out)");

    rq->flag_pagein = 1;
    rq->next_page = 0;

    STAT_INC_CNT( STAT_CNT_PAGEIN );

    disk_enqueue( pp, rq );

}

void pager_enqueue_for_pagein_fast ( pager_io_request *rq )
{
    rq->flag_urgent = 1;
    pager_enqueue_for_pagein ( rq );
}


void pager_enqueue_for_pageout( pager_io_request *rq )
{
    assert(rq->phys_page);
    if( rq->flag_pagein ) panic("enqueue_for_pagein: page is already on pager queue (in)");
    if( rq->flag_pageout ) panic("enqueue_for_pagein: page is already on pager queue (out)");

    rq->flag_pageout = 1;
    rq->next_page = 0;

    STAT_INC_CNT( STAT_CNT_PAGEOUT );

    disk_enqueue( pp, rq );
}


void pager_enqueue_for_pageout_fast( pager_io_request *rq )
{
    rq->flag_urgent = 1;
    pager_enqueue_for_pageout( rq );
}

errno_t pager_fence()
{
    return disk_fence( pp );
}



#else
void
pager_init(paging_device * device)
{
    pagein_q_start = pagein_q_end = 0;
    //pagein_q_len = 0;
    pageout_q_start = pageout_q_end = 0;
    //pageout_q_len = 0;
    pagein_is_in_process = pageout_is_in_process = 0;

    hal_mutex_init(&pager_mutex, "Pager Q");
    hal_mutex_init(&pager_freelist_mutex, "PagerFree");

    disk_page_io_init(&freelist_head);
    disk_page_io_init(&superblock_io);
    disk_page_io_allocate(&superblock_io);

    pdev = device;

    if(_DEBUG) hal_printf("Pager get superblock");
    pager_get_superblock();
    if(_DEBUG) hal_printf(" ...DONE\n");

    //phantom_dump_superblock(&superblock);    getchar();


    pager_fast_fsck();

    if( need_fsck )
        {
        panic("I don't have any fsck yet. I must die.\n");
        pager_long_fsck();
        }


    //superblock.fs_is_clean = 0; // mark as dirty
    pager_update_superblock();

}

void
pager_finish()
{
    // BUG! Kick all stuff out - especially disk_page_cachers
    pager_flush_free_list();

    superblock.fs_is_clean = 1; // mark as clean
    pager_update_superblock();
    disk_page_io_finish(&superblock_io);
}





void pager_debug_print_q()
{
    if(!_DEBUG) return;

    pager_io_request *last = pagein_q_start;

    for( ; last; last = last->next_page)
    {
        hal_printf("\nreq = 0x%X, next = 0x%X, disk = %d\n", last, last->next_page, last->disk_page);
    }
}














// called under sema!
void pager_start_io() // called to start new io
{
    if( pagein_is_in_process || pageout_is_in_process ) return;
    if( pagein_is_in_process && pageout_is_in_process ) panic("double io in start_io");

    if(_DEBUG) hal_printf("pager start io\n");

    // pagein has absolute priority as system response time depends on it - right?
    if(pagein_q_start)
    {
        pager_current_request = pagein_q_start;
        pagein_q_start = pager_current_request->next_page;
        if(pagein_q_start == 0) pagein_q_end = 0;

        pagein_is_in_process = 1;
        paging_device_start_read_rq( pdev, pager_current_request, page_device_io_final_callback );
    }
    else if(pageout_q_start)
    {
        pager_current_request = pageout_q_start;
        pageout_q_start = pager_current_request->next_page;
        if(pageout_q_start == 0) pageout_q_end = 0;

        pageout_is_in_process = 1;
        paging_device_start_write_rq( pdev, pager_current_request, page_device_io_final_callback );
    }
}




static void pager_io_done() // called after io is complete
{
    //hal_printf("pager_stop_io... ");
    hal_mutex_lock(&pager_mutex);

    if( (!pagein_is_in_process) && (!pageout_is_in_process) ) panic("stop when no io in pager");
    if( pagein_is_in_process    &&   pageout_is_in_process  ) panic("double io in stop_io");

    pager_io_request *last = pager_current_request;
//set_dr7(0); // turn off all debug registers

    STAT_INC_CNT( last->flag_pageout ? STAT_CNT_PAGEOUT : STAT_CNT_PAGEIN );

    if(pagein_is_in_process)
    {
        pager_debug_print_q();

        last->flag_pagein = 0;
        if(last->pager_callback)
        {
            hal_mutex_unlock(&pager_mutex);
            last->pager_callback( last, 0 );
            hal_mutex_lock(&pager_mutex);
        }
        pagein_is_in_process = 0;
    }

    if(pageout_is_in_process)
    {
        last->flag_pageout = 0;
        if(last->pager_callback)
        {
            hal_mutex_unlock(&pager_mutex);
            last->pager_callback( last, 1 );
            hal_mutex_lock(&pager_mutex);
        }
        pageout_is_in_process = 0;
    }

    pager_start_io();
    hal_mutex_unlock(&pager_mutex);
}





void pager_enqueue_for_pagein ( pager_io_request *p )
{
    if(p->phys_page == 0)
        panic("pagein to zero address");

    /*
    hal_printf("ENQ 4 pagein req  0x%X  VA 0x%X PA 0x%X disk %d, Q START = 0x%X\n",
               p,
               p->virt_addr,
               p->phys_page,
               p->disk_page,
               pagein_q_start
              ); */

    if( p->flag_pagein || p->flag_pageout ) panic("enqueue_for_pagein: page is already on pager queue");

    hal_mutex_lock(&pager_mutex);

    p->flag_pagein = 1;
    p->next_page = 0;
    if( pagein_q_start == 0 ) { pagein_q_start = p; pagein_q_end = p; /*start_io();*/ }
    else { pagein_q_end->next_page = p; pagein_q_end = p; }

    //hal_printf("ENQ 4 pagein p->next_page 0x%X\n",               p->next_page              );

    pager_start_io();

    hal_mutex_unlock(&pager_mutex);
}

void pager_enqueue_for_pagein_fast ( pager_io_request *p )
{
#if 1
    pager_enqueue_for_pagein( p );
#else
    if( p->flag_pagein || p->flag_pageout ) panic("enqueue_for_pagein_fast: page is already on pager queue");

    hal_mutex_lock(&pager_mutex);

    p->flag_pagein = 1;
    if( pagein_q_start == 0 ) { pagein_q_start = p; pagein_q_end = p; p->next_page = 0; }
    else { p->next_page = pagein_q_start; pagein_q_start = p; }
    pager_start_io();

    hal_mutex_unlock(&pager_mutex);
#endif
}

void pager_enqueue_for_pageout( pager_io_request *p )
{
    if(p->phys_page == 0)
        panic("pageout from zero address");
    /*hal_printf("ENQ 4 pageout req  0x%X  VA 0x%X PA 0x%X disk %d, Q START = 0x%X\n",
      p,
      p->virt_addr,
      p->phys_page,
      p->disk_page,
      pagein_q_start
      ); */
    if( p->flag_pagein || p->flag_pageout ) panic("enqueue_for_pageout: page is already on pager queue");

    hal_mutex_lock(&pager_mutex);

    p->flag_pageout = 1;
    p->next_page = 0;
    if( pageout_q_start == 0 ) { pageout_q_start = p; pageout_q_end = p; }
    else { pageout_q_end->next_page = p;  pageout_q_end = p; }
    pager_start_io();

    hal_mutex_unlock(&pager_mutex);
}


void pager_enqueue_for_pageout_fast( pager_io_request *p )
{
#if 1
    pager_enqueue_for_pageout( p );
#else
    if( p->flag_pagein || p->flag_pageout ) panic("enqueue_for_pageout_fast: page is already on pager queue");

    hal_mutex_lock(&pager_mutex);

    p->flag_pageout = 1;
    if( pageout_q_start == 0 ) { pageout_q_start = p; pageout_q_end = p; p->next_page = 0; }
    else { p->next_page = pageout_q_start;  pageout_q_start = p; }
    pager_start_io();

    hal_mutex_unlock(&pager_mutex);
#endif
}


void pager_raise_request_priority(pager_io_request *p)
{
    hal_mutex_lock(&pager_mutex);

    if (pager_current_request != p && pageout_q_start != p)
    {
        pager_io_request *i;
        for (i = pageout_q_start; i; i = i->next_page)
        {
            if (i->next_page == p)
            {
                i->next_page = p->next_page;
                if (pageout_q_end == p)
                    pageout_q_end = i;
                p->next_page = pageout_q_start;
                pageout_q_start = p;
                break;
            }
        }
    }

    hal_mutex_unlock(&pager_mutex);
}

int pager_dequeue_from_pageout(pager_io_request *p)
{
    int dequeued = 0;
    hal_mutex_lock(&pager_mutex);

    if (pageout_q_start == p)
    {
        pageout_q_start = p->next_page;
        if (!pageout_q_start)
            pageout_q_end = 0;
        p->flag_pageout = 0;
        dequeued = 1;
    }
    else if (pager_current_request != p)
    {
        pager_io_request *i;
        for (i = pageout_q_start; i; i = i->next_page)
        {
            if (i->next_page == p)
            {
                i->next_page = p->next_page;
                if (pageout_q_end == p)
                    pageout_q_end = i;

                p->flag_pageout = 0;
                dequeued = 1;
                break;
            }
        }
    }

    hal_mutex_unlock(&pager_mutex);

    return dequeued;
}

errno_t pager_fence()
{
    SHOW_ERROR0( 0, "No fence!" );
    return ENODEV;
}


#endif

// ---------------------------------------------------------------------------
//
// Superblock works
//
// ---------------------------------------------------------------------------

void write_blocklist_sure( struct phantom_disk_blocklist *list, disk_page_no_t addr  )
{
#if PAGING_PARTITION
    char buf[PAGE_SIZE];

    * ((struct phantom_disk_blocklist *)buf) = *list;
    assert(!phantom_sync_write_block( pp, buf, addr, 1 ));
#else
    (void) list;
    (void) addr;
    panic("no old code here");
#endif
}

void read_blocklist_sure( struct phantom_disk_blocklist *list, disk_page_no_t addr  )
{
#if PAGING_PARTITION
    char buf[PAGE_SIZE];

    assert(!phantom_sync_read_block( pp, buf, addr, 1 ));
    *list = * ((struct phantom_disk_blocklist *)buf);
#else
    (void) list;
    (void) addr;
    panic("no old code here");
#endif
}

void write_freehead_sure( disk_page_no_t addr )
{
    write_blocklist_sure( &u.free_head, addr );
}

void read_freehead_sure()
{
    read_blocklist_sure( &u.free_head, superblock.free_list );
}

errno_t write_superblock(phantom_disk_superblock *in, disk_page_no_t addr )
{
#if PAGING_PARTITION
    char buf[PAGE_SIZE];

    * ((phantom_disk_superblock *)buf) = *in;
    return phantom_sync_write_block( pp, buf, addr, 1 );
#else
    (void) in;
    (void) addr;
    panic("no old code here");
    return EIO;
#endif
}


errno_t read_uncheck_superblock(phantom_disk_superblock *out, disk_page_no_t addr )
{
#if USE_SYNC_IO
    char buf[PAGE_SIZE];

    errno_t rc = phantom_sync_read_block( pp, buf, addr, 1 );
    if( rc ) return rc;

    *out = * ((phantom_disk_superblock *)buf);
    return 0;
#else
    disk_page_io sb;

    disk_page_io_init( &sb );
    errno_t rc = disk_page_io_load_sync(&sb, addr);
    if( rc ) return rc;

    *out = * ((phantom_disk_superblock *)disk_page_io_data(&sb));

    disk_page_io_finish( &sb );
    return 0;
#endif
}


errno_t read_check_superblock(phantom_disk_superblock *out, disk_page_no_t addr )
{
#if USE_SYNC_IO
    char buf[PAGE_SIZE];

    errno_t rc = phantom_sync_read_block( pp, buf, addr, 1 );
    if( rc ) return rc;

    if( phantom_calc_sb_checksum( (phantom_disk_superblock *)buf ))
        {
        *out = * ((phantom_disk_superblock *)buf);
        return 0;
        }

    return ENOENT;
#else
    disk_page_io sb;

    disk_page_io_init( &sb );
    errno_t rc = disk_page_io_load_sync(&sb, addr);
    if( rc ) return rc;

    if( phantom_calc_sb_checksum( (phantom_disk_superblock *)disk_page_io_data(&sb) ))
        {
        *out = * ((phantom_disk_superblock *)disk_page_io_data(&sb));

        disk_page_io_finish( &sb );
        return 0;
        }

    disk_page_io_finish( &sb );
    return ENOENT;
#endif
}

int find_superblock(
    phantom_disk_superblock *out,
    disk_page_no_t *pages, int n_pages,
    disk_page_no_t exclude, disk_page_no_t *where )
{
    int i;
    for( i = 0; i < n_pages; i++ )
    {
        disk_page_no_t curr = pages[i];
        if( curr == 0 || curr == exclude )
            continue;

        hal_printf("Looking for superblock at %d... ", curr );

        if( !read_check_superblock( out, curr ) )
        {
            *where = curr;
            hal_printf("Found superblock at %d\n", curr );
            return 1;
        }
    }

    hal_printf("Superblock not found\n" );
    return 0;
}

void
pager_fix_incomplete_format()
{
    //int n_sb_default_page_numbers = sizeof(sb_default_page_numbers)/sizeof(disk_page_no_t);

    disk_page_no_t     sb2a, sb3a, free, max;

    // TODO: Use some more sophisticated selection alg.
    sb2a = sb_default_page_numbers[1];
    sb3a = sb_default_page_numbers[2];

    superblock.sb2_addr = sb2a;
    superblock.sb3_addr = sb3a;

    // find a block for freelist
    free = pager_superblock_ptr()->disk_start_page;
    while( free == sb_default_page_numbers[0] ||
           free == sb_default_page_numbers[1] ||
           free == sb_default_page_numbers[2] ||
           free == sb_default_page_numbers[3]
         )
        free++;

    max = sb2a > sb3a ? sb2a : sb3a;
    max = max > free ? max : free;
    max++; // cover upper block itself. now max is one page above

    superblock.free_start = max;    // blocks, not covered by freelist start with this one
    superblock.free_list = 0;       // none yet - read by pager_format_empty_free_list_block

    pager_format_empty_free_list_block( free );

    superblock.free_list = free;    // head of free list

    disk_page_no_t i;
    for( i = pager_superblock_ptr()->disk_start_page; i < max; i++ )
        {
        if( i == free ||
            i == sb_default_page_numbers[0] ||
            i == sb_default_page_numbers[1] ||
            i == sb_default_page_numbers[2] ||
            i == sb_default_page_numbers[3]
             )
            continue;

        pager_put_to_free_list(i);
        }

    if( 0 == superblock.object_space_address )
        superblock.object_space_address = PHANTOM_AMAP_START_VM_POOL;

    pager_update_superblock();
}


void
pager_get_superblock()
{
    //disk_page_no_t     sb_default_page_numbers[] = DISK_STRUCT_SB_OFFSET_LIST;
    int n_sb_default_page_numbers = sizeof(sb_default_page_numbers)/sizeof(disk_page_no_t);

    disk_page_no_t     sb_found_page_numbers[4];

    phantom_disk_superblock     root_sb;
    int                         root_sb_cs_ok;

    phantom_disk_superblock     sb1;
    int                         sb1_ok = 0;

    phantom_disk_superblock     sb2;
    int                         sb2_ok = 0;

    SHOW_FLOW0( 4, "Checking superblock...");
    //getchar();

#if USE_SYNC_IO
    // TODO check rc
    //phantom_sync_read_block( pp, &root_sb, sb_default_page_numbers[0], 1 );
    assert(! read_uncheck_superblock( &root_sb, sb_default_page_numbers[0] ) );

#else
    {
        disk_page_io                sb0;

        disk_page_io_init(&sb0);

        //if(_DEBUG) hal_printf(" Load root sb");
        // TODO check rc
        errno_t rc = disk_page_io_load_sync(&sb0,sb_default_page_numbers[0]); // read block 0x10
        if( rc ) SHOW_ERROR( 0, "sb read err @%d", sb_default_page_numbers[0]);
        //if(_DEBUG) hal_printf(" ...DONE\n");

        root_sb = * ((phantom_disk_superblock *) disk_page_io_data(&sb0));

        disk_page_io_finish(&sb0);
    }
#endif
    root_sb_cs_ok = phantom_calc_sb_checksum( &root_sb );

    SHOW_FLOW( 0, "root sb sys name = '%.*s', checksum %s\n",
               DISK_STRUCT_SB_SYSNAME_SIZE, root_sb.sys_name,
               root_sb_cs_ok ? "ok" : "wrong"
             );


    sb_found_page_numbers[0] = root_sb.sb2_addr;

    sb_found_page_numbers[1] = root_sb.sb3_addr;


    disk_page_no_t     found_sb1;
    disk_page_no_t     found_sb2;

    SHOW_FLOW0( 2, "Find sb1");

    sb1_ok = find_superblock( &sb1, sb_found_page_numbers, 2, 0, &found_sb1 );

    //if(_DEBUG) hal_printf(" ...DONE\n");

    if( sb1_ok )
    {
        SHOW_FLOW0( 2, "Find sb2");
        //if(_DEBUG) hal_printf("Find sb2");
        sb2_ok = find_superblock( &sb2, sb_found_page_numbers, 2, found_sb1, &found_sb2  );
        //if(_DEBUG) hal_printf(" ...DONE\n");
    }

    SHOW_FLOW( 7, "sb1 status %d, sb2 - %d, sb3 - %d\n", root_sb_cs_ok, sb1_ok, sb2_ok );
    SHOW_FLOW( 7, "root sb sb2 addr %d, sb3 addr %d, free start %d, free list %d, fs clean %d\n",
               root_sb.sb2_addr, root_sb.sb3_addr,
               root_sb.free_start, root_sb.free_list,
               root_sb.fs_is_clean
             );

    if(
       root_sb_cs_ok && (!sb1_ok) && !(sb2_ok) &&
       root_sb.sb2_addr == 0 && root_sb.sb3_addr == 0 &&
       root_sb.free_start != 0 && root_sb.free_list == 0 &&
       root_sb.fs_is_clean
      )
    {
        SHOW_FLOW0( 0, "incomplete filesystem found, fixing...");
        superblock = root_sb;
        pager_fix_incomplete_format();
        SHOW_FLOW0( 0, "incomplete format fixed");

        phantom_fsck( 0 );

        return;
    }


    if(
       root_sb_cs_ok && sb1_ok && sb2_ok &&
       superblocks_are_equal(&root_sb, &sb1) &&
       superblocks_are_equal(&root_sb, &sb2)
      )
    {
        if( (root_sb.version >> 16) != DISK_STRUCT_VERSION_MAJOR )
        {
            hal_printf("Disk FS major version number is incorrect: 0x%X\n", root_sb.version >> 16 );
            panic( "Can't work with this FS" );
        }

        if( (root_sb.version & 0xFFFF) > DISK_STRUCT_VERSION_MINOR )
        {
            hal_printf("Disk FS minor version number is too big: 0x%X\n", root_sb.version & 0xFFFF );
            panic( "Can't work with this FS" );
        }

        if( (root_sb.version & 0xFFFF) < DISK_STRUCT_VERSION_MINOR )
            hal_printf(" Warning: Disk FS minor version number is low: 0x%X, mine is 0x%X...", root_sb.version & 0xFFFF, DISK_STRUCT_VERSION_MINOR );

        hal_printf(" all 3 superblocks are found and good, ok.\n");
        superblock = root_sb;

        if( !superblock.fs_is_clean )
        {
            need_fsck = 1;
            hal_printf("FS marked as dirty, need to check.\n");
        }
        else
        {
            phantom_fsck( 0 );
            return;
        }
    }

    // Something is wrong.

    need_fsck = 1;
    SHOW_ERROR0( 0, " (need fsck)...");
    SHOW_ERROR0( 0, " default superblock copies are wrong, looking for more...");

#if 1
    if( !sb1_ok )
        sb1_ok = find_superblock( &sb1, sb_default_page_numbers,
                                  n_sb_default_page_numbers, 0, &found_sb1 );

    if( sb1_ok )
    {
        sb_found_page_numbers[2] = sb1.sb2_addr;
        sb_found_page_numbers[3] = sb1.sb3_addr;

        sb2_ok = find_superblock( &sb2, sb_found_page_numbers, 4, found_sb1, &found_sb2  );
        if( !sb2_ok )
            sb2_ok = find_superblock( &sb2, sb_default_page_numbers,
                                      n_sb_default_page_numbers, found_sb1, &found_sb2  );
    }

    (void) sb2_ok;
#endif
    panic("I don't have any fsck yet. I must die.\n");
}




void
pager_update_superblock()
{
    errno_t rc;

    SHOW_FLOW0( 0, " updating superblock...");

#if !USE_SYNC_IO
    assert(!(superblock_io.req.flag_pagein || superblock_io.req.flag_pageout));
#endif

    phantom_calc_sb_checksum( &superblock );

#if USE_SYNC_IO
    rc = write_superblock( &superblock, sb_default_page_numbers[0] );
    if( rc ) SHOW_ERROR0( 0, "Superblock 0 (main) write error!" ); // TODO rc


    if( need_fsck )
    {
        SHOW_ERROR0( 0, " disk is insane, will update root copy only (who called me here, btw?)...");
    }
    else
    {
        rc = write_superblock( &superblock, superblock.sb2_addr );
        if( rc ) SHOW_ERROR0( 0, "Superblock 1 write error!" ); // TODO rc

        rc = write_superblock( &superblock, superblock.sb3_addr );
        if( rc ) SHOW_ERROR0( 0, "Superblock 1 write error!" ); // TODO rc

        SHOW_FLOW0( 0, "saved all 3");
    }
#else
    *((phantom_disk_superblock *)disk_page_io_data(&superblock_io)) = superblock;
    rc = disk_page_io_save_sync(&superblock_io, sb_default_page_numbers[0]); // save root copy
    if( rc ) panic( "Superblock 0 write error!" );

    if( need_fsck )
    {
        SHOW_ERROR0( 0, " disk is insane, will update root copy only (who called me here, btw?)...\n");
    }
    else
    {
        rc = disk_page_io_save_sync(&superblock_io, superblock.sb2_addr);
        if( rc ) panic( "Superblock 1 write error!" );
        rc = disk_page_io_save_sync(&superblock_io, superblock.sb3_addr);
        if( rc ) panic( "Superblock 2 write error!" );
        SHOW_FLOW0( 0, " saved all 3");
    }

#endif
}

// ---------------------------------------------------------------------------
//
// Free list works
//
// ---------------------------------------------------------------------------

//const int            pager_free_reserve_size = 30;


void
pager_format_empty_free_list_block( disk_page_no_t fp )
{
    struct phantom_disk_blocklist freelist;

    memset( &freelist, 0, sizeof( freelist ) );

    freelist.head.magic = DISK_STRUCT_MAGIC_FREEHEAD;
    freelist.head.used = 0;
    freelist.head.next = superblock.free_list;
    freelist.head._reserved = 0;

#if USE_SYNC_IO
    u.free_head = freelist;
    //errno_t rc = phantom_sync_write_block( pp, &freelist, fp, 1 );
    //if( rc ) SHOW_ERROR0( 0, "empty freelist block write error!" ); // TODO rc
    //write_blocklist_sure( &freelist, fp );
    write_freehead_sure( fp );
#else
    *( (struct phantom_disk_blocklist *)disk_page_io_data(&freelist_head) ) = freelist;
    errno_t rc = disk_page_io_save_sync(&freelist_head,fp);
    if( rc ) panic("pager_format_empty_free_list_block");
#endif
}

void pager_flush_free_list(void)
{
    hal_mutex_lock(&pager_freelist_mutex);

#if USE_SYNC_IO
    //errno_t rc = phantom_sync_write_block( pp, &u.free_head, superblock.free_list, 1 );
    //if( rc ) SHOW_ERROR0( 0, "empty freelist block write error!" ); // TODO rc
    write_freehead_sure( superblock.free_list );
#else
    errno_t rc = disk_page_io_save_sync(&freelist_head,superblock.free_list);
    if( rc ) SHOW_ERROR0( 0, "freelist block write error!" ); // TODO rc
#endif
    hal_mutex_unlock(&pager_freelist_mutex);
}

void
pager_put_to_free_list( disk_page_no_t free_page )
{
    SHOW_FLOW( 8, "Put to free %d", free_page);
    //spinlock_lock(&pager_freelist_lock, "put_to_free_list");
    hal_mutex_lock(&pager_freelist_mutex);
// DO NOT use superblock free_start!

    if( need_fsck )
    {
        SHOW_ERROR0( 1, " disk is insane, put_to_free_list skipped...");
        //spinlock_unlock(&pager_freelist_lock, "put_to_free_list");
        hal_mutex_unlock(&pager_freelist_mutex);
        return;
    }

    if(!freelist_inited)
    {
        if(superblock.free_list == 0 )
        {
            pager_format_empty_free_list_block( free_page );
            superblock.free_list = free_page;
#if !USE_SYNC_IO // format_empty assigns free_head
            errno_t rc = disk_page_io_load_sync(&freelist_head,superblock.free_list);
            if( rc ) panic("Freelist IO error");
#endif
            freelist_inited = 1;

            hal_mutex_unlock(&pager_freelist_mutex);
            pager_update_superblock();
            return; // we used free page as freelist root, done now
        }
        else
        {
#if USE_SYNC_IO
            //errno_t rc = phantom_sync_read_block( pp, &u.free_head, superblock.free_list, 1 );
            //if( rc ) panic( "empty freelist block read error!" );
            read_freehead_sure();

#else
            errno_t rc = disk_page_io_load_sync(&freelist_head,superblock.free_list);
            if( rc ) panic("Freelist IO error");
            freelist_inited = 1;
#endif
        }
    }

#if USE_SYNC_IO
    struct phantom_disk_blocklist *list = &u.free_head;
#else
    struct phantom_disk_blocklist *list = (struct phantom_disk_blocklist *) disk_page_io_data(&freelist_head);
#endif

    if( list->head.used >= N_REF_PER_BLOCK )
        {
#if USE_SYNC_IO
            //errno_t rc = phantom_sync_write_block( pp, &u.free_head, superblock.free_list, 1 );
            //if( rc ) panic( "empty freelist block write error!" );
            write_freehead_sure( superblock.free_list );

#else
            errno_t rc = disk_page_io_save_sync(&freelist_head,superblock.free_list);
            //write_freehead_sure( superblock.free_list );
            if( rc ) panic("pager_put_to_free_list disk write error");

#endif
            pager_format_empty_free_list_block( free_page );
            superblock.free_list = free_page;

            hal_mutex_unlock(&pager_freelist_mutex);
            return;
        }
    else
        list->list[list->head.used++] = free_page;

    hal_mutex_unlock(&pager_freelist_mutex);
}

void
pager_init_free_list()
{
    //spinlock_lock(&pager_freelist_lock, "init_free_list");
    hal_mutex_lock(&pager_freelist_mutex);

    if(freelist_inited)
    {
        SHOW_FLOW0( 1, "already done - skipped");

        hal_mutex_unlock(&pager_freelist_mutex);
        return;
    }

    SHOW_FLOW0( 2, "start");

    if( need_fsck )
        panic("disk is insane, can't init freelist...");

    if(superblock.free_list == 0 )
        panic("superblock.free_list == 0, can't init freelist...");

#if USE_SYNC_IO
    //errno_t rc = phantom_sync_read_block( pp, &u.free_head, superblock.free_list, 1 );
    //if( rc ) panic( "empty freelist block read error!" );
    read_freehead_sure();
#else
    errno_t rc = disk_page_io_load_sync(&freelist_head,superblock.free_list);
    if( rc ) panic("Freelist IO error");
#endif
    freelist_inited = 1;

    hal_mutex_unlock(&pager_freelist_mutex);
    SHOW_FLOW0( 2, " ...DONE");
}


int
pager_interrupt_alloc_page(disk_page_no_t *out)
{
    SHOW_FLOW0( 11, "Interrupt Alloc page... ");
    if(!freelist_inited) return 0; // can't happen

    hal_mutex_lock(&pager_freelist_mutex);

    if( free_reserve_n )
        {
        *out = free_reserve[--free_reserve_n];
        // TODO: Schedule a DPC to refill ASAP!
        hal_mutex_unlock(&pager_freelist_mutex);
        return 1;
        }

    hal_mutex_unlock(&pager_freelist_mutex);
    return 0;
}

// called with     spinkey key(freelist_lock, "...");
void
pager_refill_free_reserve()
{
    SHOW_FLOW0( 10, "Refill free reserve... ");
#if USE_SYNC_IO
    struct phantom_disk_blocklist *list = &u.free_head;
#else
    struct phantom_disk_blocklist *list = (struct phantom_disk_blocklist *) disk_page_io_data(&freelist_head);
#endif

    while( free_reserve_n < free_reserve_size )
        {

        if( list->head.used > 0 )
            free_reserve[free_reserve_n++] = list->list[--list->head.used];
        else
            {
            if(list->head.next == 0 || list->head.next == superblock.free_list)
                break; // that was last one, we will not kill freelist head

            free_reserve[free_reserve_n++] = superblock.free_list;
            superblock.free_list = list->head.next;
#if USE_SYNC_IO
            //errno_t rc = phantom_sync_read_block( pp, &u.free_head, superblock.free_list, 1 );
            //if( rc ) panic( "empty freelist block read error!" );
            read_freehead_sure();

#else
            errno_t rc = disk_page_io_load_sync(&freelist_head,superblock.free_list);
            if( rc ) panic("Freelist IO error");
#endif

            if( list->head.magic != DISK_STRUCT_MAGIC_FREEHEAD ||
                list->head.used > N_REF_PER_BLOCK ||
                list->head._reserved != 0
                )
                {
                printf("Free list head values are insane, need fsck\n");
                list->head.used = 0;
                list->head.next = 0;
                list->head.magic = 0;
                // BUG! Need some way out of here. Reboot is ok, btw.
                // We are possibly still able to do a snap because
                // superblock.free_start allocations are possibly available
                // (btw we better try to reclaim superblock.free_start space
                // in free()), and we can find out if we can make one
                // more snap...
                break;
                }

            pager_update_superblock();
            }
        }

    while( free_reserve_n < free_reserve_size && superblock.free_start < superblock.disk_page_count )
        free_reserve[free_reserve_n++] = superblock.free_start++;

        // BUG!
        // In fact, it is enough to have async save here.
        //freelist_head.save_sync(superblock.free_list);
}

void
pager_refill_free()
{
    SHOW_FLOW0( 3, "pager_refill_free... ");
    if(!freelist_inited) pager_init_free_list();
    hal_mutex_lock(&pager_freelist_mutex);

    pager_refill_free_reserve();
    hal_mutex_unlock(&pager_freelist_mutex);
}


int
pager_alloc_page(disk_page_no_t *out_page_no)
{
    SHOW_FLOW0( 11, "Alloc page... ");
    if(!freelist_inited) pager_init_free_list();

    hal_mutex_lock(&pager_freelist_mutex);

    pager_refill_free_reserve();

    if( free_reserve_n )
    {
        *out_page_no = free_reserve[--free_reserve_n];

        hal_mutex_unlock(&pager_freelist_mutex);
        SHOW_FLOW( 11, " ...alloc DONE: %d", *out_page_no);
        STAT_INC_CNT(STAT_PAGER_DISK_ALLOC);
        return 1;
    }

    hal_mutex_unlock(&pager_freelist_mutex);
    SHOW_FLOW0( 11, " ...alloc FAILED");
    return 0;
}

void
pager_free_page( disk_page_no_t page_no )
{
    SHOW_FLOW0( 11, "Free page... ");
    STAT_INC_CNT(STAT_PAGER_DISK_FREE);

    if( ((unsigned long)page_no) < ((unsigned long)pager_superblock_ptr()->disk_start_page))
        panic("Free: freeing block below disk start: %ld < %ld", (unsigned long)page_no, (unsigned long)pager_superblock_ptr()->disk_start_page );

#if 0
    int i;
    for( i = 0; i < (sizeof(sb_default_page_numbers)/sizeof(sb_default_page_numbers[0])); i ++ )
    {
        if(page_no == sb_default_page_numbers[i])
            SHOW_ERROR( 0, "tried to free possible superblock @%d", page_no );
    }
#endif
    if(
       ((pager_superblock_ptr()->sb2_addr != 0) && (page_no == pager_superblock_ptr()->sb2_addr) )
       ||
       ((pager_superblock_ptr()->sb3_addr != 0) && (page_no == pager_superblock_ptr()->sb3_addr) )
      )
    {
        SHOW_ERROR( 0, "tried to free superblock @%d", page_no );
        panic("tried to free superblock");
    }


    // TODO? We can increment superblock.free_start if it looks approptiate?
    pager_put_to_free_list( page_no );
}


// ---------------------------------------------------------------------------
//
// FS check
//
// ---------------------------------------------------------------------------


// general FS sanity check
int
pager_fast_fsck()
{
    pager_init_free_list();
#if USE_SYNC_IO
    struct phantom_disk_blocklist *flist = &u.free_head;
#else
    struct phantom_disk_blocklist *flist = (struct phantom_disk_blocklist *) disk_page_io_data(&freelist_head);
#endif
    if( flist->head.magic != DISK_STRUCT_MAGIC_FREEHEAD )
        {
        need_fsck = 1;
        SHOW_ERROR0( 0, "Free list magic is wrong, need fsck");
        return 0;
        }

    if(
       flist->head.used > N_REF_PER_BLOCK ||
       flist->head._reserved != 0
       )
        {
        need_fsck = 1;
        SHOW_ERROR0( 0, "Free list head values are insane, need fsck\n");
        return 0;
        }

    // check (and invalidate?) pagelist heads here, etc.

    return 1;
}


int
pager_long_fsck()
{
    return 0;
}




