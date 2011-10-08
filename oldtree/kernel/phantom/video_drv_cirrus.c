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

#define DEBUG_MSG_PREFIX "video"
#include <debug_ext.h>
static int debug_level_flow = 1;

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

#define CL_VENDOR 0x1013

static int cir_card = -1;

static pci_table_t cir_devices[] =
{
    { CL_VENDOR, CIRRUS_ID_CLGD5422, 0, 0, 0, "CLGD-5422", 0, 0, 0, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5426, 0, 0, 0, "CLGD-5426", 0, 0, 0, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5424, 0, 0, 0, "CLGD-5424", 0, 0, 0, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5428, 0, 0, 0, "CLGD-5428", 0, 0, 0, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5430, 0, 0, 0, "CLGD-5430", 0, 0, 0, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5434, 0, 0, 0, "CLGD-5434", 0, 0, 0, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5436, 0, 0, 0, "CLGD-5436", 0, 0, 0, 0, 0, 0 },
    { CL_VENDOR, CIRRUS_ID_CLGD5446, 0, 0, 0, "CLGD-5446", 0, 0, 0, 0, 0, 0 },


    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } // final all-zero marker
};

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
    int i;

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
        }

    }

#if HAVE_VESA
#if 0
    RM_REGS regs;
    regs.x.ax=0x1200;  //cirrus extended bios
    regs.x.bx=0x81;    //get BIOS version
    phantom_bios_int_10_args(&regs);

    SHOW_FLOW( 1, "Cirrus driver BIOS 0x1200 81 ax=0x%X", regs.x.ax );

    int card;
    regs.x.ax=0x1200;  //cirrus extended bios
    regs.x.bx=0x80;    //get chip version
    phantom_bios_int_10_args(&regs);
    card=regs.h.al;

    SHOW_FLOW( 1, "Cirrus driver BIOS 0x1200 80 al=0x%X", card );

    for (i=0;i<KNOWN_CARDS;i++)
    {
        if (card == detect_info[i].biosnum)
        {
            cir_card=i;
            break;
        }
    }
#endif

    pci_cfg_t cfg;

    cir_card = pci_find_any_in_table( &cfg, cir_devices );

    if( cir_card < 0 )
        return 0; // No card



    SHOW_FLOW( 1, "Cirrus %s found\n", cir_devices[cir_card].name );

    return 0;
#else // vesa
    return 0;
#endif // vesa
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
stop:  			cirrus_stop,

update: 		drv_video_null,
bitblt: 		drv_video_bitblt_rev,
winblt:			drv_video_win_winblt_rev,
readblt: 		drv_video_readblt_rev,

mouse:  		drv_video_null,

redraw_mouse_cursor: 	drv_video_draw_mouse_deflt,
set_mouse_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,
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








#endif // ARCH_ia32


