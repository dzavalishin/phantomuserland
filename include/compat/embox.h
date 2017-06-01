/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * Embox compatibility.
 *
**/

#ifndef __COMPAT_EMBOX_H__
#define __COMPAT_EMBOX_H__

#include <assert.h>

#include <phantom_libc.h>
#include <hal.h>
#include <time.h>

#include <kernel/cond.h>
#include <kernel/mutex.h>

typedef long blkno_t;

//extern void *phymem_alloc(size_t page_number);
//extern void phymem_free(void *page, size_t page_number);

//errno_t        				hal_alloc_phys_pages(physaddr_t  *result, int npages); // alloc and not map
//void        				hal_free_phys_pages(physaddr_t  page, int npages); // alloc and not map - WILL PANIC if page is mapped!


#define phymem_alloc( ___np ) ({ physaddr_t result = 0; errno_t rc = hal_alloc_phys_pages( &result, ___np); if( rc ) result = 0; result; })
#define phymem_free( ___pg, ___np ) hal_free_phys_pages( ___pg, ___np )



#define static_assert(x) assert(x)

#define log_debug printf



#define waitq_init(__c) hal_cond_init(__c,"embox")
#define waitq_wakeup_all hal_cond_broadcast


#endif // __COMPAT_EMBOX_H__
