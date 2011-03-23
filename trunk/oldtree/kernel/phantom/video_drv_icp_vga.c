/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM Integrator/CP video card driver.
 *
 *
**/

#ifdef ARCH_arm

#define DEBUG_MSG_PREFIX "video-icp"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <hal.h>
#include <kernel/vm.h>
#include <x86/phantom_pmap.h>

//#include <i386/pio.h>
#include <phantom_libc.h>

#include <video.h>
//#include "video_drv_icp_vga.h"

#define VRAM_PHYSICAL_ADDRESS  0x00200000

// TODO move to integrator/cp header
#define CP_IDFIELD 0xCB000000
#define MEM(___a) *((int *)___a)



static int icp_video_probe();
static int icp_video_start();
static int icp_video_stop();



struct drv_video_screen_t        video_driver_icp =
{
    "Integrator/CP",
    // size
    640, 480,
    // mouse x y flags
    0, 0, 0,

    // screen
screen:			0,

probe: 			icp_video_probe,
start: 			icp_video_start,
stop:   		icp_video_stop,

update: 		drv_video_null,
bitblt: 		drv_video_bitblt_rev,
winblt:			drv_video_win_winblt_rev,
readblt: 		drv_video_readblt_rev,

mouse:    		drv_video_null,

#if 1
redraw_mouse_cursor: 	(void *)drv_video_null,
set_mouse_cursor: 	(void *)drv_video_null,
mouse_disable:          (void *)drv_video_null,
mouse_enable:          	(void *)drv_video_null,
#else
redraw_mouse_cursor: 	drv_video_draw_mouse_deflt,
set_mouse_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,
#endif
};

// TODO temp!
//struct drv_video_screen_t        *video_drv = &video_driver_icp;


#define ICP_ID 0x41034003



//static int n_pages = 1024;
static int icp_video_probe()
{
    // TODO switch to virtual on paging start?
    video_driver_icp.screen = (void *)VRAM_PHYSICAL_ADDRESS;

    int id = MEM(CP_IDFIELD);
    if( id != ICP_ID)
    {
        printf("Integrator/CP oard id is %x, not %x\n", id, ICP_ID );
        return 0;
    }

    volatile unsigned int *lp = (void *)0xC0000000;

    /*
    lp[0x10>>2] = (int)VRAM_PHYSICAL_ADDRESS;
    lp[0x1C>>2] =
        1 | // controller enable
        (2 << 1) | // 4 bpp
        0
        ;
    */

    // this sets 640*480, TODO 800*600

    MEM(0x10000014) = 0xA05F;
    MEM(0x1000001C) = 0x12C11;

    MEM(0xC0000000) = 0x3F1F3F9C;
    MEM(0xC0000004) = 0x080B61DF;
    MEM(0xC0000008) = 0x067F3800;

    MEM(0xC0000010) = (int)VRAM_PHYSICAL_ADDRESS;
    MEM(0xC0000014) = (int)VRAM_PHYSICAL_ADDRESS;

    //MEM(0xC000001C) = 0x1829; // 16 bpp
    MEM(0xC000001C) = 0x182B; // 24 bpp

    //MEM(0x1000000C) = 0x3e005;


    //if( hal_alloc_vaddress((void **)&video_driver_icp.screen, n_pages) )
    //    panic("Can't alloc vaddress for %d videmem pages", n_pages);

    SHOW_FLOW( 7, "vmem va 0x%X", video_driver_icp.screen);


    SHOW_INFO( 0, "Integrator/CP video %d*%d found",  video_driver_icp.xsize, video_driver_icp.ysize );
    return 1;
}






static void icp_map_video(int on_off)
{
    assert( video_driver_icp.screen != 0 );
/*
    hal_pages_control_etc(
                          VBE_DISPI_LFB_PHYSICAL_ADDRESS,
                          video_driver_icp.screen,
                          n_pages, on_off ? page_map : page_unmap, page_rw,
                          INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );
*/
}

static int icp_video_start()
{
    switch_screen_bitblt_to_32bpp();
    icp_map_video( 1 );
    return 0;
}

static int icp_video_stop()
{
    icp_map_video(0);
    return 0;
}




#endif // ARCH_arm






