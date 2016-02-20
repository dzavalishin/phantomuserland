#include <assert.h>
#include <kernel/vm.h>
#include <kernel/page.h>

#include "pagelist.h"
#include "pager.h"
#include <hal.h>

static int _DEBUG = 0;

//---------------------------------------------------------------------------





void
disk_page_io_allocate(disk_page_io *me)
{
    if(me->mem_allocated) return;

    hal_pv_alloc( &(me->req.phys_page), &(me->mem), PAGE_SIZE );

    me->mem_allocated = 1;
}


void
disk_page_io_release(disk_page_io *me)
{
    if( me->req.flag_pagein || me->req.flag_pageout )
        panic("disk_page_cacher_release: operation is in progress");
    if(!me->mem_allocated) return;

    hal_pv_free( me->req.phys_page, me->mem, PAGE_SIZE );

    me->mem_allocated = 0;
}

errno_t
disk_page_io_wait(disk_page_io *me)
{

#if 0
    // BUG! RACES!
    while( req.flag_pagein || req.flag_pageout )
        hal_sleep( &req );
#else
    // BUG! polling!
    hal_sleep_msec( 1 ); 
    //phantom_scheduler_yield();
    while( me->req.flag_pagein || me->req.flag_pageout )
        hal_sleep_msec( 10 );
#endif
    return me->req.rc;
}

void disk_page_io_callback(disk_page_io *me)
{
    (void) me;
}


void
disk_page_io_load_me_async(disk_page_io *me)
{
    disk_page_io_allocate(me);
    disk_page_io_wait(me);
    // Can't call enqueue_for_pagein_fast in closed interrupts!
    pager_enqueue_for_pagein_fast( &me->req );
}


void
disk_page_io_save_me_async(disk_page_io *me)
{
    if(_DEBUG) hal_printf("disk_page_io alloc... ");
    disk_page_io_allocate(me);
    if(_DEBUG) hal_printf("disk_page_io wait... ");
    disk_page_io_wait(me);
    if(_DEBUG) hal_printf("disk_page_io kick pager... ");
    pager_enqueue_for_pageout_fast( &me->req );
    if(_DEBUG) hal_printf("disk_page_io after kick... ");
}


//---------------------------------------------------------------------------
//
// Pagelist
//
//---------------------------------------------------------------------------


hal_spinlock_t                pagelist_lock;

void pagelist_init( pagelist *me, disk_page_no_t root_page, int  _init, int magic )
{
    errno_t rc;

    if(_DEBUG) hal_printf("pagelist init... ");
    me->root_page = root_page;
    me->magic = magic;

    if(_DEBUG) hal_printf("disk_page_cache_init... ");
    disk_page_cache_init(&me->curr_p);
    if(_DEBUG) hal_printf("disk_page_cache_init OK... ");

    //if( _init ) init();
    if(_init)
    {
        disk_page_io            head_p;

        if(_DEBUG) hal_printf("pagelist: create empty... ");
        disk_page_io_init(&head_p);

        struct phantom_disk_blocklist* head = (struct phantom_disk_blocklist *)disk_page_io_data(&head_p);


        head->head.magic = me->magic;
        head->head.used = 0;
        head->head.next = 0;

        // to maker sure compiler will barf if field will be renamed
        head->head._reserved = 0;

        if(_DEBUG) hal_printf("pagelist saving head... ");
        rc = disk_page_io_save_sync(&head_p,root_page);
        if(rc) panic("IO error in pagelist_init");
        if(_DEBUG) hal_printf("pagelist releasing io... ");
        disk_page_io_release(&head_p);
        if(_DEBUG) hal_printf("pagelist create done... ");
    }

    if(_DEBUG) hal_printf("pagelist load head... ");
    disk_page_cache_seek_async( &me->curr_p, me->root_page ); //load_head();
    me->curr_displ = 0;

    me->curr = (struct phantom_disk_blocklist *)disk_page_cache_data(&me->curr_p);
    if(_DEBUG) hal_printf("pagelist init DONE\n");
}


void
pagelist_clear(pagelist *me)
{
    pagelist_flush(me);

    disk_page_no_t curr_page = me->root_page;

    while( curr_page )
        {
        disk_page_cache_seek_sync( &me->curr_p, curr_page );
        if(me->curr->head.next) pager_free_page(me->curr->head.next);
        curr_page = me->curr->head.next;
        }

    disk_page_cache_seek_sync( &me->curr_p, me->root_page );

    me->curr->head.next = 0;
    me->curr->head.used = 0;

    me->curr_displ = me->pos = 0;

    disk_page_cache_modified(&me->curr_p);
}


int 
pagelist_read_seq( pagelist *me, disk_page_no_t *out )
{
    unsigned int pagepos = me->pos - me->curr_displ;

    //if( pagepos < 0 || pagepos > N_REF_PER_BLOCK )
    if( pagepos > N_REF_PER_BLOCK )
        panic("pagelist_read_seq bad pos");

    if( pagepos == N_REF_PER_BLOCK )
        {
        if( me->curr->head.next == 0 ) return 0; // EOF
        disk_page_cache_seek_sync( &me->curr_p, me->curr->head.next );
        me->curr_displ += N_REF_PER_BLOCK;
        pagepos = 0;
        }

    if( pagepos >= me->curr->head.used ) return 0; // EOF

    *out = me->curr->list[pagepos];
    me->pos++;

    return 1;
}

void
pagelist_write_seq( pagelist *me, disk_page_no_t wr_data )
{
    unsigned int pagepos = me->pos - me->curr_displ;

    //if( pagepos < 0 || pagepos > N_REF_PER_BLOCK )
    if( pagepos > N_REF_PER_BLOCK )
        panic("pagelist_write_seq bad pos");

    if( pagepos == N_REF_PER_BLOCK )
        {
        if( me->curr->head.next == 0 )
            {
            if( !pager_alloc_page(&me->curr->head.next) )
                panic("out of disk space in pagelist");

            disk_page_cache_seek_noread( &me->curr_p, me->curr->head.next );
            me->curr->head.next = 0;
            me->curr->head.used = 0;
            me->curr->head.magic = me->magic;
            me->curr->head._reserved = 0;
            disk_page_cache_modified(&me->curr_p);
            }
        else
            disk_page_cache_seek_sync( &me->curr_p, me->curr->head.next );

        me->curr_displ += N_REF_PER_BLOCK;
        pagepos = 0;
        }

    me->curr->list[pagepos] = wr_data;
    me->curr->head.used = pagepos+1;
    me->pos++;
    disk_page_cache_modified(&me->curr_p);
}

