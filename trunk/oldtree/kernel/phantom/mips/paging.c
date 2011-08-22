/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS page table support
 *
**/

#define DEBUG_MSG_PREFIX "paging"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/config.h>
#include <kernel/init.h>
#include <malloc.h>
#include <assert.h>
#include <phantom_libc.h>

#include <mips/pmap.h>
#include <kernel/page.h>
#include <kernel/trap.h>
#include <hal.h>
#include <machdep.h>

/**
 *
 * MIPS has no pagetable walk logic - on TLB miss exception occurs and
 * kernel must bring in new TLB entry. We keep simple linear map of TLB
 * entries in an array. Page size is 4K, so we need 0x100000 entries to
 * cover 4Gb. (NPTE)
 *
**/

static void phantom_paging_start(void);


// TODO stat TLB miss

// http://people.csail.mit.edu/rinard/osnotes/h11.html

static int paging_inited;

static pt_entry_t ptab[NPTE];



#define lin2linear_ptenum(a) ( ((a) / 4096) )
#define get_pte( la ) (ptab + lin2linear_ptenum(la))




void phantom_paging_init(void)
{
    memset( ptab, 0, sizeof ptab );

    // needed by phantom_map_mem_equally();
    paging_inited = 1;

    phantom_map_mem_equally();

    SHOW_FLOW0( 2, "mem mapped" );

    phantom_paging_start();

    //SHOW_FLOW0( 2, "paging started" );
}


static void phantom_paging_start(void)
{
    // MIPS has no specific unpaged mode, paging is active all the time
    // SHOW_ERROR0( 2, "NOT impl paging start" );
}




//* Clear TLB
static void clear_tlb()
{
    tlb_entry_t te;
    memset( &te, 0, sizeof te );

    int ie = hal_save_cli();

    int i;
    for( i = 0; i < NTLBE; i++ )
        mips_tlb_write_index( i, &te );

    if(ie) hal_sti();
}



// TODO use mips_tlb_probe
//* Clear entry for given virtual address. NB! Ignores 8 bit address space id field!
static void clear_tlb_entry( addr_t va )
{
    tlb_entry_t te;

    if( va & (PAGE_SIZE-1) )
        SHOW_ERROR0( 2, "va low bits?" );

    //addr_t pageno = va >> PAGE_SHIFT;

    int ie = hal_save_cli();

    int i;
    for( i = 0; i < NTLBE; i++ )
    {
        mips_tlb_read( i, &te );

        // compare pageno to TLB va bits
        if( (te.v & TLB_V_ADDR_MASK) == (va & TLB_V_ADDR_MASK ) )
        {
            // Clear entry
            memset( &te, 0, sizeof te );
            mips_tlb_write_index( i, &te );
        }
    }

    if(ie) hal_sti();
}


// unused
#if 0
// TODO use mips_tlb_probe
//* Read entry for given virtual address. NB! Ignores 8 bit address space id field!
static errno_t read_tlb_entry( tlb_entry_t *e, addr_t va )
{
    tlb_entry_t te;

    assert_int_disabled();

    if( va & (PAGE_SIZE-1) )
        SHOW_ERROR0( 2, "va low bits?" );

    //addr_t pageno = va >> PAGE_SHIFT;

    int i;
    for( i = 0; i < NTLBE; i++ )
    {
        mips_tlb_read( i, &te );

        if( (te.v & TLB_V_ADDR_MASK) == (va & TLB_V_ADDR_MASK ) )
        {
            *e = te;
            return 0;
        }
    }

    return ENOENT;
}
#endif

/*
//! Write entry for given virtual address. NB! Ignores 8 bit address space id field!
static void write_tlb_entry( tlb_entry_t *e, void *va )
{
    (void) va;
    //write_tlb_entry_rnd( e );
    mips_tlb_write_random( e );
}
*/

// TODO on MIPS we can manually put new entry to TLB. make revalidate_pg()?
static __inline__ void invlpg(addr_t start)
{
    clear_tlb_entry( start );
#warning clear corresp cache entries
}


//* Reload TLB entry with correct data
errno_t load_tlb_entry( addr_t va, int write )
{
    clear_tlb_entry( va );

    tlb_entry_t e;

    pt_entry_t *e0 = get_pte( va & TLB_V_ADDR_MASK ); // low entry
    pt_entry_t *e1 = get_pte( (va & TLB_V_ADDR_MASK) | TLB_V_ADDR_HI ); // hi entry

    e.v = e0->v & TLB_V_ADDR_MASK; // One bit less than pte
    e.p0 = e0->p;
    e.p1 = e1->p;

    // Check if resulting PTE (not TLBE) is valid, if no - we have page fault
    int is_hi = va & TLB_V_ADDR_HI;

    if( is_hi )
    {
        if( ! (e.p1 & TLB_P_VALID) )
            return EFAULT;

        if( write && ! (e.p1 & TLB_P_DIRTY) )
            return EROFS;
    }
    else
    {
        if( ! (e.p0 & TLB_P_VALID) )
            return EFAULT;

        if( write && ! (e.p0 & TLB_P_DIRTY) )
            return EROFS;
    }

    mips_tlb_write_random( &e );
    return 0;
}


//! Called from exception handler on TLB refill events
void tlb_refill_exception(struct trap_state *ts)
{
    int write = ts->trapno == T_TLB_STORE;
    addr_t va = ts->va;
    errno_t rc = load_tlb_entry( va, write );

    // Load was successfull? We got correct mapping in TLB?
    if( rc == 0 )
        return;

    // No good mapping found. Page fault.
    vm_map_page_fault_trap_handler(ts);

    // Attempt to load entry again - page fault handler might provide mapping...
    load_tlb_entry( va, write );
}
















void phantom_map_page(linaddr_t la, pt_entry_t *mapping )
{
    assert(PAGE_ALIGNED(la));
    assert(paging_inited);

    *get_pte(la) = *mapping;

    invlpg(la);
}

static __inline__ void clear_pt_entry( pt_entry_t *e )
{
    memset( e, 0, sizeof(*e) );
}

void phantom_unmap_page(linaddr_t la )
{
    assert(PAGE_ALIGNED(la));
    assert(paging_inited);

    clear_pt_entry( get_pte(la) );

    invlpg(la);
}

// TODO this one is machindep
void arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
    int num_pages = ((long)end)/PAGE_SIZE - ((long)start)/PAGE_SIZE;
    while ( num_pages-- >= 0 ) {
        //invalidate_TLB(start);
        invlpg((unsigned int)start);
        start += PAGE_SIZE;
    }
}

// TODO this one is machindep
void arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
    int i;
    for( i=0; i<num_pages; i++ ) {
        invlpg((addr_t)pages[i]);
    }
}

void arch_cpu_invalidate_TLB(void)
{
    clear_tlb();
}




void
hal_page_control_etc(
                     physaddr_t  p, void *page_start_addr,
                     page_mapped_t mapped, page_access_t access,
                     u_int32_t flags
                )
{
    assert(PAGE_ALIGNED(p));
    assert(PAGE_ALIGNED((unsigned)page_start_addr));
    assert(flags == 0); // lets make 'em portable too

    if(mapped == page_unmap) access = page_noaccess;


    pt_entry_t	pte;

    pte.v = (((linaddr_t)page_start_addr) & PTE_V_ADDR_MASK);

    pte.p = (p & TLB_P_ADDR_MASK) >> TLB_P_ADDR_SHIFT;

    if(mapped == page_map_io)
        pte.p |= TLB_P_CACHE_UNCACHED << TLB_P_CACHE_SHIFT;
    else
        pte.p |= TLB_P_CACHE_DEFAULT << TLB_P_CACHE_SHIFT;


    if(access == page_rw)
        pte.p |= TLB_P_DIRTY;

    pte.p |= TLB_P_VALID;
    pte.p |= TLB_P_GLOBAL;

    SHOW_FLOW( 10, "Map VA 0x%x to PA 0x%x, pte.v=0x%x pte.p=0x%x\n",
                          page_start_addr, p, pte.v, pte.p );

    if(mapped != page_unmap )
        phantom_map_page( (linaddr_t)page_start_addr, &pte );
    else
        phantom_unmap_page( (linaddr_t)page_start_addr );

    invlpg((addr_t)page_start_addr);
}



































