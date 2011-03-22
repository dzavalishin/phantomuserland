/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * arm page table support
 *
**/

/*
 Overcoming arm quirks in page access mode:
 Control register:
 S=1
 R=0

 Now AP is:

 AP    system    user
 00 =    RO       --
 01 =    RW       --
 10 =    RW       RO
 11 =    RW       RW

 And complete denial of access is made by wiping descriptor at all.

*/

#define ACC_KRO_U00     0x00
#define ACC_KRW_U00     0x01
#define ACC_KRW_URO     0x02
#define ACC_KRW_URW     0x03





#define DEBUG_MSG_PREFIX "paging"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/config.h>
#include <kernel/init.h>
#include <malloc.h>
#include <phantom_libc.h>

#include <arm/pmap.h>
#include <kernel/page.h>
#include <hal.h>
#include <machdep.h>

//typedef pd_entry_t u_int32_t;
//typedef pt_entry_t u_int32_t;

typedef u_int32_t pd_entry_t;
typedef u_int32_t pt_entry_t;


static __inline__ pd_entry_t pa_to_pde(physaddr_t pa)
{
    // Mask low 10 bits off
    pd_entry_t e = pa & ((~0) << 10);
    e |= ARM_PDE_TYPE_PAGETBL;
    e |= (1<<4); // bit 4 must be 1
    // domain is left ot be zero
    return e;
}

static void phantom_paging_start(void);


static pd_entry_t *pdir;
static pt_entry_t *ptabs;

static int paging_inited = 0;

void phantom_paging_init(void)
{
    // On arm - allocate all the possible page tables statically, for simplicity
    extern char phantom_pdir_mem[];
    extern char phantom_ptab_mem[];

    pdir = (pd_entry_t *)&phantom_pdir_mem;
    ptabs = (pt_entry_t *)&phantom_ptab_mem;

    assert((((unsigned) pdir) & (PAGE_SIZE-1)) == 0); // wrong?
    assert((((unsigned)ptabs) & (PAGE_SIZE-1)) == 0); // wrong?
    assert( ((unsigned) pdir) != 0);
    assert( ((unsigned)ptabs) != 0);

    memset( ptabs, 0, ARM_PT_SIZE*NPDE );

    pd_entry_t *pde = pdir;
    //unsigned int *pde = pdir;
    void *ptep = ptabs;
    int i;

    for( i = 0; i < NPDE; i++, pde++ )
    {
        *pde = pa_to_pde((int)ptep);
        ptep += ARM_PT_SIZE;
    }

    // TODO Set the Domain Access register. Get from entry.S

    // needed by phantom_map_mem_equally();
    paging_inited = 1;

    phantom_map_mem_equally();

    SHOW_FLOW0( 2, "mem mapped" );

    phantom_paging_start();

    SHOW_FLOW0( 2, "paging started" );
}


static void phantom_paging_start(void)
{
    // Set pagedir address
    asm volatile("                           \
                 mcr p15, 0, %0, C2, C0, 0  ;\
                 " : : "r" ((int)pdir));


    // s=1&r=0 s is bit 8
    // TODO - d cache enable? ORR R1, #0x4
    // TODO - i cache enable? ORR R1, #(1<<12)
    // TODO before enabling cache add invalidation logic to invlpg below!

    // Tell CPU to start paging
    asm volatile("                      \
                 MRC p15,0,R1,C1,C0,0  ;\
                 ORR R1, #0x1          ;\
                 ORR R1, #(1<<8)       ;\
                 MCR p15,0,R1,C1,C0,0  ;\
                 nop; nop; nop; nop    ;\
                 " : : );
}

// TLB entry size??? less than page?
// TODO cache must be invalidated (and written back!) too

static __inline__ void invlpg(addr_t start)
{
    // I and D TLB

    assert( (start & 0x3FF)  == 0 );

    int i;
    for( i = 0; i < 4; i++ )
    {
        asm volatile("                      \
                     MCR p15,0,%0,c8,c5,1  ;\
                     MCR p15,0,%0,c8,c6,1  ;\
                     " : : "r" (start));
        start += 0x400; // 4 times per one kbyte, cover 4K page
    }
}



//static pt_entry_t *get_pte( linaddr_t la );

// Find PDE for given lin addr (usually == virt addr in Phantom)
//#define get_pde( la ) (pdir + lin2pdenum(la))

// Each one serves a 4K bytes page
// No mask as we are indexing array of PTs!
#define lin2linear_ptenum(a) ( ((a) / 4096) )

// Find PTE for given lin addr
// Since our map is in one piece (all page tables are following each other),
// we can do it remarkably easy!
#define get_pte( la ) (ptabs + lin2linear_ptenum(la))


void phantom_map_page(linaddr_t la, pt_entry_t mapping )
{
    assert(PAGE_ALIGNED(la));
    assert(paging_inited);
    *get_pte(la) = (mapping & ~ARM_PTE_TYPE_MASK) | ARM_PTE_TYPE_SM_PAGE;
    invlpg(la);
}


void phantom_unmap_page(linaddr_t la )
{
    assert(PAGE_ALIGNED(la));
    assert(paging_inited);
    *get_pte(la) = 0;
    invlpg(la);
}

#if 0 // Impossible on ARM :(
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
#endif


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


void ftlbentry(physaddr_t la)
{
    invlpg((unsigned int)la);
}


// Insert access permissions 4 times in right place
static __inline__ void put_ap( pt_entry_t *e, u_int32_t ap )
{
    ap &= 0x3; // 2 bits
    *e &= ~0xFF0; // Reset AP bits for sure
    *e |= (ap << 4);
    *e |= (ap << 6);
    *e |= (ap << 8);
    *e |= (ap << 10);
}

#define create_pte(__p, __bits) ( (p & ~0xFFF) | (bits & 0xFFF) )


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

    if(mapped != page_map) access = page_noaccess;

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

    pt_entry_t	pte;

    pte = create_pte(p, bits);

    SHOW_FLOW( 7, "Map VA 0x%X to PA 0x%X, pte=0x%X\n",
                          page_start_addr, p, (long)pte );

    if(mapped != page_unmap )
        phantom_map_page( (linaddr_t)page_start_addr, pte );
    else
        phantom_unmap_page( (linaddr_t)page_start_addr );
    ftlbentry((int)page_start_addr);
}








