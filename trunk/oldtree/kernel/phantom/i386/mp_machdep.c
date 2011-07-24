/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MP - processor start, trampoline
 *
**/

#define DEBUG_MSG_PREFIX "smp"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <assert.h>
#include <string.h>
#include <kernel/page.h>

#include <i386/proc_reg.h>

#include "smp-imps.h"

/* targets for relocation */
extern void bigJump(void);
extern void bootCodeSeg(void);
extern void bootDataSeg(void);
extern void MPentry(void);

extern u_int 	MP_GDT;
extern u_int 	mp_gdtbase;
extern int      bootMP_size;

extern char     bootMP[]; // Boot code

void
install_ap_tramp(void *boot_address)
{
    //int     x;
    int     size = *(int *) ((u_long) & bootMP_size);
    vm_offset_t va = (vm_offset_t)boot_address;
    u_char *src = (u_char *) ((u_long) bootMP);
    u_char *dst = (u_char *) va;
    u_int   boot_base = (u_int) bootMP;
    u_int8_t *dst8;
    u_int16_t *dst16;
    u_int32_t *dst32;

    //KASSERT (size <= PAGE_SIZE, ("'size' do not fit into PAGE_SIZE, as expected."));
    assert(size <= PAGE_SIZE);

    //pmap_kenter(va, boot_address);
    //pmap_invalidate_page (kernel_pmap, va);

    //for (x = 0; x < size; ++x)		*dst++ = *src++;

    memcpy( dst, src, size );

    /*
     * modify addresses in code we just moved to basemem. unfortunately we
     * need fairly detailed info about mpboot.s for this to work.  changes
     * to mpboot.s might require changes here.
     */

    /* boot code is located in KERNEL space */
    dst = (u_char *) va;

    /* modify the lgdt arg */
    dst32 = (u_int32_t *) (dst + ((u_int) & mp_gdtbase - boot_base));
    *dst32 = (vm_offset_t)boot_address + ((u_int) & MP_GDT - boot_base);

    /* modify the ljmp target for MPentry() */
    dst32 = (u_int32_t *) (dst + ((u_int) bigJump - boot_base) + 1);
    //*dst32 = ((u_int) MPentry - KERNBASE);
    *dst32 = ((u_int) MPentry);

    /* modify the target for boot code segment */
    dst16 = (u_int16_t *) (dst + ((u_int) bootCodeSeg - boot_base));
    dst8 = (u_int8_t *) (dst16 + 1);
    *dst16 = (u_int) boot_address & 0xffff;
    *dst8 = ((u_int) boot_address >> 16) & 0xff;

    /* modify the target for boot data segment */
    dst16 = (u_int16_t *) (dst + ((u_int) bootDataSeg - boot_base));
    dst8 = (u_int8_t *) (dst16 + 1);
    *dst16 = (u_int) boot_address & 0xffff;
    *dst8 = ((u_int) boot_address >> 16) & 0xff;


    //dump_mp_gdt( (void *) (va + ((u_int) & MP_GDT - boot_base)) );
}



