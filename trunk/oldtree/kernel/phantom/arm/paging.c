#if 0
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * arm page table support
 *
**/

#define DEBUG_MSG_PREFIX "paging"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/config.h>
#include <malloc.h>
#include <phantom_libc.h>

#include <arm/pmap.h>
//#include <i386/proc_reg.h>
//#include <kernel/ia32/cpu.h>
#include <kernel/page.h>
#include <hal.h>
#include <machdep.h>

//#include "misc.h"

static pd_entry_t *pdir;
static pt_entry_t *ptabs;

static int paging_inited = 0;

void phantom_paging_init(void)
{
    // On arm - allocate all the possible page tables, for simplicity
    extern char phantom_pdir_mem[];
    extern char phantom_ptab_mem[];

    pdir = (pd_entry_t *)&phantom_pdir_mem;
    ptabs = (pt_entry_t *)&phantom_ptab_mem;

    assert((((unsigned) pdir) & (PAGE_SIZE-1)) == 0);
    assert((((unsigned)ptabs) & (PAGE_SIZE-1)) == 0);
    assert( ((unsigned) pdir) != 0);
    assert( ((unsigned)ptabs) != 0);

    memset( ptabs, 0, PAGE_SIZE*NPDE );

    //pd_entry_t *pde = pdir;
    unsigned int *pde = pdir;
    void *ptep = ptabs;
    int i;

    for( i = 0; i < NPDE; i++, pde++ )
    {
        *pde = pa_to_pde((int)ptep) | INTEL_PDE_VALID | INTEL_PDE_USER | INTEL_PDE_WRITE;
        ptep += PAGE_SIZE;
    }

    // needed by phantom_map_mem_equally();
    paging_inited = 1;

    phantom_map_mem_equally();
    phantom_paging_start();

}


void phantom_paging_start(void)
{
    set_cr3((int)pdir);

    // Tell CPU to start paging
    asm volatile("                      \
                 movl	%0,%%cr0       ;\
                 jmp	1f             ;\
                 1:                     \
                 " : : "r" (get_cr0() | CR0_PG));
}





//static pt_entry_t *get_pte( linaddr_t la );

// Find PDE for given lin addr (usually == virt addr in Phantom)
//#define get_pde( la ) (pdir + lin2pdenum(la))

//#define lin2ptenum(a) ( ((a) >> PTESHIFT) & PTEMASK )
// No mask as we are indexing array of PTs!
#define lin2lin_ptenum(a) ( ((a) >> PTESHIFT) )

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


#if 0

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

#endif

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

void arch_cpu_invalidate_TLB(void)
{
    asm volatile("mcr p15, 0, r0, c8, c7, 0");
}


#endif
