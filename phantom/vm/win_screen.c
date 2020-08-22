/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * This source file implements Windows based wrapper for VM to
 * run in Windows-hosted environment.
 *
 * TODO: GBR -> RGB!!
 *
**/

#ifndef __linux__

#include <phantom_libc.h>

#include <string.h>
#include <assert.h>

#include <video/screen.h>
#include <video/internal.h>

#include "gcc_replacements.h"

#include "winhal.h"



struct drv_video_screen_t        drv_video_win32 =
{
    "Windows",
    // size
    0, 0, 24,
    // mouse x y flags
    0, 0, 0,

    // screen
    0,

.probe  = (void *)vid_null,
.start  = (void *)vid_null,
.accel  = (void *)vid_null,
.stop   = (void *)vid_null,

update:(void*)vid_null,
bitblt:(void*)vid_null,
//(void*)vid_null, // was winblt

.readblt = (void*)vid_null,

.mouse               =  (void*)vid_null,

.mouse_redraw_cursor =  vid_mouse_draw_deflt,
.mouse_set_cursor    =  vid_mouse_set_cursor_deflt,
.mouse_disable       =  vid_mouse_off_deflt,
.mouse_enable        =  vid_mouse_on_deflt,

};



static int init_ok = 0;
static int init_err = 0;


void    pvm_win_window_thread()
{
    if(win_scr_setup_window())
    {
        init_err = 1;
        printf("pvm_win_setup_window failed\n");
        return;
    }

    win_scr_init_window();

    int i;
    for( i = 0; i < VSCREEN_WIDTH * VSCREEN_HEIGHT * 3; i++)
    {
        win_src_screen_image[i] = 34;
    }

    drv_video_win32.screen = win_src_screen_image;
    drv_video_win32.xsize = VSCREEN_WIDTH;
    drv_video_win32.ysize = VSCREEN_HEIGHT;
    drv_video_win32.update = &win_scr_screen_update;
#if 1
    drv_video_win32.bitblt = &vid_bitblt_forw;
#if VIDEO_DRV_WINBLT
    drv_video_win32.winblt = &vid_win_winblt;
#endif
    drv_video_win32.readblt = &vid_readblt_forw;
    drv_video_win32.bitblt_part = &vid_bitblt_part_forw;
#else
    drv_video_win32.bitblt = &drv_video_bitblt_rev;
    drv_video_win32.winblt = &drv_video_win_winblt_rev;
    drv_video_win32.readblt = &drv_video_readblt_rev;
    drv_video_win32.bitblt_part = &drv_video_bitblt_part_rev;
#endif

    drv_video_win32.mouse_redraw_cursor = &vid_mouse_draw_deflt;
    drv_video_win32.mouse_set_cursor = &vid_mouse_set_cursor_deflt;
    drv_video_win32.mouse_disable = &vid_mouse_off_deflt;
    drv_video_win32.mouse_enable = &vid_mouse_on_deflt;

    init_ok = 1;

#if HOVER
    {
        eventTrack.cbSize = sizeof(eventTrack);
        eventTrack.dwFlags = TME_HOVER;
        eventTrack.hwndTrack = hWnd;
        eventTrack.dwHoverTime = 5;

        if(0 == TrackMouseEvent(&eventTrack))
            printf("Track error\n");
    }
#endif

    win_scr_event_loop();
}


void win_scr_mk_mouse_event(int wParam, int xPos, int yPos )
{
    drv_video_win32.mouse_x = xPos;
    drv_video_win32.mouse_y = yPos;
    drv_video_win32.mouse_flags = wParam;
    drv_video_win32.mouse();

    struct ui_event e;
    e.type = UI_EVENT_TYPE_MOUSE;
    e.time = fast_time();
    e.focus= 0;

    e.m.buttons = wParam;
    e.abs_x = xPos;
    e.abs_y = yPos; //VSCREEN_HEIGHT - yPos - 1;

    ev_q_put_any( &e );
    //printf("-ms-");            printf("%d,%d\n", xPos, yPos );
}


int pvm_video_init()
{
    video_drv = &drv_video_win32;

    drv_video_win32.screen = 0; // Not ready yet

    printf("Starting windows graphics 'driver'\n" );


    if( win_src_make_thread((void *)&pvm_win_window_thread) )
        panic("can't start window thread");


    int repeat = 10000;
    while(repeat-- > 0)
    {
        hal_sleep_msec( 20 );
        //hal_sleep_msec( 100 );
        if( init_err ) break;
        if( init_ok )
        {
            scr_zbuf_init();
            scr_zbuf_turn_upside(1);
            return 0;
        }

    }


    return -1;
}



#endif // __linux__


