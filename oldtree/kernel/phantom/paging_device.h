/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Paging IO
 *
 * TODO: separate queues for critical and usual IO. Disk address sort of request.
 *
 *
**/

//---------------------------------------------------------------------------

#ifndef paging_deviceH
#define paging_deviceH

#include <phantom_libc.h>
#include <phantom_types.h>

#include <pager_io_req.h>
#include <kernel/dpc.h>

//#define REMAPPED_PAGING_IO 0
#define SIMPLE_IDE_DRIVER 1

//---------------------------------------------------------------------------
struct vm_page;

//#define REMAPPED_PAGING_IO (1)

// Actually, not a device, but paging io interface
typedef struct paging_device
{
    int                 n_pages;

    int                 io_is_in_progress;
    int                 is_write;

    disk_page_no_t      disk;           // Disk block number (incoming parameter)
    physaddr_t          mem;

    void                (*callback)();

    dpc_request         io_done_dpc;

    int 		tid;  // IO thread for our disk

// Don't change struct format
//#if REMAPPED_PAGING_IO
    void * 		io_vaddr; // Place in address space to map page for IO to
//#endif

    hal_cond_t		start_io_sema;

    hal_mutex_t         mutex;

    void *		private; // Private part of impl
} paging_device;


static __inline__ int
paging_device_can_grow(paging_device *me) {     (void) me; return 0; } // can I grow pagespace

//static __inline__ void
//paging_device_io_completion_interrupt(paging_device *me) { dpc_request_trigger( &me->io_done_dpc, me); }

static __inline__     int
paging_device_pages_available(paging_device *me) { return me->n_pages; } // size in pages

//void        	paging_device_start_io(paging_device *me);
//void        	paging_device_io_done(paging_device *me);



/* TODO: Add error variable to callback */
//void                paging_device_start_read (paging_device *me, disk_page_no_t disk, phys_page_t mem, void *vmem, void (*callback)() );
//void                paging_device_start_write(paging_device *me, disk_page_no_t disk, phys_page_t mem, void *vmem, void (*callback)() );
void                paging_device_start_read (paging_device *me, disk_page_no_t disk, physaddr_t mem, void (*callback)() );
void                paging_device_start_write(paging_device *me, disk_page_no_t disk, physaddr_t mem, void (*callback)() );

void                paging_device_start_read_rq (paging_device *me, pager_io_request *, void (*callback)() );
void                paging_device_start_write_rq(paging_device *me, pager_io_request *, void (*callback)() );

// Called in interrupt time, will result in calling to
// io_done in dpc state which will, in turn, call our io_done

void  		init_paging_device(paging_device *me, const char *linux_devname, int n_pages );



// -----------------------------------------------------------------------
// Debugging
// -----------------------------------------------------------------------


void phantom_check_disk_save_virtmem( void *start, int npages );
void phantom_check_disk_check_virtmem( void *start, int npages );


#endif
