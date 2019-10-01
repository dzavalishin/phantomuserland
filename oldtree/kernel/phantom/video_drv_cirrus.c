/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Cirrus driver, incomplete, untested.
 *
**/

#ifdef ARCH_ia32

#define DEBUG_MSG_PREFIX "cirrus"
#include <debug_ext.h>
#define debug_level_flow 9
#define debug_level_error 10

#include <hal.h>
//#include <video.h>
#include <video/screen.h>
#include <video/internal.h>

#include <ia32/phantom_pmap.h>

#include <ia32/pio.h>
#include <ia32/vm86.h>
#include <phantom_libc.h>
#include <kernel/vm.h>
#include <kernel/bus/pci.h>

#include "video_drv_cirrus.h"

static void map_video(int on_off);
static void cirrus_unlock(void);

static void cirrus_load_cursol_palette(void);
static void cirrus_dump_cursol_palette(void) __attribute__((unused));

static void currus_set_cursor_pos(void);
static void cirrus_set_mouse_cursor( drv_video_bitmap_t *cursor );


#define CL_VENDOR 0x1013

static int cir_card = -1;

#define CHIP_ID p0

// p0 = chip id
static pci_table_t cir_devices[] =
{
    { CL_VENDOR, CIRRUS_ID_CLGD5422, 0, 0, 0, "CLGD-5422", 0, 0, 0x23, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5426, 0, 0, 0, "CLGD-5426", 0, 0, 0x24, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5424, 0, 0, 0, "CLGD-5424", 0, 0, 0x25, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5428, 0, 0, 0, "CLGD-5428", 0, 0, 0x26, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5430, 0, 0, 0, "CLGD-5430", 0, 0, 0x28, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5434, 0, 0, 0, "CLGD-5434", 0, 0, 0x2A, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5436, 0, 0, 0, "CLGD-5436", 0, 0, 0x2B, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5446, 0, 0, 0, "CLGD-5446", 0, 0, 0x2e, 0, 0, 0 },


    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } // final all-zero marker
};

static pci_table_t *cir_device = 0;
static unsigned int memsize = 0;


/*
CIRRUS_DETECT detect_info[]={
{ CLGD5426, 0x15, 0, "CLGD-5426", 0 },
{ CLGD5428, 0x18, 0, "CLGD-5428", 0 },
{ CLGD5429, 0x19, 0, "CLGD-5429", 1 },
{ CLGD5430, 0x32, 0xa0, "CLGD-5430", 2 },
{ CLGD5434, 0x31, 0xa4, "CLGD-5434", 3 },
{ CLGD5434E, 0x33, 0xa8, "CLGD-5434 rev E",4 },
{ CLGD5436, 0x36, 0xac, "CLGD-5436",5 },
{ CLGD5440, 0x32, 0xa0, "CLGD-5440",2 },//find by reading chip id register 0xb0
{ CLGD5446, 0x39, 0xb8, "CLGD-5446",5 },
{ CLGD5480, 0x3a, 0xbc, "CLGD-5480",6 },
{ CLGD7541, 0x41, 0x1204, "CLGD-7541",7 },
{ CLGD7542, 0x43, 0x1200, "CLGD-7542",-1 },
{ CLGD7543, 0x42, 0x1202, "CLGD-7543",7 },
{ CLGD7548, 0x44, 0x38, "CLGD-7548",-1 },
{ CLGD7555, 0x46, 0, "CLGD-7555",-1 },
{ CLGD7556,  0x47, 0, "CLGD-7556",-1 }
};
*/

/* from BIOS:

cirrus_check:
  push ax
  push dx
  mov ax, #0x9206
  mov dx, #0x3C4
  out dx, ax
  inc dx
  in al, dx
  cmp al, #0x12
  pop dx
  pop ax
  ret
 */


static int cirrus_probe()
{

    int part_id = read_vga_register( 0x3D4, 0x27 );
    SHOW_FLOW( 1, "Cirrus driver part Id (0x3D4,0x27) = 0x%X", part_id );

    {
        int old6 = read_vga_register( 0x3C4, 6 );
        write_vga_register( 0x3C4, 6, 0 );
        if(read_vga_register( 0x3C4, 6 ) == 0xF)
        {
            write_vga_register( 0x3C4, 6, 0x12 );
            if( 0x12 != read_vga_register( 0x3C4, 6 ) )
                goto fail;

            SHOW_FLOW( 1, "It seems to be a real cirrus 0x%X", part_id );

        }
        else
        {
        fail:
            write_vga_register( 0x3C4, 6, old6 );
            SHOW_FLOW0( 1, "Cirrus driver unable to identify chip directly");
            return VIDEO_PROBE_FAIL;
        }

    }


    pci_cfg_t cfg;

    cir_card = pci_find_any_in_table( &cfg, cir_devices );

    if( cir_card < 0 )
        return VIDEO_PROBE_FAIL; // No card

    cir_device = cir_devices+cir_card;

#if 1
    {
        memsize = 1*1024*1024;

        // QEMU gives this, and it is unlikelyu that we'll get real Cirrus :)
        int r15 = read_vga_register( 0x3C4, 0x15 );
        switch( r15 )
        {
        case 3: 	memsize = 2*1024*1024; break;
        case 4: 	memsize = 4*1024*1024; break;
        }
        SHOW_FLOW( 1, "Memsize r15=%x (%d Kb)", r15, memsize/1024 );
    }
#else
    {
        int dram = read_vga_register( 0x3C4, 0xF );
        int conf = read_vga_register( 0x3C4, 0x17 );

        switch(dram & CIRRUS_MEMSIZE_MASK)
        {
        case CIRRUS_MEMSIZE_512k: 	memsize = 512*1024; break;
        case CIRRUS_MEMSIZE_1M: 	memsize = 1024*1024; break;
        case CIRRUS_MEMSIZE_2M: 	memsize = 2*1024*1024; break;
        }

        if( conf & CIRRUS_MEMSIZEEXT_DOUBLE )
            memsize *= 2;

        // HACK on QEMU gives right mem size

        SHOW_FLOW( 1, "Memsize reg=%x (%d Kb) conf=%x", dram, memsize/1024, conf );

        if( dram & CIRRUS_MEMFLAGS_BANKSWITCH )
            SHOW_ERROR( 0, "Bank switch is active! memsize reg=%x", dram );
    }
#endif

    cirrus_unlock();

    //cirrus_dump_cursol_palette();
    cirrus_load_cursol_palette();
    //cirrus_dump_cursol_palette();

    write_vga_register( 0x3C4, 0x13, 48 ); // Take 4Kb for mouse cursor

    SHOW_FLOW( 1, "Cirrus %s found, chip id %x, expect %x", cir_devices[cir_card].name, part_id >> 2, cir_device->CHIP_ID );

    return VIDEO_PROBE_ACCEL;
}


static errno_t cirrus_accel_start(void)
{
    if( cir_device == 0 )
        return ENXIO;
#if 1 // doesn't work
    SHOW_FLOW( 0, "Cirrus accelerator add on start, chip %s", cir_device->name );

    // Take over mouse
    video_drv->mouse_redraw_cursor = currus_set_cursor_pos;
    video_drv->mouse_set_cursor = cirrus_set_mouse_cursor;

    // Turn off unused funcs
    video_drv->mouse_disable = vid_null;
    video_drv->mouse_enable = vid_null;
#else
    SHOW_FLOW( 0, "Cirrus accelerator disabled, chip %s", cir_device->name );
#endif

    return 0;
}

static int cirrus_start()
{
    map_video(1);
    //memset( video_driver_cirrus.screen, 0xFF, 20000 );
//getchar();
    // Allways OK
    return 0;
}

static int cirrus_stop()
{
    map_video(0);
    // Allways OK
    return 0;
}

struct drv_video_screen_t        video_driver_cirrus =
{
    "Cirrus SVGA",
    // size
    0, 0, 24,
    // mouse x y flags
    0, 0, 0,

    // screen
    0,

probe: 			cirrus_probe,
start: 			cirrus_start,
accel: 			cirrus_accel_start,
stop:  			cirrus_stop,

#if 0
update: 		vid_null,
bitblt: 		drv_video_bitblt_rev,
#if VIDEO_DRV_WINBLT
winblt:			drv_video_win_winblt_rev,
#endif
readblt: 		drv_video_readblt_rev,
bitblt_part:            drv_video_bitblt_part_rev,

mouse:  		vid_null,
#endif

mouse_redraw_cursor: 	currus_set_cursor_pos,
mouse_set_cursor:       cirrus_set_mouse_cursor,
mouse_disable:          vid_null,
mouse_enable:          	vid_null,
};


static physaddr_t video_driver_cirrus_pa;
static int n_pages;
void set_video_driver_cirrus_pa( physaddr_t pa, size_t size )
{
    n_pages = ((size-1)/hal_mem_pagesize())+1;

    video_driver_cirrus_pa = pa;

    void *vva;
    if( hal_alloc_vaddress(&vva, n_pages) )
        panic("Can't alloc vaddress for %d videmem pages", n_pages);

    video_driver_cirrus.screen = vva;

    SHOW_FLOW( 1, "videomem vaddr = %X, %d pages, %d mbytes\n", vva, n_pages, n_pages*4/1024 );
}

#if 1
static void map_video(int on_off)
{
    (void) on_off;
    panic("no cirrus driver yet!");
}
#else
static void map_video(int on_off)
{
    assert( video_driver_bios_vesa.screen != 0 );

    hal_pages_control_etc(
                          video_driver_cirrus_pa,
                          video_driver_cirrus.screen,
                          n_pages, page_map_io, page_rw, 0 );
}

#endif



/* Unlock chipset-specific registers */

static void cirrus_unlock(void)
{
    int vgaIOBase, temp;

    /* Are we Mono or Color? */
    vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;

    outb(0x3C4, 0x06);
    outb(0x3C5, 0x12);		/* unlock cirrus special */

    /* Put the Vert. Retrace End Reg in temp */

    outb(vgaIOBase + 4, 0x11);
    temp = inb(vgaIOBase + 5);

    /* Put it back with PR bit set to 0 */
    /* This unprotects the 0-7 CRTC regs so */
    /* they can be modified, i.e. we can set */
    /* the timing. */

    outb(vgaIOBase + 5, temp & 0x7F);
}



static void currus_set_cursor_pos(void)
{
    unsigned x = video_drv->mouse_x;
    unsigned y = scr_get_ysize() - video_drv->mouse_y;

    SHOW_FLOW( 10, "%d * %d", x, y );
#if 0
    outb( 0x3C4, CIRRUS_CURSOR_SHOW );
    outb( 0x3C4, 0x10 | ((x & 7) << 5) );
    outb( 0x3C4, 0x11 | ((y & 7) << 5) );
#else
    write_vga_register( 0x3C4, 0x12, CIRRUS_CURSOR_SHOW );

    write_vga_register( 0x3C4, 0x10 | ((x & 7) << 5), x >> 3 );
    write_vga_register( 0x3C4, 0x11 | ((y & 7) << 5), y >> 3 );
#endif
}


static void cirrus_load_cursol_palette(void)
{
    // hidden palette, 16 entries - for mouse cursor

    write_vga_register( 0x3C4, 0x12, CIRRUS_CURSOR_HIDDENPEL|CIRRUS_CURSOR_SHOW );
    outb( 0x3C8, 0 );

    int i = 16*3; // 16 rgb
    while( i-- > 0 )
        outb( 0x3C9, 0x2F );

    outb( 0x3C8, 0x0F );

    i = 16*3; // 16 rgb
    while( i-- > 0 )
        outb( 0x3C9, 0xAF );


    write_vga_register( 0x3C4, 0x12, CIRRUS_CURSOR_SHOW );
}

static void cirrus_dump_cursol_palette(void)
{
    // hidden palette, 16 entries - for mouse cursor

    write_vga_register( 0x3C4, 0x12, CIRRUS_CURSOR_HIDDENPEL|CIRRUS_CURSOR_SHOW );
    outb( 0x3C8, 0 );

    int i = 16; // 16 rgb
    while( i-- > 0 )
    {
        int r = inb( 0x3C9 );
        int g = inb( 0x3C9 );
        int b = inb( 0x3C9 );

        printf("%2x %2x %2x\n", r, g, b );
    }

    write_vga_register( 0x3C4, 0x12, CIRRUS_CURSOR_SHOW );
}


static void cirrus_set_mouse_cursor( drv_video_bitmap_t *cursor )
{
    u_int8_t *dst = (u_int8_t *) ((addr_t)video_drv->screen + memsize - 16 * 1024);
    SHOW_FLOW( 0, "vram %p, dst %p", video_drv->screen, dst );

    // optimal value is 48, it takes exact 4Kb from vram
    int r13 = read_vga_register( 0x3C4, 0x13 );
    dst += (r13 & 0x3f) * 256;
    SHOW_FLOW( 0, "r13 %d, dst %p", r13, dst );

    // We use 32x32 cursor

    //memmove( dst, cursor->pixel, cbytes ); // TODO wrong - need conversion -- OVERWRITTEN BELOW

#if 0
    int cbytes = 32*32*2/8; // 32 x 32 bits * 2 planes / 8 bits in byte
    memset( dst, 0xFF, cbytes );
    //SHOW_ERROR0( 0, "not impl");
#else
    u_int32_t *p = (void *)dst;

    int i, j;

    const int shift_up = 32 - cursor->ysize;
    const int shift_left = 32 - cursor->xsize;

    rgba_t get_cp( int x, int y )
    {
        y = 31 - y;

        y -= shift_up;
        x -= shift_left;

        if( x < 0 ) return (rgba_t) { 0, 0, 0, 0 };
        if( y < 0 ) return (rgba_t) { 0, 0, 0, 0 };
        if( x >= cursor->xsize ) return (rgba_t) { 0, 0, 0, 0 };
        if( y >= cursor->ysize ) return (rgba_t) { 0, 0, 0, 0 };

        return cursor->pixel[ (y*cursor->xsize) + x ];
    }


    for( i=0; i < 32; i++ )
    {
        u_int32_t andMask = 0;
        u_int32_t xorMask = 0;


        for ( j=0; j < 32; j++ )
        {
            rgba_t pixel = get_cp( 31-j, i );
            if( pixel.a )
            {
                if( pixel.r || pixel.g || pixel.b )
                {
                    andMask|= 1 << j;//foreground
                    xorMask|= 1 << j;
                }
                else
                    xorMask|= 1 << j;//background
            }
            //andMask|=(1<<j);//inverted

        }


        p[0]  = andMask;
        p[32] = xorMask;
        p++;
    }
#endif

}



















#if 0

/* Indentify chipset, initialize and return non-zero if detected */

static int cirrus_test(void)
{
    int oldlockreg;
    int lockreg;

    outb(0x3c4, 0x06);
    oldlockreg = inb(0x3c5);

    cirrus_unlock();

    /* If it's a Cirrus at all, we should be */
    /* able to read back the lock register */

    outb(0x3C4, 0x06);
    lockreg = inb(0x3C5);

    /* Ok, if it's not 0x12, we're not a Cirrus542X. */
    if (lockreg != 0x12) {
	outb(0x3c4, 0x06);
	outb(0x3c5, oldlockreg);
	return 0;
    }
    /* The above check seems to be weak, so we also check the chip ID. */

    outb(__svgalib_CRT_I, 0x27);
    switch (inb(__svgalib_CRT_D) >> 2) {
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:			/* 5429 */
    case 0x28:			/* 5430 */
    case 0x2A:			/* 5434 */
    case 0x2B:			/* 5436 */
	break;
    default:
	outb(0x3c4, 0x06);
	outb(0x3c5, oldlockreg);
	return 0;
    }

    if (cirrus_init(0, 0, 0))
	return 0;		/* failure */
    return 1;
}

/* Set display start address (not for 16 color modes) */
/* Cirrus supports any address in video memory (up to 2Mb) */

static void cirrus_setdisplaystart(int address)
{
    outw(0x3d4, 0x0d + ((address >> 2) & 0x00ff) * 256);	/* sa2-sa9 */
    outw(0x3d4, 0x0c + ((address >> 2) & 0xff00));	/* sa10-sa17 */
    inb(0x3da);			/* set ATC to addressing mode */
    outb(0x3c0, 0x13 + 0x20);	/* select ATC reg 0x13 */
    /* Cirrus specific bits 0,1 and 18,19,20: */
    outb(0x3c0, (inb(0x3c1) & 0xf0) | (address & 3));
    /* write sa0-1 to bits 0-1; other cards use bits 1-2 */
    outb(0x3d4, 0x1b);
    outb(0x3d5, (inb(0x3d5) & 0xf2)
	 | ((address & 0x40000) >> 18)	/* sa18: write to bit 0 */
	 |((address & 0x80000) >> 17)	/* sa19: write to bit 2 */
	 |((address & 0x100000) >> 17));	/* sa20: write to bit 3 */
    outb(0x3d4, 0x1d);
    if (cirrus_memory > 2048)
	outb(0x3d5, (inb(0x3d5) & 0x7f)
	     | ((address & 0x200000) >> 14));	/* sa21: write to bit 7 */
}


static int cirrus_linear(int op, int param)
{
    if (op == LINEAR_ENABLE) {
	cirrus_setlinear(0xE);
	return 0;
    }
    if (op == LINEAR_DISABLE) {
	cirrus_setlinear(0);
	return 0;
    }
    if (cirrus_chiptype >= CLGD5424 && cirrus_chiptype <= CLGD5429) {
	if (op == LINEAR_QUERY_BASE) {
	    if (param == 0)
		return 0xE00000;	/* 14MB */
	    /*
	     * Trying 64MB on a system with 16MB of memory is unsafe if the
	     * card maps at 14MB. 14 MB was not attempted because of the
	     * system memory check in vga_setlinearaddressing(). However,
	     * linear addressing is enabled when looking at 64MB, causing a
	     * clash between video card and system memory at 14MB.
	     */
	    if (__svgalib_physmem() <= 13 * 1024 * 1024) {
		if (param == 1)
		    return 0x4000000;	/* 64MB */
		if (param == 2)
		    return 0x4E00000;	/* 78MB */
		if (param == 3)
		    return 0x2000000;	/* 32MB */
		if (param == 4)
		    return 0x3E00000;	/* 62MB */
	    }
	    return -1;
	}
    }
    if (cirrus_chiptype >= CLGD5430) {
	if (op == LINEAR_QUERY_BASE) {
	    if (param == 0)
		return 0x04000000;	/* 64MB */
	    if (param == 1)
		return 0x80000000;	/* 2048MB */
	    if (param == 2)
		return 0x02000000;	/* 32MB */
	    if (param == 3)
		return 0x08000000;	/* 128MB */
	    /* While we're busy, try some common PCI */
	    /* motherboard-configured addresses as well. */
	    /* We only read, so should be safe. */
	    if (param == 4)
		return 0xA0000000;
	    if (param == 5)
		return 0xA8000000;
	    if (param == 6)
		return 0xF0000000;
	    if (param == 7)
		return 0XFE000000;
	    /*
	     * Some PCI/VL motherboards only seem to let the
	     * VL slave slot respond at addresses >= 2048MB.
	     */
	    if (param == 8)
		return 0x84000000;
	    if (param == 9)
		return 0x88000000;
	    return -1;
	}
    }
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY)
	return 0;		/* No granularity or range. */
    else
	return -1;		/* Unknown function. */
}




#endif





#endif // ARCH_ia32


