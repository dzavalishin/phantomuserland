/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Protected mode VESA driver, incomplete, untested.
 *
 *
**/

#ifdef ARCH_ia32

#define DEBUG_MSG_PREFIX "video"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <hal.h>
//#include <video.h>
#include <video/screen.h>
#include <video/internal.h>

//pressEnter
#include "misc.h"

#include <sys/cdefs.h>
#include <sys/types.h>
#include <errno.h>

#include <ia32/pio.h>
#include <ia32/seg.h>
#include <phantom_libc.h>

#include <kernel/vm.h>
#include <kernel/trap.h>
#include <kernel/page.h>

#include <ia32/private.h>

#define DO16BIT 0

#if DO16BIT
static errno_t load_pm_vesa( void *ROM_va, size_t ROM_size, size_t hdr_offset );
#endif

static int direct_vesa_probe();

// First three are really 16 bit, use 32 just to make things cleaner
errno_t call_16bit_code( u_int32_t cs, u_int32_t ss, u_int32_t entry, struct trap_state *ts );

struct drv_video_screen_t        video_driver_direct_vesa =
{
    "Protected mode Vesa",
    // size
    0, 0, 24,
    // mouse x y flags
    0, 0, 0,

    // screen
    0,

probe: 	direct_vesa_probe,
start: 	(void *)vid_null,
stop:  	(void *)vid_null,

#if 0
update:	vid_null,
bitblt: (void *)vid_null,
winblt: (void *)vid_null,
readblt: (void *)vid_null,
bitblt_part:            drv_video_bitblt_part_rev,

mouse:  vid_null,

redraw_mouse_cursor: 	drv_video_draw_mouse_deflt,
set_mouse_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,
#endif
};



struct pm_vesa_header
{
    char         signature[4];
    u_int16_t    entryPoint;    // offset of main entry point
    u_int16_t    entryInit;     // offset of init func
    u_int16_t    biosDataSel;   // at least 0x600 bytes of scratch for BIOS code - clean
    u_int16_t    A0000Sel;
    u_int16_t    B0000Sel;
    u_int16_t    B8000Sel;
    u_int16_t    codeAsDataSel; // access code seg as data
    u_int8_t     protectedMode;
    u_int8_t     checksum;
} __packed;


#define PBUF 1
#define ROM_SIZE (32*1024)

static int direct_vesa_probe()
{
    const int ROM_pa = 0xC0000;
    const int ROM_size = ROM_SIZE;

#if PBUF
    char ROM_va[ROM_SIZE];
    memcpy_p2v( ROM_va, ROM_pa, ROM_SIZE );
#else
    const int ROM_pages = ROM_size/PAGE_SIZE;
    char *ROM_va = (char *)phystokv(ROM_pa);
    hal_pages_control( ROM_pa, ROM_va, ROM_pages, page_map, page_ro );
#endif

    char *p = ROM_va;
    SHOW_FLOW( 0, "Look for VESA PM entry starting at 0x%X", ROM_pa);

    char *entry = 0;
    int cnt = ROM_size;
    while(cnt--)
    {
        if(
           p[0] == 'P' && p[1] == 'M' &&
           p[2] == 'I' && p[3] == 'D' )
        {
            SHOW_FLOW( 0, "Found VESA PM entry at 0x%X", p);
            entry = p;
            break;
        }
        p++;
    }

    if(entry == NULL)
        SHOW_FLOW0( 0, "no VESA PM entry");
    else
    {
        //struct pm_vesa_header hdr = *((struct pm_vesa_header *)entry);
        int hdr_offset = entry - ROM_va;

        int c = sizeof(struct pm_vesa_header);

        char csum = 0;

        while(c--)
            csum += *entry++;

        if(csum)
            SHOW_ERROR0( 0, "VESA PM entry checksum is wrong");
        else
        {
#if DO16BIT
            SHOW_FLOW0( 1, "gettig VESA PM BIOS copy");
            load_pm_vesa( ROM_va, ROM_size, hdr_offset );
#else
            (void) hdr_offset;
            (void) ROM_va;
            (void) ROM_size;

            struct pm_vesa_header *h = ((void *)ROM_va)+hdr_offset;

            SHOW_FLOW( 1, "init: %x, entry %x", h->entryInit, h->entryPoint );
#endif
        }
    }

#if !PBUF
    hal_pages_control( ROM_pa, ROM_va, ROM_pages, page_unmap, page_ro );
#endif
pressEnter("PM VESA done");

    // Experimental code
    return VIDEO_PROBE_FAIL;
}


// TODO this is X86-specific code, move to i386 dir

#define BDA_size 0x600
#define STK_size 1024
#define DBA_size 128*1024

#if DO16BIT
static void *data_buffer;

static errno_t load_pm_vesa( void *in_ROM_va, size_t ROM_size, size_t hdr_offset )
{
    // ROM copy
    physaddr_t	ROM_pa;
    void *	ROM_va;

    // Bios Data Area
    physaddr_t	BDA_pa;
    void *	BDA_va;

    // Stack
    physaddr_t	STK_pa;
    void *	STK_va;

    // Data Buffer Area (for 32-16 bit data exchange)
    physaddr_t	DBA_pa;
    void *	DBA_va;

    hal_pv_alloc( &ROM_pa, &ROM_va, ROM_size );
    hal_pv_alloc( &BDA_pa, &BDA_va, BDA_size );
    hal_pv_alloc( &STK_pa, &STK_va, STK_size );
    hal_pv_alloc( &DBA_pa, &DBA_va, DBA_size );

    data_buffer = DBA_va;

    struct pm_vesa_header *hdr = ROM_va+hdr_offset;

    assert(hdr->signature[0] == 'P' && hdr->signature[1] == 'M' &&
           hdr->signature[2] == 'I' && hdr->signature[3] == 'D' );


    memcpy( ROM_va, in_ROM_va, ROM_size );
    memset( BDA_va, 0, BDA_size );

    hdr->biosDataSel = VBE3_BD_16;
    hdr->A0000Sel = VBE3_A0_16;
    hdr->B0000Sel = VBE3_B0_16;
    hdr->B8000Sel = VBE3_B8_16;
    hdr->codeAsDataSel = VBE3_DS_16;
    hdr->protectedMode = 0xFF;

    // load segment descriptors!

    make_descriptor(gdt, VBE3_CS_16, ROM_pa, ROM_size-1, ACC_PL_K | ACC_CODE_R, SZ_16 );
    make_descriptor(gdt, VBE3_DS_16, ROM_pa, ROM_size-1, ACC_PL_K | ACC_DATA_W, SZ_16 );
    make_descriptor(gdt, VBE3_BD_16, BDA_pa, BDA_size-1, ACC_PL_K | ACC_DATA_W, SZ_16 );
    make_descriptor(gdt, VBE3_ST_16, STK_pa, STK_size-1, ACC_PL_K | ACC_DATA_W, SZ_16 );
    make_descriptor(gdt, VBE3_DB_16, DBA_pa, DBA_size-1, ACC_PL_K | ACC_DATA_W, SZ_16 );

    make_descriptor(gdt, VBE3_A0_16, 0xA0000, 0xffff, ACC_PL_K | ACC_DATA_W, SZ_16 );
    make_descriptor(gdt, VBE3_B0_16, 0xB0000, 0xffff, ACC_PL_K | ACC_DATA_W, SZ_16 );
    make_descriptor(gdt, VBE3_B8_16, 0xB8000, 0xffff, ACC_PL_K | ACC_DATA_W, SZ_16 );


    int ie = hal_save_cli();

#if 0
    struct trap_state ts;
    errno_t ret;
    if( (ret = call_16bit_code( VBE3_CS_16, VBE3_ST_16, hdr->entryInit, &ts )) )
        goto err;

    if( (ret = call_16bit_code( VBE3_CS_16, VBE3_ST_16, hdr->entryPoint, &ts )) )
        goto err;
#endif
err:
    if(ie) hal_sti();

    return 0;
}

#endif


#if 0
#ifdef HAVE_CODE16GCC
#define CODE16_STRING ".code16gcc"
#else
#define CODE16_STRING ".code16"
#endif


/* Switch GAS into 16-bit mode.  */
#define CODE16 asm(CODE16_STRING);

/* Switch back to 32-bit mode.  */
#define CODE32 asm(".code32");


/*
 * Macros to switch between 16-bit and 32-bit code
 * in the middle of a C function.
 * Be careful with these!
 * If you accidentally leave the assembler in the wrong mode
 * at the end of a function, following functions will be assembled wrong
 * and you'll get very, very strange results...
 * It's safer to use the higher-level macros below when possible.
 */
#define i16_switch_to_32bit(cs32) asm volatile("\
	ljmp	%0,$1f\
        .code32\
	1:\
	" : : "i" (cs32));
#define switch_to_16bit(cs16) asm volatile("\
		ljmp	%0,$1f\
		"CODE16_STRING"\
	1:\
	" : : "i" (cs16));
#endif




errno_t call_16bit_code( u_int32_t cs, u_int32_t ss, u_int32_t entry, struct trap_state *ts )
{
    (void) cs;
    (void) ss;
    (void) entry;
    (void) ts;
    // need AX, BX, CX, DX, ES, DI

    // load regs
    // set stack

    //asm volatile("lcall	%0,%1" : : "i" (cs), "i" (entry));

    // return to old stack
    // store regs

    return ENXIO;
}


#endif // ARCH_ia32

