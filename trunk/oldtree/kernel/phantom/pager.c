//---------------------------------------------------------------------------

#include <assert.h>

// debug regs
//#include <x86/debug_reg.h>
#include <kernel/vm.h>

#include <phantom_disk.h>

#include "pager.h"
#include "pagelist.h"


static int _DEBUG = 0;



static hal_mutex_t  			pager_mutex;
static hal_mutex_t  			pager_freelist_mutex;







#define free_reserve_size  32


static paging_device       		*pdev;

static pager_io_request    		*pagein_q_start = 0;    // points to first (will go now or going now) page
static pager_io_request    		*pagein_q_end = 0;      // points to last page
//static int                 		pagein_q_len;

static pager_io_request    		*pageout_q_start = 0;
static pager_io_request    		*pageout_q_end = 0;
//static int                 		pageout_q_len;

static int                 		pagein_is_in_process = 0;
static int                 		pageout_is_in_process = 0;


static phantom_disk_superblock     	superblock;
static int                         	need_fsck;


static disk_page_io                	freelist_head;
static int                        	freelist_inited = 0;
static disk_page_no_t               	free_reserve[free_reserve_size]; // for interrupt time allocs
static int                         	free_reserve_n;




static disk_page_no_t     sb_default_page_numbers[] = DISK_STRUCT_SB_OFFSET_LIST;



      /*
__debug_set_pager_q_bp()
{
    //set_b0( (~0x3) & kvtophys(&pagein_q_start), DR7_LEN_4, DR7_RW_DATA );
    //set_b1( (~0x3) & kvtophys(&pageout_q_start), DR7_LEN_4, DR7_RW_DATA );

}      */









// called when io is done on page device
/* TODO: must be called in DPC context! */
void page_device_io_final_callback()
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


void
pager_io_done()
{ pager_stop_io(); }


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
    disk_page_io_save_sync(&freelist_head,superblock.free_list);

    superblock.fs_is_clean = 1; // mark as clean
    pager_update_superblock();
}



/*
void pager_free_io_resources(pager_io_request *req)
{

//BUG! Do we REALLY need it here?

// handle flag_io_phys_mem?

    if(req->free_vaddr) hal_v_address_space_free_page(req->virt_addr);
    req->free_vaddr = 0;

    if(req->free_mpage)  hal_free_phys_page(req->phys_page);
    req->free_mpage = 0;
}
*/

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

    //set_b0( (~0x3) & kvtophys(pagein_q_start), DR7_LEN_4, DR7_RW_DATA );
    //set_b1( (~0x3) & kvtophys(pageout_q_start), DR7_LEN_4, DR7_RW_DATA );


    // pagein has absolute priority as
    // system responce time depends on it - right?
    if(pagein_q_start)
    {
        pagein_is_in_process = 1;
        paging_device_start_read_rq( pdev, pagein_q_start, page_device_io_final_callback );
    }
    else if(pageout_q_start)
    {
        pageout_is_in_process = 1;
        paging_device_start_write_rq( pdev, pageout_q_start, page_device_io_final_callback );
    }
}




void pager_stop_io() // called after io is complete
{
    //hal_printf("pager_stop_io... ");
    hal_mutex_lock(&pager_mutex);

    if( (!pagein_is_in_process) && (!pageout_is_in_process) ) panic("stop when no io in pager");
    if( pagein_is_in_process    &&   pageout_is_in_process  ) panic("double io in stop_io");

//set_dr7(0); // turn off all debug registers
    if(pagein_is_in_process)
        {
        pager_io_request *last = pagein_q_start;

        //hal_printf("pager_stop_io pagein BEF rem Q START = 0x%X Q END = 0x%X... ", pagein_q_start, pagein_q_end);

        pager_debug_print_q();

        pagein_q_start = last->next_page;
        if(pagein_q_start == 0) pagein_q_end = 0;

        // free page resources allotted for pager activity?
        //free_io_resources(last);
        //hal_printf("pager_stop_io pagein AFT rem Q START = 0x%X Q END = 0x%X... ", pagein_q_start, pagein_q_end);

        last->flag_pagein = 0;
        if(last->pager_callback) last->pager_callback( last, 0 );
        //hal_wakeup( last );
        pagein_is_in_process = 0;
        }

    if(pageout_is_in_process)
        {
        pager_io_request *last = pageout_q_start;
        pageout_q_start = last->next_page;
        if(pageout_q_start == 0) pageout_q_end = 0;

        //hal_printf("pager_stop_io pageout bef callbk... " );
        // free page resources allotted for pager activity?
        //free_io_resources(last);

        last->flag_pageout = 0;
        if(last->pager_callback) last->pager_callback( last, 1 );
        //hal_printf("pager_stop_io pageout aft callbk... " );
        //hal_wakeup( last );
        pageout_is_in_process = 0;
        }

    pager_start_io();
    hal_mutex_unlock(&pager_mutex);
    }





void
pager_enqueue_for_pagein ( pager_io_request *p )
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

    //spinlock_lock( &pager_lock, "enqueue_for_pagein");
    hal_mutex_lock(&pager_mutex);

    p->flag_pagein = 1;
    p->next_page = 0;
    if( pagein_q_start == 0 ) { pagein_q_start = p; pagein_q_end = p; /*start_io();*/ }
    else { pagein_q_end->next_page = p; pagein_q_end = p; }

    //hal_printf("ENQ 4 pagein p->next_page 0x%X\n",               p->next_page              );

    pager_start_io();

    //spinlock_unlock( &pager_lock, "enqueue_for_pagein");
    hal_mutex_unlock(&pager_mutex);
}

// BUG!! CAN'T INSERT INTO THE BEGINNING OF Q! Q head is used in request finalization
// as a pointer to CURRENTLY FINISHED REQUEST!
void
pager_enqueue_for_pagein_fast ( pager_io_request *p )
{
#if 1
    pager_enqueue_for_pagein( p );
#else
    hal_printf("ENQ 4 pagein fast VA 0x%X PA 0x%X disk %d\n",
               p->virt_addr,
               p->phys_page,
               p->disk_page
              );

    if( p->flag_pagein || p->flag_pageout ) panic("enqueue_for_pagein_fast: page is already on pager queue");

    spinlock_lock( &pager_lock, "enqueue_for_pagein");

    p->flag_pagein = 1;
    if( pagein_q_start == 0 ) { pagein_q_start = p; pagein_q_end = p; p->next_page = 0; /*start_io();*/ }
    else { p->next_page = pagein_q_start; pagein_q_start = p; }
    pager_start_io();

    spinlock_unlock( &pager_lock, "enqueue_for_pagein");
#endif
}

void
pager_enqueue_for_pageout( pager_io_request *p )
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

    //spinlock_lock( &pager_lock, "enqueue_for_pageout");
    //hal_printf("ENQ 4 pgout bef lock... ");
    hal_mutex_lock(&pager_mutex);
    //hal_printf("ENQ 4 pgout aft lock... ");

    p->flag_pageout = 1;
    p->next_page = 0;
    if( pageout_q_start == 0 ) { pageout_q_start = p; pageout_q_end = p; /*start_io();*/ }
    else { pageout_q_end->next_page = p;  pageout_q_end = p; }
    pager_start_io();

    //spinlock_unlock( &pager_lock, "enqueue_for_pageout");
    hal_mutex_unlock(&pager_mutex);
    //hal_printf("ENQ 4 pgout done\n");
    }


// BUG!! CAN'T INSERT INTO THE BEGINNING OF Q! Q head is used in request finalization
// as a pointer to CURRENTLY FINISHED REQUEST!

void
pager_enqueue_for_pageout_fast( pager_io_request *p )
{
#if 1
    pager_enqueue_for_pageout( p );
#else
    hal_printf("ENQ 4 pageout req  0x%X  VA 0x%X PA 0x%X disk %d, Q START = 0x%X\n",
               p,
               p->virt_addr,
               p->phys_page,
               p->disk_page,
               pagein_q_start
              );

    if( p->flag_pagein || p->flag_pageout ) panic("enqueue_for_pageout_fast: page is already on pager queue");

    //spinlock_lock( &pager_lock, "enqueue_for_pageout");
    hal_mutex_lock(&pager_mutex);

    p->flag_pageout = 1;
    if( pageout_q_start == 0 ) { pageout_q_start = p; pageout_q_end = p; p->next_page = 0; /*start_io();*/ }
    else { p->next_page = pageout_q_start;  pageout_q_start = p; }
    pager_start_io();

    //spinlock_unlock( &pager_lock, "enqueue_for_pageout");
    hal_mutex_unlock(&pager_mutex);
#endif
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

    // FIXME object_space_address const is wrong?
    if( 0 == superblock.object_space_address )
        superblock.object_space_address = 0x1400000; // Win32 likes it to be here

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

    hal_printf("root sb sys name = '%.*s', checksum %s\n",
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
    printf(" (need fsck)...");
    printf(" default superblock copies are wrong, looking for more...");

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
#if 1
    disk_page_io sb;

    disk_page_io_init(&sb);
    printf(" updating superblock...");

    phantom_calc_sb_checksum( &superblock );
    *((phantom_disk_superblock *)disk_page_io_data(&sb)) = superblock;

    disk_page_io_save_sync(&sb,sb_default_page_numbers[0]); // save root copy

    if( need_fsck )
    {
        printf(" disk is insane, will update root copy only (who called me here, btw?)...\n");
    }
    else
    {
        disk_page_io_save_sync(&sb,superblock.sb2_addr);
        disk_page_io_save_sync(&sb,superblock.sb3_addr);
    }

    disk_page_io_finish(&sb);

    printf(" saved all 3\n");
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

    freelist.head.magic = DISK_STRUCT_MAGIC_FREEHEAD;
    freelist.head.used = 0;
    freelist.head.next = 0;

    *( (struct phantom_disk_blocklist *)disk_page_io_data(&freelist_head) ) = freelist;
    disk_page_io_save_sync(&freelist_head,fp);
}


void
pager_put_to_free_list( disk_page_no_t free_page )
{
    if(_DEBUG) hal_printf("Put to free... ");
    //spinlock_lock(&pager_freelist_lock, "put_to_free_list");
    hal_mutex_lock(&pager_freelist_mutex);
// DO NOT use superblock free_start!

    if( need_fsck )
    {
        printf(" disk is insane, put_to_free_list skipped...");
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
            pager_format_empty_free_list_block( free_page );
            list->head.next = free_page;
            disk_page_io_save_sync(&freelist_head,superblock.free_list);
            superblock.free_list = free_page;
            disk_page_io_load_sync(&freelist_head,superblock.free_list);

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
        if(_DEBUG) hal_printf("Pager init free list - already done - skipped\n");

        hal_mutex_unlock(&pager_freelist_mutex);
        return;
    }

    if(_DEBUG) hal_printf("Pager init free list... ");
    if( need_fsck )
        panic("disk is insane, can't init freelist...");

    if(superblock.free_list == 0 )
        panic("superblock.free_list == 0, can't init freelist...");

    disk_page_io_load_sync(&freelist_head,superblock.free_list);
    freelist_inited = 1;

    hal_mutex_unlock(&pager_freelist_mutex);
    if(_DEBUG) hal_printf(" ...DONE\n");
}


int
pager_interrupt_alloc_page(disk_page_no_t *out)
{
    if(_DEBUG) hal_printf("Interrupt Alloc page... ");
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
    if(_DEBUG) hal_printf("Refill free reserve... ");
    struct phantom_disk_blocklist *list = (struct phantom_disk_blocklist *) disk_page_io_data(&freelist_head);

    while( free_reserve_n < free_reserve_size )
        {

        if( list->head.used > 0 )
            free_reserve[free_reserve_n++] = list->list[--list->head.used];
        else
            {
            if( list->head.next == 0 )
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
    if(_DEBUG) hal_printf("Alloc page... ");
    if(!freelist_inited) pager_init_free_list();

    hal_mutex_lock(&pager_freelist_mutex);

    pager_refill_free_reserve();

    if( free_reserve_n )
    {
        *out_page_no = free_reserve[--free_reserve_n];

        hal_mutex_unlock(&pager_freelist_mutex);
        if(_DEBUG) hal_printf(" ...alloc DONE: %d\n", *out_page_no);
        return 1;
    }

    hal_mutex_unlock(&pager_freelist_mutex);
    if(_DEBUG) hal_printf(" ...alloc FAILED\n");
    return 0;
}

void
pager_free_page( disk_page_no_t page_no )
{
    if(_DEBUG) hal_printf("Free page... ");

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
        printf("Free list magic is wrong, need fsck\n");
        return 0;
        }

    if(
       flist->head.used > N_REF_PER_BLOCK ||
       flist->head._reserved != 0
       )
        {
        need_fsck = 1;
        printf("Free list head values are insane, need fsck\n");
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



