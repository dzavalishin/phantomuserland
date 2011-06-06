/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Video drier search and probe code. Practically turned off
 * if VESA is found. That's wrong.
 *
**/

#define DEBUG_MSG_PREFIX "video"
#include "debug_ext.h"
static int debug_level_flow = 1;

#include <phantom_libc.h>

#include "video.h"
#include "hal.h"

#include "misc.h"

#ifdef ARCH_arm
extern struct drv_video_screen_t        video_driver_icp;
#endif // ARCH_arm


// TODO: panic must switch to text mode!

// Placeholder for absent drv methods
//void drv_video_null() {}


#define JUST_VGA 0

struct drv_video_screen_t *video_drivers[] =
{
#ifdef ARCH_ia32
    &video_driver_basic_vga,
#if !JUST_VGA

    // Incomplete, and suspected to break VESA driver
    //&video_driver_cirrus,

    // &video_driver_bochs_vesa_emulator,

    // test one. never reports success
    &video_driver_direct_vesa,

    // General reg clone driver
    &video_driver_gen_clone,
#if !VESA_ENFORCE
    &video_driver_bios_vesa,
#endif
#endif

#endif //ia32

#ifdef ARCH_arm
	&video_driver_icp,
#endif // ARCH_arm

};


struct drv_video_screen_t        *video_drv = 0;

static void phantom_select_video_driver()
{
    long selected_sq = 0;
    struct drv_video_screen_t *selected_drv = NULL;

    SHOW_FLOW0( 2, "Look for video driver" );

    unsigned int i;
    for( i = 0; i < (sizeof(video_drivers)/sizeof(struct drv_video_screen_t *)); i++ )
    {
        struct drv_video_screen_t *drv = video_drivers[i];

        SHOW_FLOW( 2, "Probing %s video driver: ", drv->name);
        if( !drv->probe() )
        {
            SHOW_FLOW( 2, "Video driver %s : No", drv->name);
            continue;
        }
        SHOW_FLOW( 2, "Video driver %s : Yes", drv->name);

        long sq = drv->xsize * drv->ysize;

        if( sq > selected_sq )
        {
            selected_drv = drv;
            selected_sq = sq;
        }
    }

    if( selected_drv == NULL )
    {
        printf("No video driver found!");
    }
    else
    {
        if(video_drv != NULL)
            video_drv->stop();

        SHOW_FLOW( 1, "The best is %s video driver\n", selected_drv->name);
        video_drv = selected_drv;
    }
}

static int was_enforced = 0;


static void video_post_start()
{

    video_zbuf_init();
    //drv_video_init_all_windows_queue(); // static init is ok


    //hal_sleep_msec(10000);
    SHOW_FLOW0( 3, "Video console init" );

    phantom_init_console_window();
    //hal_sleep_msec(10000);
    SHOW_FLOW0( 3, "Video mouse cursor init" );
    drv_video_set_mouse_cursor(drv_video_get_default_mouse_bmp());
}


void phantom_start_video_driver(void)
{
    if( was_enforced && video_drv != 0 )
    {
        SHOW_FLOW0( 1, "Skipping video drv select due to enforce" );
        return;
    }

    phantom_select_video_driver();

    //hal_sleep_msec(10000);
    SHOW_FLOW0( 2, "Video start" );

    int res = 1;
    if(video_drv) res = video_drv->start();
    // TODO if start fails, mark driver as not working and select again

    if( res )
        panic("I don't know how to work with this video hardware, sorry");

    //hal_sleep_msec(10000);
    SHOW_FLOW0( 2, "Video post-start" );
    video_post_start();
}

void phantom_stop_video_driver()
{
    phantom_stop_console_window();
    if(video_drv) video_drv->stop();
}

// TODO start/stop - we can stop unstarted drivers - fix
void phantom_enforce_video_driver(struct drv_video_screen_t *vd)
{
    if(video_drv)
        video_drv->stop();

    video_drv =  vd;

    int res;
    if(video_drv) res = video_drv->start();

    if( res )
        panic("I don't know how to work with this video hardware, sorry");

    was_enforced = 1;
    video_post_start();
}

