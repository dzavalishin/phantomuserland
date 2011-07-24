/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Paging IO through the new partitioning disk IO stack.
 *
 * Created on: 30.09.2010
 *
**/

#define DEBUG_MSG_PREFIX "part_pager_io"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/page.h>
#include <errno.h>
#include <threads.h>

#include "paging_device.h"
#include <hal.h>

#include <assert.h>
#include <string.h>




#if 0




void init_partition_paging_device(paging_device *me, phantom_disk_partition_t *p )
{
	me->private = p;

    int nPages = p->size * p->block_size / PAGE_SIZE;

    me->n_pages = nPages;
}







void
paging_device_start_read_rq (paging_device *me, pager_io_request *req, void (*callback)() )
{
	phantom_disk_partition_t *p = me->private;
	disk_enqueue( p, req );
}

void
paging_device_start_write_rq(paging_device *me, pager_io_request *req, void (*callback)() )
{
	phantom_disk_partition_t *p = me->private;
	disk_enqueue( p, req );
}


#endif
