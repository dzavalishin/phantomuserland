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


void phantom_paging_init(void)
{
    memset( ptab, 0, sizeof ptab );

    // needed by phantom_map_mem_equally();
    paging_inited = 1;

    phantom_map_mem_equally();

    SHOW_FLOW0( 2, "mem mapped" );

    phantom_paging_start();

    SHOW_FLOW0( 2, "paging started" );
}


static void phantom_paging_start(void)
{
    SHOW_ERROR0( 2, "NOT impl paging start" );
}

//* Read TLB entry at given location
static void read_tlb_entry_loc( pt_entry_t *e, int loc )
{
    SHOW_ERROR0( 2, "NOT impl" );
}

//* Write TLB entry at given location
static void write_tlb_entry_loc( pt_entry_t *e, int loc )
{
    SHOW_ERROR0( 2, "NOT impl" );
}

//* Write TLB entry at random location
static void write_tlb_entry_rnd( pt_entry_t *e )
{
    SHOW_ERROR0( 2, "NOT impl" );
}


//* Clear TLB
static void clear_tlb()
{
    pt_entry_t te;
    memset( &te, 0, sizeof te );

    int ie = hal_save_cli();

    int i;
    for( i = 0; i < NTLBE; i++ )
        write_tlb_entry_loc( &te, i );

    if(ie) hal_sti();
}



//* Clear entry for given virtual address. NB! Ignores 8 bit address space id field!
static void clear_tlb_entry( addr_t va )
{
    pt_entry_t te;

    if( va & (PAGE_SIZE-1) )
        SHOW_ERROR0( 2, "va low bits?" );

    addr_t pageno = va >> PAGE_SHIFT;

    int ie = hal_save_cli();

    int i;
    for( i = 0; i < NTLBE; i++ )
    {
        read_tlb_entry_loc( &te, i );

#warning impl
        // compare pageno to TLB va bits

        // Clear entry
        memset( &te, 0, sizeof te );
        write_tlb_entry_loc( &te, i );

    }

    if(ie) hal_sti();
}



//* Read entry for given virtual address. NB! Ignores 8 bit address space id field!
static errno_t read_tlb_entry( pt_entry_t *e, addr_t va )
{
    pt_entry_t te;

    assert_int_disabled();

    if( va & (PAGE_SIZE-1) )
        SHOW_ERROR0( 2, "va low bits?" );

    addr_t pageno = va >> PAGE_SHIFT;

    int i;
    for( i = 0; i < NTLBE; i++ )
    {
        read_tlb_entry_loc( &te, i );

#warning impl
        // compare pageno to TLB va bits

    }

    return ENOENT;
}

//* Write entry for given virtual address. NB! Ignores 8 bit address space id field!
static void write_tlb_entry( pt_entry_t *e, void *va )
{
    write_tlb_entry_rnd( e );
}

// TODO on MIPS we can manually put new entry to TLB. make revalidate_pg()?
static __inline__ void invlpg(addr_t start) { clear_tlb_entry( start ); }

#define lin2linear_ptenum(a) ( ((a) / 4096) )
#define get_pte( la ) (ptab + lin2linear_ptenum(la))


void phantom_map_page(linaddr_t la, pt_entry_t mapping )
{
    assert(PAGE_ALIGNED(la));
    assert(paging_inited);

#warning impl
    //*get_pte(la) = (mapping & ~ARM_PTE_TYPE_MASK) | ARM_PTE_TYPE_SM_PAGE;

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
#if 0
    // We need it for V86 mode - REDO IN A MORE SPECIFIC WAY, so that only VM86 pages are user accessible
    pt_entry_t bits = ARM_PTE_TYPE_SM_PAGE;

    if(mapped == page_map)
        bits |= ARM_PTE_BUFFERED|ARM_PTE_CACHED;

//    if(mapped == page_map_io)
//        bits |= 0;


    if(access == page_rw)
        put_ap( &bits, ACC_KRW_URW );
    else
        put_ap( &bits, ACC_KRO_U00 ); // No other way :(


    pte = create_pte(p, bits);
#endif
    SHOW_FLOW( 10, "Map VA 0x%X to PA 0x%X, pte=0x%X\n",
                          page_start_addr, p, *(long*)&pte );

    if(mapped != page_unmap )
        phantom_map_page( (linaddr_t)page_start_addr, pte );
    else
        phantom_unmap_page( (linaddr_t)page_start_addr );

    invlpg((addr_t)page_start_addr);
}



































