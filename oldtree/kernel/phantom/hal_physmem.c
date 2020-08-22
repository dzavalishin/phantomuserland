/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Physical memory and virtual address space allocators.
 * This code is just an interface to actual allocator instances.
 * See physalloc.c
 *
**/

#define DEBUG_MSG_PREFIX "physmem"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 0


#include <kernel/config.h>
#include <kernel/physalloc.h>
#include <phantom_libc.h>
#include <string.h>
#include <threads.h>

#include <kernel/init.h>
#include <kernel/debug.h>
#include <kernel/stats.h>
#include <kernel/page.h>

#include <video/window.h> // for debug paint
#include <kernel/debug_graphical.h>

#include "vm_map.h"

// PHYSALLOC_MAXPAGES
#ifdef ARCH_ia32
#include <ia32/phantom_pmap.h>
#else
#  warning PHYSALLOC_MAXPAGES defined for 32bit 4K pages
#  define PHYSALLOC_MAXPAGES 0x400000
#endif

static void cmd_mem_stat( int ac, char **av );


#define USE_RESERVE 0

// Automatically reclaim mem when out of phys mem pages
#define AUTOFREE MEM_RECLAIM

// todo if we change memory allocation seriously, change kvtophys()
// and phystokv() as well

// -----------------------------------------------------------------------
// Virtual address space allocator. Allocates ADDRESSES only, not memory
// -----------------------------------------------------------------------



static physalloc_t   	vm_map;
static map_elem_t    	mapbuf[MAP_SIZE_ELEM(PHYSALLOC_MAXPAGES)];


//static void hal_init_physmem_alloc_thread(void);


void
hal_init_vm_map(void)
{
    phantom_phys_alloc_init_static( &vm_map, PHYSALLOC_MAXPAGES, mapbuf );

    phantom_phys_free_region( &vm_map,
                                 PHANTOM_AMAP_START_VADDR_POOL/PAGE_SIZE,
                                 PHANTOM_AMAP_SIZE_VADDR_POOL/PAGE_SIZE
                            );

    vm_map.allocable_size = PHANTOM_AMAP_SIZE_VADDR_POOL/PAGE_SIZE;
    //vm_map.n_used_pages = vm_map.allocable_size;
    vm_map.n_used_pages = 0;


    // No! Later! See main.c
    //hal_init_physmem_alloc_thread();
}

errno_t
hal_alloc_vaddress(void **result, int num) // alloc address of a page, but not memory
{
    physalloc_item_t ret;

    // TODO alloc_region steps by map_elem size, it takes 32 pages to fullfill request of 1 page!

    errno_t rc;

    if( num == 1)
        rc = phantom_phys_alloc_page( &vm_map, &ret );
    else
        rc = phantom_phys_alloc_region( &vm_map, &ret, num );


    if( rc )
    {
        *result = 0;
        return rc;
    }

    STAT_INC_CNT_N(STAT_CNT_VA_ALLOC, num);

    *result = (void *)(ret * PAGE_SIZE);
    //printf("hal_alloc_vaddress req %d pages, ret 0x%X (internal %d)\n", num, *result, ret );
    return rc;
}


void
hal_free_vaddress(void *addr, int num)
{
    assert( ( ((int)addr) & (PAGE_SIZE-1)) == 0 );
    STAT_INC_CNT_N(STAT_CNT_VA_FREE, num);
    phantom_phys_free_region( &vm_map, (int)((int)addr) / PAGE_SIZE, num );
}



void paint_vaspace_allocator_memory_map(window_handle_t w, rect_t *r )
{
    paint_allocator_memory_map(w, r, &vm_map );
}







// -----------------------------------------------------------------------
// Main physical memory allocator. Allocates whole pages only.
// -----------------------------------------------------------------------

static physalloc_t   	pm_map;  // Main arena
static physalloc_t   	low_map; // Memory below 1 Mb - special use only

static int total_phys_pages = 0;

#if USE_RESERVE
static hal_spinlock_t	pm_lock; // reenter lock

static physaddr_t 	physReserveGet();
static void 		replentishThread(void *arg);
#endif


void
hal_init_physmem_alloc(void)
{
#if USE_RESERVE
    hal_spin_init(&pm_lock);
#endif
    // FIXME 64 bit problem
    phantom_phys_alloc_init( &pm_map, PHYSALLOC_MAXPAGES ); // All the possible mem
    pm_map.n_used_pages = pm_map.allocable_size = 0;

    //phantom_phys_free_region( &pm_map, start/PAGE_SIZE, npages );

    // Unmap it for any case
    //hal_pages_control( start, 0, npages, page_unmap, page_noaccess );

    phantom_phys_alloc_init( &low_map, 0x100000/PAGE_SIZE ); // All the possible low mem
    low_map.n_used_pages = 0; //low_map.allocable_size = 0x100000/PAGE_SIZE;
    low_map.allocable_size = 0;

}

void
hal_init_physmem_alloc_thread(void)
{
#if USE_RESERVE
    hal_start_thread( replentishThread, 0, THREAD_FLAG_KERNEL );
#endif
    dbg_add_command(&cmd_mem_stat, "mem", "Physical memory stats");

}


void
hal_physmem_add( physaddr_t start, size_t npages )
{
    assert(!(start & (PAGE_SIZE - 1)));
    total_phys_pages += npages;
    pm_map.allocable_size += npages;
    pm_map.n_used_pages += npages;
    phantom_phys_free_region(&pm_map, start/PAGE_SIZE, npages);
}




errno_t
hal_alloc_phys_pages(physaddr_t *result, int npages) // alloc and not map
{
    physalloc_item_t ret;

#if USE_RESERVE
    int ie = hal_save_cli();
    hal_spin_lock(&pm_lock);
#endif
    int rc;

#if AUTOFREE
    while(1)
    {
        rc = phantom_phys_alloc_region( &pm_map, &ret, npages );
        if( rc == 0 )
            break;
        physmem_try_to_reclaim_page();
    }
#else
    rc = phantom_phys_alloc_region( &pm_map, &ret, npages );
#endif


#if USE_RESERVE
    hal_spin_unlock(&pm_lock);
    if(ie) hal_sti();
#endif

    if( rc )
    {
        *result = 0;
        return rc;
    }

    STAT_INC_CNT_N(STAT_CNT_PMEM_ALLOC, npages);


    *result = (physaddr_t)(ret * PAGE_SIZE);
//printf("PhysMem alloc %08X-%08X\n",           *result, (physaddr_t)((ret+npages) * PAGE_SIZE) );
    return rc;
}

void
hal_free_phys_pages(physaddr_t  paddr, int npages)
{
    assert( ( ((int)paddr) & (PAGE_SIZE-1)) == 0 );

    // Unmap it for any case - NO, IDIOT! It unmaps page 0!
    //hal_pages_control( paddr, 0, npages, page_unmap, page_noaccess );

    phantom_phys_free_region( &pm_map, (int) (paddr / PAGE_SIZE), npages );

    STAT_INC_CNT_N(STAT_CNT_PMEM_FREE, npages);
}




errno_t
hal_alloc_phys_page(physaddr_t  *result)
{
    physalloc_item_t ret;

#if USE_RESERVE
    // Reenter hadling. This shouldn't happen, but it is.
    // Usually from page fault. TODO fix it really.
    int ie = hal_save_cli();
    if(pm_lock.lock || pm_map.lock.lock)
    {
        SHOW_ERROR0( 0, "reenter");
        // Allocate from special pool
        *result = physReserveGet();
        if(ie) hal_sti();
        return 0;
    }

    hal_spin_lock(&pm_lock);
#endif
    int rc;

#if AUTOFREE
    while(1)
    {
        rc = phantom_phys_alloc_page( &pm_map, &ret );
        if( rc == 0 )
            break;
        physmem_try_to_reclaim_page();
    }
#else
        rc = phantom_phys_alloc_page( &pm_map, &ret );
#endif

#if USE_RESERVE
    hal_spin_unlock(&pm_lock);
    if(ie) hal_sti();
#endif
    if( rc )
    {
        *result = 0;
        return rc;
    }

    STAT_INC_CNT_N(STAT_CNT_PMEM_ALLOC, 1);

    *result = (physaddr_t)(ret * PAGE_SIZE);
//printf("PhysMem alloc %08X\n", *result );
    return rc;
}

void
hal_free_phys_page(physaddr_t  paddr) // alloc and not map - WILL PANIC if page is mapped!
{
    STAT_INC_CNT_N(STAT_CNT_PMEM_FREE, 1);
    hal_free_phys_pages(paddr, 1);
}


int phantom_phys_free_count( physalloc_t *arena )
{
    return total_phys_pages - arena->n_used_pages;
}


int have_lot_of_free_physmem()
{
    // 200 free pages are ok
    return phantom_phys_free_count( &pm_map ) > 200;
}

// Really out of physmem
int low_low_free_physmem()
{
    return phantom_phys_free_count( &pm_map ) < 10;
}

// Just need some more
int low_free_physmem()
{
    return phantom_phys_free_count( &pm_map ) < 100;
}

void paint_physmem_allocator_memory_map(window_handle_t w, rect_t *r )
{
    paint_allocator_memory_map(w, r, &pm_map );
}


// -----------------------------------------------------------------------
// Combined allocator interface. Allocates memory, address space for it,
// maps and returns both physical and virtual addresses.
// -----------------------------------------------------------------------


void hal_pv_alloc( physaddr_t *pa, void **va, int size_bytes )
{
    int npages = ((size_bytes-1)/PAGE_SIZE) + 1;

    if( hal_alloc_vaddress(va, npages) )
        panic("out of vaddr");

    if( hal_alloc_phys_pages( pa, npages) )
        panic("out of physmem");

    hal_pages_control( *pa, *va, npages, page_map, page_rw );
}

void hal_pv_free( physaddr_t pa, void *va, int size_bytes )
{
    int npages = ((size_bytes-1)/PAGE_SIZE) + 1;

    hal_pages_control( pa, va, npages, page_unmap, page_noaccess );

    hal_free_vaddress(va, npages);
    hal_free_phys_pages( pa, npages); // TODO BUG causes crash in VM86 if called before
}



// -----------------------------------------------------------------------
// Low ( < 1Mb, "DOS memory" ) physical memory allocator. Used by VM86.
// -----------------------------------------------------------------------


void
hal_physmem_add_low( physaddr_t start, size_t npages )
{
    SHOW_INFO( 1, "add low mem: %p, %d pages, %d kb", (void *)start, npages, npages*4 );
    phantom_phys_free_region( &low_map, start/PAGE_SIZE, npages );

    low_map.allocable_size += npages;
    low_map.n_used_pages += npages;

    // Unmap it for any case
    //hal_pages_control( start, 0, npages, page_unmap, page_noaccess );
}


errno_t
hal_alloc_phys_pages_low(physaddr_t *result, int npages) // alloc and not map
{
    physalloc_item_t ret;
    int rc = phantom_phys_alloc_region( &low_map, &ret, npages );
    if( rc )
    {
        *result = 0;
        return rc;
    }

    STAT_INC_CNT_N(STAT_CNT_LOMEM_ALLOC, npages);

    *result = (physaddr_t)(ret * PAGE_SIZE);
    //printf("hal_alloc_phys_pages_low req %d pages, ret 0x%X (internal %d)\n", npages, *result, ret );
    return rc;
}

void
hal_free_phys_pages_low(physaddr_t  paddr, int npages)
{
    assert( ( ((int)paddr) & (PAGE_SIZE-1)) == 0 );

    STAT_INC_CNT_N(STAT_CNT_LOMEM_FREE, npages);
    // Unmap it for any case
    //hal_pages_control( paddr, 0, npages, page_unmap, page_noaccess );
    phantom_phys_free_region( &low_map, (int)((int)paddr) / PAGE_SIZE, npages );
}







// -----------------------------------------------------------------------
// Helpers - memory copy from/to physical address. Creates temp mapping.
// -----------------------------------------------------------------------



void
memcpy_v2p( physaddr_t to, void *from, size_t size )
{
    void *addr;
    if(hal_alloc_vaddress( &addr, 1))
        panic("out of vaddresses");

#if 1 // panics, need test
    if(!PAGE_ALIGNED(to))
    {
        // Process partial page

        physaddr_t page = PREV_PAGE_ALIGN(to);
        int shift = to-page;
        size_t part = PAGE_SIZE-shift;

        if( part > size ) part = size;

        assert( shift < PAGE_SIZE );
        assert( shift > 0 );

        hal_page_control( page, addr, page_map, page_rw );

        memcpy( addr+shift, from, part );

        hal_page_control( page, addr, page_unmap, page_noaccess );

        to += part;
        from += part;
        size -= part;
    }
#endif

    if( size > 0 ) assert(PAGE_ALIGNED(to));

    while( size > 0 )
    {
        int stepSize = size > PAGE_SIZE ? PAGE_SIZE : size;
        size -= stepSize;

        hal_page_control( to, addr, page_map, page_rw );

        memcpy( addr, from, stepSize );

        hal_page_control( to, addr, page_unmap, page_noaccess );

        to += PAGE_SIZE;
        from += PAGE_SIZE;

    }

    assert( size == 0 );


    hal_free_vaddress(addr, 1);
}


void
memcpy_p2v( void *to, physaddr_t from, size_t size )
{
    void *addr;
    if(hal_alloc_vaddress( &addr, 1))
        panic("out of vaddresses");

    assert(PAGE_ALIGNED(from));

    do
    {
        int stepSize = size > PAGE_SIZE ? PAGE_SIZE : size;
        size -= stepSize;

        hal_page_control( from, addr, page_map, page_rw );

        memcpy( to, addr, stepSize );

        hal_page_control( from, addr, page_unmap, page_noaccess );

        to += PAGE_SIZE;
        from += PAGE_SIZE;

    } while( size > 0 );

    assert( size == 0 );


    hal_free_vaddress(addr, 1);
}




void
hal_copy_page_v2p( physaddr_t to, void *from )
{
    void *addr;
    if(hal_alloc_vaddress( &addr, 1))
        panic("out of vaddresses");
    hal_page_control( to, addr, page_map, page_rw );

    memcpy( addr, from, hal_mem_pagesize() );

    hal_page_control( to, addr, page_unmap, page_noaccess );
    hal_free_vaddress(addr, 1);
}

void
memzero_page_v2p( physaddr_t to )
{
    void *addr;
    if(hal_alloc_vaddress( &addr, 1))
        panic("out of vaddresses");
    hal_page_control( to, addr, page_map, page_rw );

    bzero( addr, hal_mem_pagesize() );

    hal_page_control( to, addr, page_unmap, page_noaccess );
    hal_free_vaddress(addr, 1);
}






#if USE_RESERVE

// -----------------------------------------------------------------------
// Physalloc interrupt-time helper. Can provide physical allocation
// when main one is locked.
// -----------------------------------------------------------------------



#define PHYS_RESERVE_SIZE 1024

static physaddr_t 		physReserve[PHYS_RESERVE_SIZE];
static int 			nPhysReserve = 0;
static hal_cond_t 		run;
static hal_mutex_t 		runm;
static hal_spinlock_t           pr_lock;


static void physReservePut( physaddr_t p )
{
    int ie = hal_save_cli();
    hal_spin_lock(&pr_lock);

    if( nPhysReserve >= PHYS_RESERVE_SIZE )
        panic("overreserve");

    physReserve[nPhysReserve++] = p;

    hal_spin_unlock(&pr_lock);
    if(ie) hal_sti();
}


static physaddr_t physReserveGet()
{
    int ie = hal_save_cli();
    hal_spin_lock(&pr_lock);

    if( nPhysReserve <= 0 )
        panic("underreserve");

    physaddr_t ret = physReserve[--nPhysReserve];
    SHOW_FLOW( 4, "allocating 0x%X from phys reserve", ret );
    hal_spin_unlock(&pr_lock);
    if(ie) hal_sti();

    hal_cond_broadcast( &run );

    return ret;
}


static void physReplentishReserve()
{

    while(nPhysReserve < PHYS_RESERVE_SIZE)
    {
        physaddr_t p;
        if( hal_alloc_phys_page( &p ) )
        {
            SHOW_ERROR0( 0 , "Out of physical memory filling allocator reserve");
            break;
        }
        physReservePut( p );
    }

}



static void replentishThread(void *arg)
{
    t_current_set_name("PhysResrv");
    hal_cond_init(&run, "PhysReserve");
    hal_mutex_init(&runm, "PhysReserve");
    hal_spin_init(&pr_lock);

    while(1)
    {
        SHOW_FLOW0( 4, "refilling phys reserve" );
        physReplentishReserve();

        hal_mutex_lock(&runm);
        hal_cond_wait(&run, &runm);
        hal_mutex_unlock(&runm);
    }

}

#endif // USE_RESERVE


#define PG2MB(__p, __div) ((int) ( ((__p) * 4L) / (__div) ))

#define PG2KB(__p) ((__p) * 4)

static void dump_mem_stat( physalloc_t *map, const char *name, long div, const char *unit )
{
    int total = PG2MB(map->allocable_size, div);
    int free = PG2MB( (map->allocable_size - map->n_used_pages), div );
    int used = PG2MB(map->n_used_pages, div);
    printf("  %-14s: %5d %s, %5d %s free, %5d %s used\n", name, total, unit, free, unit, used, unit );
}

size_t pahantom_total_phys_mem_kb() { return PG2KB(pm_map.allocable_size); }
size_t pahantom_free_phys_mem_kb() { return PG2KB( pm_map.allocable_size - pm_map.n_used_pages ); }



static void cmd_mem_stat( int ac, char **av )
{
    (void) ac;
    (void) av;

    dump_mem_stat( &vm_map, "Address space", 1024L, "Mb" );
    dump_mem_stat( &pm_map, "Hi mem", 1024L, "Mb"  );
    dump_mem_stat( &low_map, "Low mem", 1L, "Kb"  );
}







