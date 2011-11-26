/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Null video driver. 
 *
 *
**/

#ifdef ARCH_mips

#define DEBUG_MSG_PREFIX "video-null"
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


static physaddr_t       pa;
static void *           va;




static int null_video_probe();
static int null_video_start();
static int null_video_stop();



struct drv_video_screen_t        video_driver_null =
{
    "NullVideo",
    1024, 820, 32,
    // mouse x y flags
    0, 0, 0,

    // screen
screen:			0,

probe: 			null_video_probe,
start: 			null_video_start,
stop:   		null_video_stop,

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


#define VRAM_SIZE (video_driver_null.xsize * video_driver_null.ysize * 4)


static int null_video_probe()
{
    if(va == 0)
        hal_pv_alloc( &pa, &va, VRAM_SIZE );

    video_driver_null.screen = va;
    return VIDEO_PROBE_SUCCESS;
}


static int null_video_start()
{
    switch_screen_bitblt_to_32bpp(1);
    return 0;
}

static int null_video_stop()
{
    return 0;
}




#endif // ARCH_mips






