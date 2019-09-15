/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: no, this is not a kernel code.
 *
 * This source file implements heasdless (no graphics) wrapper for VM to run in Unox or Windows hosted environment.
 *
 *
**/


#include <string.h>
#include <assert.h>

#include <video/screen.h>
#include <video/internal.h>

#include "gcc_replacements.h"


#define VSCREEN_WIDTH 1024
#define VSCREEN_HEIGHT 768

// For any case
static char screen_image [VSCREEN_WIDTH*VSCREEN_HEIGHT*4];


struct drv_video_screen_t        drv_video_headless =
{
    "Headless",
    // size
    VSCREEN_WIDTH, VSCREEN_HEIGHT, 32,
    // mouse x y flags
    0, 0, 0,

    // screen
    screen_image,

probe: (void *)vid_null,
start: (void *)vid_null,
stop:  (void *)vid_null,

    vid_null,
    (void*)vid_null,
    (void*)vid_null,

    vid_null,

mouse:    		vid_null,

mouse_redraw_cursor: 	vid_mouse_draw_deflt,
mouse_set_cursor: 	vid_mouse_set_cursor_deflt,
mouse_disable:          vid_mouse_off_deflt,
mouse_enable:          	vid_mouse_on_deflt,

};




/*
static int eline = 0;


static int init_ok = 0;
static int init_err = 0;





//static
void drv_x11_screen_update(void)
{
    win_x11_update();
    eline = 1;
}



void    win_x11_set_screen( void *vmem )
{
    drv_video_x11.screen = vmem;
}




void    pvm_x11_window_thread()
{

    if(win_x11_init(VSCREEN_WIDTH,VSCREEN_HEIGHT))
    {
        init_err = 1;
        return; // BUG: report error
    }




    //drv_video_x11.screen = screen_image;
    drv_video_x11.xsize = VSCREEN_WIDTH;
    drv_video_x11.ysize = VSCREEN_HEIGHT;
    drv_video_x11.update = &drv_x11_screen_update;

#if 0
    drv_video_x11.bitblt = &vid_bitblt_forw;
#if VIDEO_DRV_WINBLT
    drv_video_x11.winblt = &vid_win_winblt;
#endif
    drv_video_x11.readblt = &vid_readblt_forw;
    drv_video_x11.bitblt_part = &vid_bitblt_part_forw;
#else
    drv_video_x11.bitblt = &vid_bitblt_rev;
#if VIDEO_DRV_WINBLT
    drv_video_x11.winblt = &vid_win_winblt_rev;
#endif
    drv_video_x11.readblt = &vid_readblt_rev;
    drv_video_x11.bitblt_part = &vid_bitblt_part_rev;
#endif

    drv_video_x11.mouse_redraw_cursor = &vid_mouse_draw_deflt;
    drv_video_x11.mouse_set_cursor = &vid_mouse_set_cursor_deflt;
    drv_video_x11.mouse_disable = &vid_mouse_off_deflt;
    drv_video_x11.mouse_enable = &vid_mouse_on_deflt;

    init_ok = 1;


    win_x11_message_loop();
    //printf("Message loop end\n");

}
*/

int pvm_video_init()
{
    video_drv = &drv_video_headless;

    drv_video_headless.screen = screen_image;

    printf("Starting headless graphics 'driver'\n" );

    scr_zbuf_init();
    scr_zbuf_turn_upside(1);
    switch_screen_bitblt_to_32bpp( 1 );
    return 0;
}


/*

void win_x11_key_event( int x, int y, unsigned int state, unsigned int keycode, int isRelease )
{
    printf("-ky- %x %x\r", state, keycode );
}

void win_x11_mouse_event( int x, int y, unsigned int state )
{
    drv_video_x11.mouse_x = x;
    drv_video_x11.mouse_y = y;
    drv_video_x11.mouse_flags = state;
    drv_video_x11.mouse();
#if 1
    struct ui_event e;
    e.type = UI_EVENT_TYPE_MOUSE;
    e.time = fast_time();
    e.focus= 0;

    e.m.buttons = state;
    e.abs_x = x;
    e.abs_y = VSCREEN_HEIGHT - y - 1;

    ev_q_put_any( &e );
    printf("-ms- %x\r", state );
#endif
}


*/

