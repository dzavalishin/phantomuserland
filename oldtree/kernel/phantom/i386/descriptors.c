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
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/config.h>

#include <ia32/private.h>

#include <ia32/eflags.h>
#include <ia32/seg.h>
#include <ia32/tss.h>
#include <ia32/proc_reg.h>
#include <ia32/pio.h>
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

struct i386_tss	       	tss;
struct i386_tss	       	cpu_tss[MAX_CPUS];
struct vm86tss		tss_vm86;


// used from direct VESA driver
void set_descriptor_limit( struct real_descriptor *d, unsigned limit )
{
    if (limit > 0xfffff)
    {
        limit >>= 12;
        d->granularity |= SZ_G;
    }
    else
        d->granularity &= ~SZ_G;
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
    // TODO need stacks alloc/free for intrs?
    tss.ss0 = KERNEL_DS;

    //tss.esp0 = get_esp(); // why?
    tss.esp0 = (int) (intr_stack+IS_SIZE-4);

    tss.io_bit_map_offset = sizeof(tss);

    //tss_vm86.tss.ss0 = KERNEL_DS_16; // WHY!!!?
    tss_vm86.tss.ss0 = KERNEL_DS;
    //tss.esp0 = get_esp(); // why?
    tss_vm86.tss.io_bit_map_offset = sizeof(tss_vm86);


    make_descriptor(gdt, MAIN_TSS, kvtolin(&tss), sizeof(tss)-1,
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

        cpu_tss[i].io_bit_map_offset = sizeof(tss);

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


static int      segsToUse = 0x10;

errno_t get_uldt_cs_ds(
                       linaddr_t cs_base, u_int16_t *ocs, size_t cs_limit,
                       linaddr_t ds_base, u_int16_t *ods, size_t ds_limit
                      )
{
    if( (segsToUse >> 3) + 1 >= LDTSZ )
        return ENOMEM;

    u_int16_t cs = segsToUse | 0x7;
    u_int16_t ds = segsToUse | 0xF;

    segsToUse += 0x10; // dajte dva


    fill_ldt_descriptor(cs, cs_base, 1 + (cs_limit-1)/PAGE_SIZE,
                        ACC_PL_U|ACC_CODE_R, SZ_32|SZ_G);

    fill_ldt_descriptor(ds, ds_base, 1 + (ds_limit-1)/PAGE_SIZE,
                        ACC_PL_U|ACC_DATA_W, SZ_32|SZ_G);

    *ocs = cs;
    *ods = ds;

    return 0;
}




