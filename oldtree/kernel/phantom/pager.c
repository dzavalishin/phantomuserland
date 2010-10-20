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
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <assert.h>

#include <kernel/vm.h>
#include <kernel/stats.h>

#include <phantom_disk.h>

#include "pager.h"
#include "pagelist.h"


static int _DEBUG = 0;



static hal_mutex_t              pager_mutex;
static hal_mutex_t              pager_freelist_mutex;




static void                	pager_io_done(void);
static void 			page_device_io_final_callback(void);



#define free_reserve_size  32


static paging_device            *pdev;

static pager_io_request         *pager_current_request = 0;

static pager_io_request         *pagein_q_start = 0;    // points to first (will go now) page
static pager_io_request         *pagein_q_end = 0;      // points to last page
//static int                        pagein_q_len;

static pager_io_request         *pageout_q_start = 0;
static pager_io_request         *pageout_q_end = 0;
//static int                      pageout_q_len;

static int                      pagein_is_in_process = 0;
static int                      pageout_is_in_process = 0;


static phantom_disk_superblock      superblock;
static int                          need_fsck;


static disk_page_io                 freelist_head;
static int                          freelist_inited = 0;
static disk_page_no_t                   free_reserve[free_reserve_size]; // for interrupt time allocs
static int                          free_reserve_n;


static disk_page_io                 superblock_io;


static disk_page_no_t     sb_default_page_numbers[] = DISK_STRUCT_SB_OFFSET_LIST;








// called when io is done on page device
/* TODO: must be called in DPC context! */
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




phantom_disk_superblock *
pager_superblock_ptr()
{
    return &superblock;
}













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

// ---------------------------------------------------------------------------
//
// Superblock works
//
// ---------------------------------------------------------------------------

int read_superblock(phantom_disk_superblock *out, disk_page_no_t addr )
{
    disk_page_io sb;

    disk_page_io_init( &sb );
    disk_page_io_load_sync(&sb, addr); // BUG! Must check for read error

    if( phantom_calc_sb_checksum( (phantom_disk_superblock *)disk_page_io_data(&sb) ))
        {
        *out = * ((phantom_disk_superblock *)disk_page_io_data(&sb));

        disk_page_io_finish( &sb );
        return 1;
        }

    disk_page_io_finish( &sb );
    return 0;
}

int find_superblock(
    phantom_disk_superblock *out,
    disk_page_no_t *pages, int n_pages,
    disk_page_no_t exclude, disk_page_no_t *where )
{
// BUG! We must check somehow for disk page number to be in our disk
// limits here. Lets hope driver will do it for us. But, then, we have to
// check for IO errors somehow!

    int i;
    for( i = 0; i < n_pages; i++ )
    {
        disk_page_no_t curr = pages[i];
        if( curr == 0 || curr == exclude )
            continue;

        hal_printf("Looking for superblock at %d... ", curr );

        if( read_superblock( out, curr ) )
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

    // BUG! Use some more sophisticated selection alg.
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
    superblock.free_list = free;    // head of free list

    /*
    {
    phantom_disk_blocklist freelist;

    freelist.head.magic = DISK_STRUCT_MAGIC_FREEHEAD;
    freelist.head.used = 0;
    freelist.head.next = 0;

    *( (phantom_disk_blocklist *)freelist_head.data() ) = freelist;
    freelist_head.save_sync(free);
    }
    */

    pager_format_empty_free_list_block( free );

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

    hal_printf("Checking superblock...");
    //getchar();

    {
        disk_page_io                sb0;

        disk_page_io_init(&sb0);

        //if(_DEBUG) hal_printf(" Load root sb");
        disk_page_io_load_sync(&sb0,sb_default_page_numbers[0]); // read block 0x10
        //if(_DEBUG) hal_printf(" ...DONE\n");

        root_sb = * ((phantom_disk_superblock *) disk_page_io_data(&sb0));

        disk_page_io_finish(&sb0);
    }

    root_sb_cs_ok = phantom_calc_sb_checksum( &root_sb );

    SHOW_FLOW( 0, "root sb sys name = '%.*s', checksum %s\n",
               DISK_STRUCT_SB_SYSNAME_SIZE, root_sb.sys_name,
               root_sb_cs_ok ? "ok" : "wrong"
              );


    sb_found_page_numbers[0] = root_sb.sb2_addr;
    
    sb_found_page_numbers[1] = root_sb.sb3_addr;
    

    disk_page_no_t     found_sb1;
    disk_page_no_t     found_sb2;

    if(_DEBUG) hal_printf("Find sb1");
    
    sb1_ok = find_superblock( &sb1, sb_found_page_numbers, 2, 0, &found_sb1 );
    
    if(_DEBUG) hal_printf(" ...DONE\n");

    if( sb1_ok )
    {
        if(_DEBUG) hal_printf("Find sb2");
        sb2_ok = find_superblock( &sb2, sb_found_page_numbers, 2, found_sb1, &found_sb2  );
        if(_DEBUG) hal_printf(" ...DONE\n");
    }

    hal_printf("sb1 status %d, sb2 - %d, sb3 - %d\n", root_sb_cs_ok, sb1_ok, sb2_ok );
    hal_printf("root sb sb2 addr %d, sb3 addr %d, free start %d, free list %d, fs clean %d\n",
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
        printf(" incomplete filesystem found, fixing...");
        superblock = root_sb;
        pager_fix_incomplete_format();
        printf(" Ok!\n");

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
#endif
    panic("I don't have any fsck yet. I must die.\n");
}




void
pager_update_superblock()
{
    SHOW_FLOW0( 0, " updating superblock...");

    assert(!(superblock_io.req.flag_pagein || superblock_io.req.flag_pageout));

    phantom_calc_sb_checksum( &superblock );
    *((phantom_disk_superblock *)disk_page_io_data(&superblock_io)) = superblock;

    disk_page_io_save_sync(&superblock_io, sb_default_page_numbers[0]); // save root copy

    if( need_fsck )
    {
        SHOW_ERROR0( 0, " disk is insane, will update root copy only (who called me here, btw?)...\n");
    }
    else
    {
        disk_page_io_save_sync(&superblock_io, superblock.sb2_addr);
        disk_page_io_save_sync(&superblock_io, superblock.sb3_addr);
    }

    SHOW_FLOW0( 0, " saved all 3");
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

    freelist.head.magic = DISK_STRUCT_MAGIC_FREEHEAD;
    freelist.head.used = 0;
    freelist.head.next = superblock.free_list;

    *( (struct phantom_disk_blocklist *)disk_page_io_data(&freelist_head) ) = freelist;
    disk_page_io_save_sync(&freelist_head,fp);
}

void pager_flush_free_list(void)
{
    hal_mutex_lock(&pager_freelist_mutex);
    disk_page_io_save_sync(&freelist_head,superblock.free_list);
    hal_mutex_unlock(&pager_freelist_mutex);
}

void
pager_put_to_free_list( disk_page_no_t free_page )
{
    SHOW_FLOW0( 10, "Put to free... ");
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
            disk_page_io_load_sync(&freelist_head,superblock.free_list);
            freelist_inited = 1;

            hal_mutex_unlock(&pager_freelist_mutex);
            pager_update_superblock();
            return; // we used free page as freelist root, done now
            }
        else
            {
            disk_page_io_load_sync(&freelist_head,superblock.free_list);
            freelist_inited = 1;
            }
        }

    struct phantom_disk_blocklist *list = (struct phantom_disk_blocklist *) disk_page_io_data(&freelist_head);

    if( list->head.used >= N_REF_PER_BLOCK )
        {
            disk_page_io_save_sync(&freelist_head,superblock.free_list);
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
        SHOW_FLOW0( 1, "Pager init free list - already done - skipped");

        hal_mutex_unlock(&pager_freelist_mutex);
        return;
    }

    SHOW_FLOW0( 2, "Pager init free list... ");
    if( need_fsck )
        panic("disk is insane, can't init freelist...");

    if(superblock.free_list == 0 )
        panic("superblock.free_list == 0, can't init freelist...");

    disk_page_io_load_sync(&freelist_head,superblock.free_list);
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
        // BUG! Schedule a DPC to refill ASAP!
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
    struct phantom_disk_blocklist *list = (struct phantom_disk_blocklist *) disk_page_io_data(&freelist_head);

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
            disk_page_io_load_sync(&freelist_head,superblock.free_list);

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
    if(_DEBUG) hal_printf("pager_refill_free... ");
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

    if( ((unsigned long)page_no) < ((unsigned long)pager_superblock_ptr()->disk_start_page))
        panic("Free: freeing block below disk start: %ld < %ld", (unsigned long)page_no, (unsigned long)pager_superblock_ptr()->disk_start_page );


    // BUG! - check for other superblocks?
    if(page_no == 0) panic("tried to free superblock");
    // BUG! We can increment superblock.free_start if it looks approptiate?
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
    struct phantom_disk_blocklist *flist = (struct phantom_disk_blocklist *) disk_page_io_data(&freelist_head);

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




