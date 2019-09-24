/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Intel ia32 page table support.
 * 
 * CONF_DUAL_PAGEMAP:
 * 
 * We keep two page directories. One covers all memory, other excludes 
 * persistent (paged) memory range. Two pagemaps are switched to enable/
 * disable persistent memory access for current thread.
 * 
 * TODO rename all funcs to arch_ prefix
 *
**/

#define DEBUG_MSG_PREFIX "paging"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/config.h>
#include <malloc.h>
#include <phantom_libc.h>

#include <ia32/phantom_pmap.h>
#include <ia32/proc_reg.h>

#include <kernel/ia32/cpu.h>
#include <kernel/page.h>
#include <hal.h>
#include <machdep.h>

#include "misc.h"

#if CONF_DUAL_PAGEMAP
static pd_entry_t *pdir_on;
static pd_entry_t *pdir_off;
#else
static pd_entry_t *pdir;
#endif
static pt_entry_t *ptabs;

static int paging_inited = 0;

/// Differentiate between page tables in paged and non-page areas
int ptep_is_paged_area(int npde)
{
    unsigned lin = pdenum2lin(npde);
    
    //if( lin < PHANTOM_AMAP_START_VADDR_POOL) return 0;    
    //return lin < (PHANTOM_AMAP_START_VADDR_POOL+PHANTOM_AMAP_SIZE_VADDR_POOL);

    return lin > PHANTOM_AMAP_START_VM_POOL;
    //return lin > (PHANTOM_AMAP_START_VM_POOL+__MEM_GB);
    //return 0;
}

void phantom_paging_init(void)
{
    set_cr0( get_cr0() | CR0_WP ); // CPU can't write to read-only pages when privilege level is 0

    // On ia32 - allocate all the possible page tables, for simplicity
#if CONF_DUAL_PAGEMAP
    extern char phantom_pdir_on_mem[];
    pdir_on = (pd_entry_t *)&phantom_pdir_on_mem;
    assert((((unsigned) pdir_on) & (PAGE_SIZE-1)) == 0);
    assert( ((unsigned) pdir_on) != 0);

    extern char phantom_pdir_off_mem[];
    pdir_off = (pd_entry_t *)&phantom_pdir_off_mem;
    assert((((unsigned) pdir_off) & (PAGE_SIZE-1)) == 0);
    assert( ((unsigned) pdir_off) != 0);
#else
    extern char phantom_pdir_mem[];
    pdir = (pd_entry_t *)&phantom_pdir_mem;
    assert((((unsigned) pdir) & (PAGE_SIZE-1)) == 0);
    assert( ((unsigned) pdir) != 0);
#endif

    extern char phantom_ptab_mem[];
    ptabs = (pt_entry_t *)&phantom_ptab_mem;
    assert((((unsigned)ptabs) & (PAGE_SIZE-1)) == 0);
    assert( ((unsigned)ptabs) != 0);

    memset( ptabs, 0, PAGE_SIZE*NPDE );

#if CONF_DUAL_PAGEMAP
    unsigned int *pde_on = pdir_on;
    unsigned int *pde_off = pdir_off;
    void *ptep = ptabs;
    int i;

    for( i = 0; i < NPDE; i++, pde_on++, pde_off++ )
    {
        *pde_on = pa_to_pde((int)ptep) | INTEL_PDE_VALID | INTEL_PDE_USER | INTEL_PDE_WRITE;
        if(!ptep_is_paged_area(i))
            *pde_off = pa_to_pde((int)ptep) | INTEL_PDE_VALID | INTEL_PDE_USER | INTEL_PDE_WRITE;
        ptep += PAGE_SIZE;
    }
#else
    //pd_entry_t *pde = pdir;
    unsigned int *pde = pdir;
    void *ptep = ptabs;
    int i;

    for( i = 0; i < NPDE; i++, pde++ )
    {
        *pde = pa_to_pde((int)ptep) | INTEL_PDE_VALID | INTEL_PDE_USER | INTEL_PDE_WRITE;
        ptep += PAGE_SIZE;
    }
#endif
    // needed by phantom_map_mem_equally();
    paging_inited = 1;

    // Now map all mem equally
    //hal_pages_control( 0, 0, NPDE*NPTE, page_map, page_rw );

    phantom_map_mem_equally();
    // Now map all mem equally - JUST 1/4
    //- upper 2GBs are handled by virt mem, and 0x40000000 - 0x80000000 is for addr space allocator
    //hal_pages_control( 0, 0, NPDE*NPTE/4, page_map, page_rw );

#if 0
    // If we have superpages support, enable 'em in CPU
    if( cpuid.feature_flags & CPUF_4MB_PAGES )
    {
        set_cr4(get_cr4() | CR4_PSE);
    }
#endif

    // Load page directory phys addr to CR3, enable paging
    phantom_paging_start();

    // Note that pa = va now

}


void phantom_paging_start(void)
{
#if CONF_DUAL_PAGEMAP
    //set_cr3((int)pdir_on); // temp test
    set_cr3((int)pdir_off); // By default is off
#else
    set_cr3((int)pdir);
#endif
    // Tell CPU to start paging
    asm volatile("                      \
                 movl	%0,%%cr0       ;\
                 jmp	1f             ;\
                 1:                     \
                 " : : "r" (get_cr0() | CR0_PG));
}

#if CONF_DUAL_PAGEMAP
/**
 * \brief Enable or disable paged mem access. Must be called just 
 * from t_set_paged_mem() in threads lib, as it saves cr3 state for 
 * thread switch.
 * 
 * \return cr3 value for threads lib to save.
 * 
**/
int32_t arch_switch_pdir( bool paged_mem_enable )
{
    int32_t pdir = (int32_t) (paged_mem_enable ? pdir_on : pdir_off);
    //int32_t pdir = (int32_t) pdir_on;
    set_cr3(pdir);
    return pdir;
}


int32_t arch_get_pdir( bool paged_mem_enable )
{
    int32_t pdir = (int32_t) (paged_mem_enable ? pdir_on : pdir_off);
    return pdir;
}


int arch_is_object_land_access_enabled() //< check if current thread attempts to access object space having access disabled
{
    return get_cr3() == (u_int32_t)pdir_on;
}

#endif

//static pt_entry_t *get_pte( linaddr_t la );

// Find PDE for given lin addr (usually == virt addr in Phantom)
//#define get_pde( la ) (pdir + lin2pdenum(la))

//#define lin2ptenum(a) ( ((a) >> PTESHIFT) & PTEMASK )
// No mask as we are indexing array of PTs!
#define lin2lin_ptenum(a) ( ((a) >> PTESHIFT) )

//#define ptenum2lin(a) ((a) << PTESHIFT )

// Find PTE for given lin addr
// Since our map is in one piece (all page tables are following each other),
// we can do it remarkably easy!
#define get_pte( la ) (ptabs + lin2lin_ptenum(la))


void phantom_map_page(linaddr_t la, pt_entry_t mapping )
{
    assert(PAGE_ALIGNED(la));
    assert(paging_inited);
    *get_pte(la) = mapping;
    invlpg(la);
}


void phantom_unmap_page(linaddr_t la )
{
    assert(PAGE_ALIGNED(la));
    assert(paging_inited);
    *get_pte(la) = 0;
    invlpg(la);
}


int phantom_is_page_accessed(linaddr_t la )
{
    assert(PAGE_ALIGNED(la));
    assert(paging_inited);
    return (*get_pte(la)) & INTEL_PTE_REF;
}

int phantom_is_page_modified(linaddr_t la )
{
    assert(PAGE_ALIGNED(la));
    assert(paging_inited);
    return (*get_pte(la)) & INTEL_PTE_MOD;
}



void
phantom_dump_pdir()
{
    pt_entry_t *e = ptabs;
    int was_invalid = 0;
    //phyasddr_t range_start = ptenum2lin(0);

    assert(paging_inited);
    printf("------------ PageTable\n");

    int i;
    for( i = 0; i < NPDE*NPTE; i++, e++ )
    {

        physaddr_t lin = ptenum2lin(i);

        if(!was_invalid && !( *e & INTEL_PTE_VALID))
        {
            //printf("INVALID\n");
            printf("%08x ", lin);
            was_invalid = 1;
            continue;
        }

        if(was_invalid)
        {
            if(!( *e & INTEL_PTE_VALID))
                continue;

            was_invalid = 0;
            physaddr_t elin = ptenum2lin(i-1);
            printf("-%08x INVALID\n", elin);
        }

        printf("%08x ", lin);
        if(pte_to_pa(*e) == lin)
            printf(" (EQ) ");
        else
            printf( " -> %08X", pte_to_pa(*e) );

        printf( " %s%s%s%s%s%s%s ",
			*e & INTEL_PTE_GLOBAL ? " GLOBAL" : "",
			*e & INTEL_PTE_MOD ? " MOD" : "",
			*e & INTEL_PTE_REF ? " REF" : "",
			*e & INTEL_PTE_NCACHE ? " NCACHE" : "",
			*e & INTEL_PTE_WTHRU ? " WTHRU" : "",
			*e & INTEL_PTE_USER ? " USER" : "",
			*e & INTEL_PTE_WRITE ? " WRITE" : ""
              );

        printf("\n");

    }

    printf("\n");
}


void arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
    int num_pages = ((long)end)/PAGE_SIZE - ((long)start)/PAGE_SIZE;
    while ( num_pages-- >= 0 ) {
        //invalidate_TLB(start);
        invlpg((unsigned int)start);
        start += PAGE_SIZE;
    }
}

void arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
    int i;
    for(i=0; i<num_pages; i++) {
        //invalidate_TLB(pages[i]);
        invlpg((unsigned int)pages[i]);
    }
}

