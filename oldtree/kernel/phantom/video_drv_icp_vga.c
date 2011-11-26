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
//#include <ia32/phantom_pmap.h>

#include <phantom_libc.h>

//#include <video.h>
#include <video/screen.h>
#include <video/internal.h>

#define VGA_800x600 0
#define VGA_1024x768 1



#define LCDTiming0 		0xC0000000
#define LCDTiming1 		0xC0000004
#define LCDTiming2 		0xC0000008
#define LCDTiming3 		0xC000000C

#define LCDUPBASE               0xC0000010
#define LCDLPBASE               0xC0000014

#define LCDControl              0xC000001C


static physaddr_t       pa;
static void *           va;


//#define VRAM_PHYSICAL_ADDRESS  0x00200000
// Kernel is loaded at 1 Mb, so use mem below it, but 1 page above trap vectors
//#define VRAM_PHYSICAL_ADDRESS  0x00001000
#define VRAM_PHYSICAL_ADDRESS  pa

#define VRAM_SIZE (1280*1024*4)


// TODO move to integrator/cp header
#define CP_IDFIELD 0xCB000000
#define MEM(___a) *((int *)___a)



static int icp_video_probe();
static int icp_video_start();
static int icp_video_stop();



struct drv_video_screen_t        video_driver_icp =
{
    "Integrator/CP",
#if VGA_1024x768
    1024, 820, 32,
#if VGA_800x600
    // size
    800, 600,
#else
    // size
    640, 480,
#endif
#endif
    // mouse x y flags
    0, 0, 0,

    // screen
screen:			0,

probe: 			icp_video_probe,
start: 			icp_video_start,
stop:   		icp_video_stop,

#if 0
update: 		vid_null,
bitblt: 		drv_video_bitblt_rev,
winblt:			drv_video_win_winblt_rev,
readblt: 		drv_video_readblt_rev,
bitblt_part:            drv_video_bitblt_part_rev,

mouse:    		vid_null,
#endif

#if 1
redraw_mouse_cursor: 	(void *)vid_null,
set_mouse_cursor: 	(void *)vid_null,
mouse_disable:          (void *)vid_null,
mouse_enable:          	(void *)vid_null,
#endif
};


#define ICP_ID 0x41034003



//static int n_pages = 1024;
static int icp_video_probe()
{
    if(va == 0)
        hal_pv_alloc( &pa, &va, VRAM_SIZE );

    // TODO switch to virtual on paging start?
    video_driver_icp.screen = va;
    //video_driver_icp.screen = (void *)VRAM_PHYSICAL_ADDRESS;

    int id = MEM(CP_IDFIELD);
    if( id != ICP_ID)
    {
        printf("Integrator/CP board id is %x, not %x\n", id, ICP_ID );
        return VIDEO_PROBE_FAIL;
    }


#define PIX_PER_LINE_MASK 0xFC
#define PIX_PER_LINE_SHIFT 2


#if VGA_1024x768
    // 1024*768 - this won't work on real hw!
    MEM(0x10000014) = 0xA05F;
    //MEM(0x1000001C) = 0x12C11; // 25MHz
    MEM(0x1000001C) = 0xD061; // 36 MHz?

    int tm0 = 0x3F1F3F9C;

    tm0 &= ~PIX_PER_LINE_MASK;
    //tm0 |= (63) << PIX_PER_LINE_SHIFT;
    tm0 |= ((video_driver_icp.xsize/16)-1) << PIX_PER_LINE_SHIFT;

    MEM(LCDTiming0) = tm0;

    MEM(LCDTiming1) = (video_driver_icp.ysize)-1;
    //MEM(LCDTiming1) = 728-1;

    MEM(LCDTiming2) = 0x067F3800;

#else
#if VGA_800x600
    // 800*600 - this won't work on real hw!
    MEM(0x10000014) = 0xA05F;
    //MEM(0x1000001C) = 0x12C11; // 25MHz
    MEM(0x1000001C) = 0xD061; // 36 MHz?

    int tm0 = 0x3F1F3F9C;

    tm0 &= ~PIX_PER_LINE_MASK;
    tm0 |= (49) << PIX_PER_LINE_SHIFT;

    MEM(LCDTiming0) = tm0;

    //MEM(LCDTiming1) = 0x080B61DF;
    MEM(LCDTiming1) = 600-1;

    MEM(LCDTiming2) = 0x067F3800;

#else
    // this sets 640*480
    MEM(0x10000014) = 0xA05F;
    MEM(0x1000001C) = 0x12C11; // 25MHz

    MEM(LCDTiming0) = 0x3F1F3F9C;
    MEM(LCDTiming1) = 0x080B61DF;
    MEM(LCDTiming2) = 0x067F3800;
#endif
#endif

    MEM(LCDUPBASE) = (int)VRAM_PHYSICAL_ADDRESS;
    //MEM(0xC0000014) = (int)VRAM_PHYSICAL_ADDRESS;

    //MEM(0xC000001C) = 0x1829; // 16 bpp
    MEM(LCDControl) = 0x182B; // 24 bpp

    //MEM(0x1000000C) = 0x3e005;


    //if( hal_alloc_vaddress((void **)&video_driver_icp.screen, n_pages) )
    //    panic("Can't alloc vaddress for %d videmem pages", n_pages);

    SHOW_FLOW( 7, "vmem va 0x%X", video_driver_icp.screen);


    SHOW_INFO( 0, "Integrator/CP video %d*%d found",  video_driver_icp.xsize, video_driver_icp.ysize );
    return VIDEO_PROBE_SUCCESS;
}






static void icp_map_video(int on_off)
{
    assert( video_driver_icp.screen != 0 );
/*
    hal_pages_control_etc(
                          VBE_DISPI_LFB_PHYSICAL_ADDRESS,
                          video_driver_icp.screen,
                          n_pages, on_off ? page_map_io : page_unmap, page_rw, 0 );
*/
}



static int icp_video_start()
{
    switch_screen_bitblt_to_32bpp(1);
    icp_map_video( 1 );
    return 0;
}

static int icp_video_stop()
{
    icp_map_video(0);
    return 0;
}




#endif // ARCH_arm






