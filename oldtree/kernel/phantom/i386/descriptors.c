/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * IDT/GDT/LDT code
 *
**/

#define DEBUG_MSG_PREFIX "i386desc"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/config.h>
#include <kernel/physalloc.h>

#include <ia32/private.h>
#include <ia32/selector.h>
#include <ia32/eflags.h>
#include <ia32/seg.h>
#include <ia32/tss.h>
#include <ia32/proc_reg.h>
//#include <ia32/pio.h>
#include <ia32/ldt.h>

#include <phantom_types.h>
#include <phantom_libc.h>

#include <kernel/vm.h>
#include <kernel/smp.h>
#include <kernel/init.h>
#include <kernel/trap.h>
#include <kernel/page.h>

#include <hal.h>
#include <ia32/vm86.h>

#include "../misc.h"





struct real_descriptor 	gdt[GDTSZ];
struct real_descriptor 	ldt[LDTSZ];
struct real_gate 	idt[IDTSZ];

//struct i386_tss	       	tss;
struct i386_tss	       	cpu_tss[MAX_CPUS];
struct vm86tss		tss_vm86;

#define main_tss (cpu_tss[0])


// used from direct VESA driver
void set_descriptor_limit( struct real_descriptor *d, unsigned limit )
{
    if (limit > 0xfffff)
    {
        if( (limit+1) & 0xFFF )            printf("warning - big limit not aligned to page");
        limit >>= 12;
        //limit = 1 + ((limit-1)/PAGE_SIZE);
        d->granularity |= SZ_G;
    }
    else
        d->granularity &= ~SZ_G;

    //limit--; // ia32 wants it that way

    d->limit_low = limit & 0xffff;
    d->limit_high = limit >> 16;

}



void phantom_load_gdt()
{
    struct region_descriptor rd;

    rd.rd_limit = sizeof(gdt) - 1;
    rd.rd_base = kvtolin(&gdt);

    asm volatile("lgdt %0" : : "m" ((&rd)->rd_limit));

    // Now Intel wants us to reload segments

    asm volatile("ljmp %0,$1f  ;\
                 1:             \
                 " : : "i" (KERNEL_CS));


    asm volatile("movw %w0,%%ds" : : "r" (KERNEL_DS));
    asm volatile("movw %w0,%%es" : : "r" (KERNEL_DS));
    asm volatile("movw %w0,%%ss" : : "r" (KERNEL_DS));

    asm volatile("movw %w0,%%fs" : : "r" (0));
    asm volatile("movw %w0,%%gs" : : "r" (0));

    // and activate the LDT
    wrldt(MAIN_LDT);

}


void phantom_load_main_tss()
{
    // get it ready for reuse - we are called on exit from VM86 mode too
    gdt[MAIN_TSS / 8].access &= ~ACC_TSS_BUSY;
    gdt[VM86_TSS / 8].access &= ~ACC_TSS_BUSY;

    // now load TSS
    asm volatile("ltr %0" : : "rm" ((u_int16_t)MAIN_TSS) );
}


#define IS_SIZE (1024*16)
static char intr_stack[IS_SIZE];

static char cpu_intr_stack[MAX_CPUS][IS_SIZE];

void phantom_init_descriptors(void)
{
    main_tss.ss0 = KERNEL_DS;

    //main_tss.esp0 = get_esp(); // why?
    main_tss.esp0 = (int) (intr_stack+IS_SIZE-4);

    main_tss.io_bit_map_offset = sizeof(main_tss);

    //tss_vm86.tss.ss0 = KERNEL_DS_16; // WHY!!!?
    tss_vm86.tss.ss0 = KERNEL_DS;
    //main_tss.esp0 = get_esp(); // why?
    tss_vm86.tss.io_bit_map_offset = sizeof(tss_vm86);


    make_descriptor(gdt, MAIN_TSS, kvtolin(&main_tss), sizeof(main_tss)-1,
                    ACC_PL_K | ACC_TSS | ACC_P, 0 );

    make_descriptor(gdt, VM86_TSS, kvtolin(&tss_vm86), sizeof(tss_vm86)-1,
                    ACC_PL_K | ACC_TSS | ACC_P, 0 );

    make_descriptor(gdt, KERNEL_CS, kvtolin(0), 0xFFFFFFFF,
                    ACC_PL_K | ACC_CODE_R, SZ_32 );
    make_descriptor(gdt, KERNEL_DS, kvtolin(0), 0xFFFFFFFF,
                    ACC_PL_K | ACC_DATA_W, SZ_32 );

    make_descriptor(gdt, USER_CS, kvtolin(0), 0xFFFFFFFF,
                    ACC_PL_U | ACC_CODE_R, SZ_32 );
    make_descriptor(gdt, USER_DS, kvtolin(0), 0xFFFFFFFF,
                    ACC_PL_U | ACC_DATA_W, SZ_32 );

    make_descriptor(gdt, KERNEL_CS_16, kvtolin(0), 0xFFFF,
                    ACC_PL_K | ACC_CODE_R, SZ_32 );
    make_descriptor(gdt, KERNEL_DS_16, kvtolin(0), 0xFFFF,
                    ACC_PL_K | ACC_DATA_W, SZ_32 );


    make_descriptor(gdt, MAIN_LDT, kvtolin(&ldt), LDTSZ * sizeof(struct real_descriptor) - 1,
                    ACC_P | ACC_PL_K | ACC_LDT, 0 );


// -----------------------------------------------------------------------
// Now LDT
// -----------------------------------------------------------------------

    /* Initialize the master LDT descriptor in the GDT.  */
    make_descriptor(gdt, MAIN_LDT,
                    kvtolin(&ldt), sizeof(ldt)-1,
                    ACC_PL_K|ACC_LDT, 0);

    /* Initialize the LDT descriptors.  */
    fill_ldt_gate(USER_LDT_SCALL,
                  (int)&syscall, KERNEL_CS,
                  ACC_PL_U|ACC_CALL_GATE, 0);
/*
#define VM_MIN_ADDRESS 0
#define VM_MAX_ADDRESS 0
    fill_ldt_descriptor(USER_LDT_CS,
                        VM_MIN_ADDRESS, VM_MAX_ADDRESS-VM_MIN_ADDRESS,
                        // XXX LINEAR_...
                        ACC_PL_U|ACC_CODE_R, SZ_32);

    fill_ldt_descriptor(USER_LDT_DS,
                        VM_MIN_ADDRESS, VM_MAX_ADDRESS-VM_MIN_ADDRESS,
                        ACC_PL_U|ACC_DATA_W, SZ_32);
*/


    int i;
    for( i = 0; i < MAX_CPUS; i++ )
    {
        cpu_tss[i].ss0 = KERNEL_DS;

        cpu_tss[i].esp0 = (int) (cpu_intr_stack[i]+IS_SIZE-4);

        cpu_tss[i].io_bit_map_offset = sizeof(cpu_tss[0]);

        make_descriptor(gdt, (CPU_TSS+i*8), kvtolin(&(cpu_tss[i])), sizeof(struct i386_tss)-1,
                    ACC_PL_K | ACC_TSS | ACC_P, 0 );

    }

    phantom_load_gdt();
    phantom_load_main_tss();
}



void phantom_load_cpu_tss(int ncpu)
{
    // get it ready for reuse - we are called on exit from VM86 mode too
    //gdt[MAIN_TSS / 8].access &= ~ACC_TSS_BUSY;

    // now load TSS
    asm volatile("ltr %0" : : "rm" ((u_int16_t)(CPU_TSS+ncpu*8)) );
}






extern int syscall(void);


errno_t get_uldt_cs_ds(
                       linaddr_t cs_base, u_int16_t *ocs, size_t cs_limit,
                       linaddr_t ds_base, u_int16_t *ods, size_t ds_limit
                      )
{
    SHOW_FLOW( 3, "User CS %p 0x%x bytes, DS %p 0x%x bytes", (void *)cs_base, cs_limit, (void *)ds_base, ds_limit );

#if 0
    if( (segsToUse >> 3) + 1 >= LDTSZ )
        return ENOMEM;

    u_int16_t cs = segsToUse | 0x7;
    segsToUse += 0x08;
    u_int16_t ds = segsToUse | 0x7;
    segsToUse += 0x08;
#else
    u_int16_t cs = alloc_ldt_selector();
    u_int16_t ds = alloc_ldt_selector();
    if( (cs == NULL_SELECTOR) || (ds == NULL_SELECTOR) )
        return ENOMEM;

    ds |= SELECTOR_USER;
    cs |= SELECTOR_USER;
#endif

    //fill_ldt_descriptor(cs, cs_base, 1 + (cs_limit-1)/PAGE_SIZE, ACC_PL_U|ACC_CODE_R, SZ_32|SZ_G);
    //fill_ldt_descriptor(ds, ds_base, 1 + (ds_limit-1)/PAGE_SIZE, ACC_PL_U|ACC_DATA_W, SZ_32|SZ_G);

    fill_ldt_descriptor(cs, cs_base, cs_limit-1, ACC_PL_U|ACC_CODE_R, SZ_32);
    fill_ldt_descriptor(ds, ds_base, ds_limit-1, ACC_PL_U|ACC_DATA_W, SZ_32);

#if 0
    struct real_descriptor *dsd = get_descriptor(ldt,ds);
    printf(
           "ds 0x%x: lim low %x, base low %x, base med %x, acc %x, lim hi %x, gran %x, base hi %x  (g must be %x)\ndump:\n",
           ds,

           dsd->limit_low,
           dsd->base_low,
           dsd->base_med,
           dsd->access,
           dsd->limit_high,
           dsd->granularity,
           dsd->base_high,
           SZ_32|SZ_G
          );
    hexdump( dsd, sizeof(*dsd), 0, 0 );
#endif

    *ocs = cs;
    *ods = ds;

    return 0;
}

errno_t get_uldt_sel( u_int16_t *osel, linaddr_t sel_base, size_t sel_limit, int code, int is32 )
{
#if 0
    if( (segsToUse >> 3) >= LDTSZ )
        return ENOMEM;

    u_int16_t s = segsToUse | 0x7;

    segsToUse += 0x8;
#else
    u_int16_t s = alloc_ldt_selector();
    if( s == NULL_SELECTOR )
        return ENOMEM;

    s |= SELECTOR_USER;
#endif

    fill_ldt_descriptor(s, sel_base, sel_limit-1, ACC_PL_U | ( code ? ACC_CODE_R : ACC_DATA_W ), is32 ? SZ_32 : 0 );

    *osel = s;

    return 0;
}

#define USE_LDT_PHYSALLOC 1




#if USE_LDT_PHYSALLOC
static map_elem_t    	ldt_mapbuf[MAP_SIZE_ELEM(LDTSZ)];
static physalloc_t   	ldt_map;
#else
static int      segsToUse = 0x10;
#endif

static void init_ldt_alloc(void)
{
    phantom_phys_alloc_init_static( &ldt_map, LDTSZ, ldt_mapbuf );
    phantom_phys_free_region( &ldt_map, LDT_RESERVED, LDTSZ-LDT_RESERVED-1 ); // -1 = error in phantom_phys_free_region?
    ldt_map.allocable_size = LDTSZ-LDT_RESERVED-1;
    ldt_map.n_used_pages = 0;
}

INIT_ME(init_ldt_alloc, 0, 0)


selector_id alloc_ldt_selector(void)
{
    //STAT_INC_CNT(STAT_CNT_LDT_ALLOC);
#if USE_LDT_PHYSALLOC
    physalloc_item_t ret;
    errno_t rc = phantom_phys_alloc_page( &ldt_map, &ret );
    if( rc )
        return NULL_SELECTOR;
    u_int16_t s = (((u_int16_t)ret) << 3) | SELECTOR_LDT;
    SHOW_FLOW( 3, "alloc ldt sel %x", s );
    return s;
#else

    if( (segsToUse >> 3) >= LDTSZ )
        return NULL_SELECTOR;

    u_int16_t s = segsToUse | SELECTOR_LDT;

    segsToUse += 0x8;

    return s;
#endif
}

void free_ldt_selector(selector_id sel)
{
    assert(SELECTOR_IS_LDT(sel));
    //STAT_INC_CNT(STAT_CNT_LDT_FREE);

    fill_ldt_descriptor(sel, 0, 0, 0, 0 );

#if USE_LDT_PHYSALLOC
    SHOW_FLOW( 3, "free ldt sel %x", sel );
    phantom_phys_free_page( &ldt_map, sel>>3 );
#else
    SHOW_ERROR( 0, "unimpl free ldt sel %x", sel );
#endif
}

// -----------------------------------------------------------------------
//
// Modify kernel DS limit to enable/disable access to paged/persistent memory
//
//

errno_t
arch_ia32_modify_ds_limit( bool on_off)
{   
	unsigned int sel;

	__asm __volatile("movw %%ds,%w0" : "=rm" (sel));
    assert( sel == KERNEL_DS );

	__asm __volatile("movw %%es,%w0" : "=rm" (sel));
    assert( sel == KERNEL_DS );

	__asm __volatile("movw %%ss,%w0" : "=rm" (sel));
    assert( sel == KERNEL_DS );


    struct real_descriptor *g; 
    g = (struct real_descriptor *) (((char *)gdt) + (KERNEL_DS & SEL_MASK) );

    set_descriptor_limit( g, on_off ? 0xFFFFFFFF : (PHANTOM_AMAP_START_VM_POOL) );

    asm volatile("movw %w0,%%ds" : : "r" (KERNEL_DS));
    asm volatile("movw %w0,%%es" : : "r" (KERNEL_DS));
    asm volatile("movw %w0,%%ss" : : "r" (KERNEL_DS));

    return 0;
}

