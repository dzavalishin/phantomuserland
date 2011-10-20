/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Protected mode BOCHS virtual video card driver.
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
#include <kernel/vm.h>
#include <ia32/phantom_pmap.h>

#include <ia32/pio.h>
#include <phantom_libc.h>

//#include <video.h>
#include <video/screen.h>
#include <video/internal.h>

#include "video_drv_bochs_vbe.h"

static int bochs_video_probe();
static int bochs_video_start();
static int bochs_video_stop();


// TODO - try 32bpp?

struct drv_video_screen_t        video_driver_bochs_vesa_emulator =
{
    "Bochs VBE emulator",
    // size
    BOCHS_VIDEO_DRV_DEFAULT_X_SIZE, BOCHS_VIDEO_DRV_DEFAULT_Y_SIZE,
    // bpp
    24,
    // mouse x y flags
    0, 0, 0,

    // screen
screen:			0,

probe: 			bochs_video_probe,
start: 			bochs_video_start,
stop:   		bochs_video_stop,

update: 		drv_video_null,
bitblt: 		drv_video_bitblt_rev,
winblt:			drv_video_win_winblt_rev,
readblt: 		drv_video_readblt_rev,
bitblt_part:            drv_video_bitblt_part_rev,

mouse:    		drv_video_null,

redraw_mouse_cursor: 	drv_video_draw_mouse_deflt,
set_mouse_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,

};





static void vbe_write(unsigned short index, unsigned short value)
{
   outw(VBE_DISPI_IOPORT_INDEX, index);
   outw(VBE_DISPI_IOPORT_DATA, value);
}

static int vbe_read(unsigned short index)
{
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}


// Return nonzero on err
int vbe_set(unsigned short xres, unsigned short yres, unsigned short bpp)
{
    vbe_write(VBE_DISPI_INDEX_ID, VBE_DISPI_ID2);
    int id = vbe_read(VBE_DISPI_INDEX_ID);

    printf("VBE ver 0x%x... ", id);
    //getchar();

    if( id < VBE_DISPI_ID2 || id > VBE_DISPI_ID3 )
        return 1;

    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write(VBE_DISPI_INDEX_XRES, xres);
    vbe_write(VBE_DISPI_INDEX_YRES, yres);
    vbe_write(VBE_DISPI_INDEX_BPP, bpp);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    return 0;
}

static int n_pages = 1024;
static int bochs_video_probe()
{
    //printf("Probing for ");
    vbe_write(VBE_DISPI_INDEX_ID, VBE_DISPI_ID2);
    int id = vbe_read(VBE_DISPI_INDEX_ID);

    if( id < VBE_DISPI_ID2 || id > VBE_DISPI_ID3 )
        return VIDEO_PROBE_FAIL;

    if( hal_alloc_vaddress((void **)&video_driver_bochs_vesa_emulator.screen, n_pages) )
        panic("Can't alloc vaddress for %d videmem pages", n_pages);

    SHOW_FLOW( 7, "vmem va 0x%X", video_driver_bochs_vesa_emulator.screen);


    printf("Bochs VBE emulator ver 0x%x found\n", id);
    return VIDEO_PROBE_SUCCESS;
}




#define VBE_DISPI_LFB_PHYSICAL_ADDRESS  0xE0000000


static void bochs_map_video(int on_off)
{
    assert( video_driver_bochs_vesa_emulator.screen != 0 );

    hal_pages_control_etc(
                          VBE_DISPI_LFB_PHYSICAL_ADDRESS,
                          video_driver_bochs_vesa_emulator.screen,
                          n_pages, on_off ? page_map : page_unmap, page_rw,
                          INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );
}

static int bochs_video_start()
{
    bochs_map_video( 1 );
    return vbe_set(BOCHS_VIDEO_DRV_DEFAULT_X_SIZE, BOCHS_VIDEO_DRV_DEFAULT_Y_SIZE, 24);
}

static int bochs_video_stop()
{
    bochs_map_video(0);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    video_drv_basic_vga_set_text_mode();
    return 0;
}




#endif // ARCH_ia32






