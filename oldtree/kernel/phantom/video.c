/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * Video drier search and probe code. 
 *
**/

#define DEBUG_MSG_PREFIX "video"
#include <debug_ext.h>
static int debug_level_flow = 1;

#include <phantom_libc.h>

//#include <video.h>
#include <video/screen.h>
#include <video/internal.h>
#include <hal.h>
#include <kernel/init.h>

#include "misc.h"

#ifndef VESA_ENFORCE
#  warning no VESA_ENFORCE def
#endif

#ifdef ARCH_arm
extern struct drv_video_screen_t        video_driver_icp;

#ifdef BOARD_arm_raspberry
extern struct drv_video_screen_t        video_driver_raspberry_pi;
#endif

#endif // ARCH_arm

#ifdef ARCH_mips
extern struct drv_video_screen_t        video_driver_null;
#endif // ARCH_mips



extern struct drv_video_screen_t        video_driver_vmware_svga;
extern struct drv_video_screen_t        video_driver_parallels_svga;

// TODO: panic must switch to text mode!

// Placeholder for absent drv methods
//void vid_null() {}


#define JUST_VGA 0

struct drv_video_screen_t *video_drivers[] =
{
#ifdef ARCH_ia32
    &video_driver_basic_vga,
#if !JUST_VGA

    // Incomplete, and suspected to break VESA driver
    &video_driver_cirrus,

    // &video_driver_bochs_vesa_emulator,

    &video_driver_vmware_svga,

    // test one. never reports success
    &video_driver_direct_vesa,

    // Parallels paravirt video
    &video_driver_parallels_svga,

    // General reg clone driver
//    &video_driver_gen_clone,
#if !VESA_ENFORCE
    &video_driver_bios_vesa,
#endif
#endif

#endif //ia32

#ifdef ARCH_arm

#  ifdef BOARD_arm_icp
    &video_driver_icp,
#  endif

#  ifdef BOARD_arm_raspberry
    &video_driver_raspberry_pi,
#  endif

#endif // ARCH_arm


#ifdef ARCH_mips
    &video_driver_null,
#endif // ARCH_mips

};


struct drv_video_screen_t        *video_drv = 0;


static void set_video_defaults(void);

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
        if( drv->probe() != VIDEO_PROBE_SUCCESS )
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
        SHOW_FLOW0( 1, "No video driver found!");
    }
    else
    {
        if(video_drv != NULL)
            video_drv->stop();

        SHOW_FLOW( 1, "The best is %s video driver", selected_drv->name);
        video_drv = selected_drv;
        set_video_defaults();
    }

}

static void select_accel_driver(void)
{
    struct drv_video_screen_t *selected_drv = NULL;

    SHOW_FLOW0( 2, "Look for accelerating video driver" );

    unsigned int i;
    for( i = 0; i < (sizeof(video_drivers)/sizeof(struct drv_video_screen_t *)); i++ )
    {
        struct drv_video_screen_t *drv = video_drivers[i];

        if( (drv->accel == 0) || (drv->accel == (void *)vid_null) )
        {
            SHOW_FLOW( 2, "Video driver %s : Not accelerated", drv->name);
            continue;
        }

        SHOW_FLOW( 2, "Probing %s video driver: ", drv->name);
        if( !drv->probe() )
        {
            SHOW_FLOW( 2, "Video driver %s : No", drv->name);
            continue;
        }
        SHOW_FLOW( 2, "Video driver %s : Yes", drv->name);

        selected_drv = drv;

        break; // First one is ok
    }

    if( selected_drv == NULL )
    {
        SHOW_FLOW0( 1, "No video driver found!");
    }
    else
    {
        SHOW_FLOW( 1, "Adding acceleration from %s video driver", selected_drv->name);
        selected_drv->accel();
    }

}



static int was_enforced = 0;


static void video_post_start()
{

    scr_zbuf_init();
    drv_video_init_windows();

    // Have VESA driver, add companion accelerator if possible
    if( was_enforced )
        select_accel_driver();

    SHOW_FLOW0( 3, "Video console init" );
    phantom_init_console_window();

    SHOW_FLOW0( 3, "Video mouse cursor init" );
    scr_mouse_set_cursor(drv_video_get_default_mouse_bmp());
}


void phantom_start_video_driver(void)
{
#if VESA_ENFORCE
    if( was_enforced && video_drv != 0 )
    {
        SHOW_FLOW0( 1, "Skipping video drv select due to enforce" );
        return;
    }
#endif

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
    set_video_defaults();

    int res = 1; // by default - fail?
    if(video_drv) res = video_drv->start();

    if( res )
        panic("I don't know how to work with this video hardware, sorry");

    was_enforced = 1;
    video_post_start();
}


static void set_video_defaults(void)
{
    // fill in defaults - r/w

    if( 0 == video_drv->update)                     video_drv->update      = (void *)vid_null;
    if( 0 == video_drv->bitblt)                     video_drv->bitblt      = vid_bitblt_rev;
#if VIDEO_DRV_WINBLT
    if( 0 == video_drv->winblt)                     video_drv->winblt      = vid_win_winblt_rev;
#endif
    if( 0 == video_drv->readblt)                    video_drv->readblt     = vid_readblt_rev;
    if( 0 == video_drv->bitblt_part)                video_drv->bitblt_part = vid_bitblt_part_rev;

    // fill in defaults - mouse cursor

    if( 0 == video_drv->mouse_redraw_cursor)        video_drv->mouse_redraw_cursor = vid_mouse_draw_deflt;
    if( 0 == video_drv->mouse_set_cursor)           video_drv->mouse_set_cursor    = vid_mouse_set_cursor_deflt;
    if( 0 == video_drv->mouse_disable )             video_drv->mouse_disable       = vid_mouse_off_deflt;
    if( 0 == video_drv->mouse_enable )              video_drv->mouse_enable        = vid_mouse_on_deflt;

    if( 0 == video_drv->mouse)                      video_drv->mouse = (void*)vid_null;
}



