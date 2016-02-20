
//---------------------------------------------------------------------------

#ifndef pagelistH
#define pagelistH

#include <phantom_disk.h>
#include <string.h>
#include <errno.h>

#include <pager_io_req.h>
#include <spinlock.h>
#include <kernel/sem.h>

//---------------------------------------------------------------------------

// all calls can sleep - not for interrupts
typedef struct disk_page_io
{
    pager_io_request    req; // NB! Must be first!

    int                 mem_allocated;
    void *              mem;

    hal_sem_t           done;

} disk_page_io;


errno_t disk_page_io_wait(disk_page_io *me);
void disk_page_io_release(disk_page_io *me);

void disk_page_io_allocate(disk_page_io *me);

void disk_page_io_load_me_async(disk_page_io *me);
void disk_page_io_save_me_async(disk_page_io *me);

void disk_page_io_callback(disk_page_io *me);


static __inline__ void disk_page_io_init(disk_page_io *me) 
{ 
    memset( me, 0, sizeof(disk_page_io) );
    me->mem_allocated = 0;
    //me->req.pager_callback = (void *)disk_page_io_callback;
    pager_io_request_init(&me->req);
    hal_sem_init( &(me->done), "dpio" );
}

static __inline__ void disk_page_io_finish(disk_page_io *me) { disk_page_io_release(me); }

static __inline__ void disk_page_io_seek(disk_page_io *me, disk_page_no_t dpage ) { me->req.disk_page = dpage; }
static __inline__ disk_page_no_t disk_page_io_tell(disk_page_io *me) { return me->req.disk_page; }

static __inline__ void *disk_page_io_data(disk_page_io *me) { disk_page_io_allocate(me); return me->mem; }


static __inline__ void disk_page_io_load_async(disk_page_io *me, disk_page_no_t dpage ) { disk_page_io_seek( me, dpage ); disk_page_io_load_me_async(me); }
static __inline__ void disk_page_io_save_async(disk_page_io *me, disk_page_no_t dpage ) { disk_page_io_seek( me, dpage ); disk_page_io_save_me_async(me); }

static __inline__ errno_t disk_page_io_load_sync(disk_page_io *me, disk_page_no_t dpage )  { disk_page_io_load_async( me, dpage ); return disk_page_io_wait(me); }
static __inline__ errno_t disk_page_io_save_sync(disk_page_io *me, disk_page_no_t dpage )  { disk_page_io_save_async( me, dpage ); return disk_page_io_wait(me); }

static __inline__ void disk_page_io_load_me_sync(disk_page_io *me) { disk_page_io_load_me_async(me); disk_page_io_wait(me); }
static __inline__ void disk_page_io_save_me_sync(disk_page_io *me) { disk_page_io_save_me_async(me); disk_page_io_wait(me); }





typedef struct disk_page_cache
{
    disk_page_io    io;
    int             dirty;
} disk_page_cache;


static __inline__ void disk_page_cache_wait(disk_page_cache *me)
{
    disk_page_io_wait(&me->io);
}

static __inline__ void disk_page_cache_flush(disk_page_cache *me)
{
    if( me->dirty )
    {
        disk_page_cache_wait(me);
        disk_page_io_save_me_async(&me->io);
        me->dirty = 0;
    }
}

static __inline__ void disk_page_cache_init(disk_page_cache *me)
{
    me->dirty = 0;
    disk_page_io_init(&me->io);
}
static __inline__ void disk_page_cache_finish(disk_page_cache *me)
{
    disk_page_cache_flush(me);
    disk_page_cache_wait(me);
    disk_page_io_finish(&me->io);
} // we have to wait or it will panic

static __inline__ void disk_page_cache_seek_async( disk_page_cache *me, disk_page_no_t dpage )
{
    disk_page_cache_flush(me);
    disk_page_cache_wait(me);
    disk_page_io_seek( &me->io, dpage );
    disk_page_io_load_me_async(&me->io);
}
static __inline__ void disk_page_cache_seek_sync( disk_page_cache *me, disk_page_no_t dpage )
{
    disk_page_cache_flush(me);
    disk_page_cache_wait(me);
    disk_page_io_seek( &me->io, dpage );
    disk_page_io_load_me_sync(&me->io);
}
static __inline__ void disk_page_cache_seek_noread( disk_page_cache *me, disk_page_no_t dpage )
{
    disk_page_cache_flush(me);
    disk_page_cache_wait(me);
    disk_page_io_seek( &me->io, dpage );
}
static __inline__ disk_page_no_t disk_page_cache_tell(disk_page_cache *me)
{
    return disk_page_io_tell( &me->io );
}

static __inline__ void disk_page_cache_modified(disk_page_cache *me)
{
    me->dirty = 1;
}

static __inline__ void *disk_page_cache_data(disk_page_cache *me)
{
    disk_page_cache_wait(me);
    return disk_page_io_data(&me->io);
}


// Just a huge on-disk extensible array of pointers to disk pages
//
// Access is assumed to be mostly sequental

typedef struct pagelist
{
    int                     magic;

    disk_page_no_t          root_page;
    //disk_page_no_t           curr_page;

    disk_page_cache         curr_p;
    struct phantom_disk_blocklist* curr;

    int                     curr_displ;     // displacement of curr start from start
    int                     pos;
} pagelist;

//virtual void            save_head() { head_p.flush(); }
//virtual void            load_head() { head_p.seek_sync(root_page); }

//virtual void            save_curr() { head_p.flush(); }
//virtual void            load_curr() { if(curr_dirty) save_curr(); head_p.load_sync(curr_page); }
static __inline__ void            pagelist_flush(pagelist *me) { disk_page_cache_flush(&me->curr_p); }

void pagelist_init( pagelist *me, disk_page_no_t root_page, int  init, int magic );
static __inline__ void pagelist_finish(pagelist *me) { pagelist_flush(me); disk_page_cache_finish(&me->curr_p); }

//virtual disk_page_no_t   read( int offset ) = 0;
//virtual void            write( int offset, disk_page_no_t ) = 0;

//virtual disk_page_no_t   pop() = 0;
//virtual void            push( disk_page_no_t ) = 0;

//disk_page_no_t   root(void);


void            pagelist_clear(pagelist *me);
void            pagelist_write_seq( pagelist *me, disk_page_no_t );

static __inline__ void            pagelist_seek(pagelist *me) { me->curr_displ = me->pos = 0; disk_page_cache_seek_async( &me->curr_p, me->root_page ); }
//virtual disk_page_no_t   read_seq(void);
int             pagelist_read_seq( pagelist *me, disk_page_no_t *out );






#endif
