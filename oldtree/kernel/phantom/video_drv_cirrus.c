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
#include <video.h>
#include <video/screen.h>

#include <ia32/phantom_pmap.h>

#include <ia32/pio.h>
#include <ia32/vm86.h>
#include <phantom_libc.h>
#include <kernel/vm.h>

#include "video_drv_cirrus.h"

static void map_video(int on_off);

static int cir_card = -1;

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
#if HAVE_VESA
    RM_REGS regs;
    int i, card;

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

    regs.x.ax=0x1200;  //cirrus extended bios
    regs.x.bx=0x81;    //get BIOS version
    phantom_bios_int_10_args(&regs);

    SHOW_FLOW( 1, "Cirrus driver BIOS 0x1200 81 ax=0x%X", regs.x.ax );

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

    if( cir_card == -1 )
        return 0; // No card


    SHOW_FLOW( 1, "Cirrus %s found\n", detect_info[cir_card].desc );

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
                          n_pages, page_map, page_rw,
                          INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );
}

#endif







/*

Int 10/AH=12h/BL=80h

----------------------

Cirrus Logic BIOS - INQUIRE VGA TYPE

AH = 12h
BL = 80h

Return:
AX = controller type in bits 13-0 (see #0028)

bit 14:
???

bit 15:
???. BL = silicon revision number (bit 7 set if not available). BH = ??? bit 2 set if using CL-GD 6340 LCD interface

See Also: AH=12h/BL=81h - AH=12h/BL=82h - AH=12h/BL=85h - AH=12h/BL=9Ah - AH=12h/BL=A1h

(Table 0028) Values for Cirrus Logic video controller type: 0000h no extended alternate select support 0001h reserved 0002h CL-GD510/520 0003h CL-GD610/620 0004h CL-GD5320 0005h CL-GD6410 0006h CL-GD5410 0007h CL-GD6420 0008h CL-GD6412 0010h CL-GD5401 0011h CL-GD5402 0012h CL-GD5420 0013h CL-GD5422 0014h CL-GD5424 0015h CL-GD5426 0016h CL-GD5420r1 0017h CL-GD5402r1 0018h CL-GD5428 0019h CL-GD5429 0020h CL-GD6205/15/25 0021h CL-GD6215 0022h CL-GD6225 0023h CL-GD6235 0024h CL-GD6245 0030h CL-GD5432 0031h CL-GD5434 0032h CL-GD5430 0033h CL-GD5434 rev. E and F 0035h CL-GD5440 0036h CL-GD5436 0039h CL-GD5446 0040h CL-GD6440 0041h CL-GD7542 (Nordic) 0042h CL-GD7543 (Viking) 0043h CL-GD7541 (Nordic Lite) 0050h CL-GD5452 (Northstar) 0052h CL-GD5452 (Northstar) ???

See Also: #0706 - #0717

Category: Video - Int 10h - C

----------------------






*/

#endif // ARCH_ia32


