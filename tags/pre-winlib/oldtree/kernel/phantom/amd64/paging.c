#if 0
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ia32 page table support
 *
**/

#define DEBUG_MSG_PREFIX "pgtable"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10



#include <kernel/mmu.h>
#include <kernel/page.h>
#include <hal.h>
#include <stdio.h>


#include <amd64/phantom_pmap.h>




void
hal_page_control_etc(
                     physaddr_t  p, void *page_start_addr,
                     page_mapped_t mapped, page_access_t access,
                     u_int32_t flags
                )
{
    assert(PAGE_ALIGNED(p));
    assert(PAGE_ALIGNED((unsigned)page_start_addr));
    assert((flags & INTEL_PTE_PFN) == 0);

    if(mapped == page_unmap) access = page_noaccess;

    int bits = INTEL_PTE_USER | flags; // We need it for V86 mode - REDO IN A MORE SPECIFIC WAY, so that only VM86 pages are user accessible

    if(mapped == page_map)
        bits |= INTEL_PTE_VALID;

    if(mapped == page_map_io)
        bits |= INTEL_PTE_VALID|INTEL_PTE_WTHRU|INTEL_PTE_NCACHE;

    if(access == page_rw)
        bits |= INTEL_PTE_WRITE;

    pt_entry_t	pte;

    pte = create_pte(p, bits);

    SHOW_FLOW( 7, "Mapping VA 0x%X to PA 0x%X, pte is 0x%X\n", page_start_addr, p, (long)pte );

    if(mapped != page_unmap )
        phantom_map_page( (linaddr_t)page_start_addr, pte );
    else
        phantom_unmap_page( (linaddr_t)page_start_addr );
    ftlbentry((int)page_start_addr);
}



#endif
