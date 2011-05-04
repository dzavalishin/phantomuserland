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

//#ifdef ARCH_ia32

#define DEBUG_MSG_PREFIX "video-intel"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include "hal.h"
#include <kernel/vm.h>
#include <x86/phantom_pmap.h>

#include <i386/pio.h>
#include <phantom_libc.h>

#include "video.h"
#include "video_drv_i810_vbe.h"

static int i810_video_probe();
static int i810_video_start();
static int i810_video_stop();




struct drv_video_screen_t        video_driver_i810 =
{
    "Intel810",
    // size
    800, 600,
    // mouse x y flags
    0, 0, 0,

    // screen
screen:			0,

probe: 			i810_video_probe,
start: 			i810_video_start,
stop:   		i810_video_stop,

update: 		drv_video_null,
bitblt: 		drv_video_bitblt_rev,
winblt:			drv_video_win_winblt_rev,
readblt: 		drv_video_readblt_rev,

mouse:    		drv_video_null,

redraw_mouse_cursor: 	drv_video_draw_mouse_deflt,
set_mouse_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,

};






static int n_pages = 1024;
static int i810_video_probe()
{

    if( hal_alloc_vaddress((void **)&video_driver_i810_vesa_emulator.screen, n_pages) )
        panic("Can't alloc vaddress for %d videmem pages", n_pages);

    SHOW_FLOW( 7, "vmem va 0x%X", video_driver_i810_vesa_emulator.screen);


    printf("Intel board 0x%x found\n", id);
    return 1;
}






static void i810_map_video(int on_off)
{
    assert( video_driver_i810_vesa_emulator.screen != 0 );

    hal_pages_control_etc(
                          VBE_DISPI_LFB_PHYSICAL_ADDRESS,
                          video_driver_i810_vesa_emulator.screen,
                          n_pages, on_off ? page_map : page_unmap, page_rw,
                          INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );
}

static int i810_video_start()
{
    i810_map_video( 1 );
    return 0;
}

static int i810_video_stop()
{
    i810_map_video(0);
    
    video_drv_basic_vga_set_text_mode();
    return 0;
}




//#endif // ARCH_ia32




























