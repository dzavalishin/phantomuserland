/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * This source file implements Windows based wrapper for VM to
 * run with VNC as a main screen.
 *
 *
**/

#include "vnc_server.h"

#include <phantom_libc.h>

#include <string.h>
#include <assert.h>

#include <queue.h>

#include <video/screen.h>
#include <video/internal.h>


int VSCREEN_WIDTH = CONFIG_VNCSERVER_SCREENWIDTH;
int VSCREEN_HEIGHT = CONFIG_VNCSERVER_SCREENHEIGHT;

static char vnc_screen_image[ CONFIG_VNCSERVER_SCREENHEIGHT * CONFIG_VNCSERVER_SCREENWIDTH * 4 ];

struct drv_video_screen_t        drv_video_vnc =
{
    "VNC",
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


static void vnc_update_all( void )
{
    rect_t scr_r;
    scr_get_rect( &scr_r );

    vnc_update_rectangle(g_vnc_sessions[0], &scr_r, true );
}

//static int init_ok = 0;
//static int init_err = 0;

static hal_mutex_t vnc_lock;


void vnc_mutex_lock( void ) { hal_mutex_lock( &vnc_lock ); }
void vnc_mutex_unlock( void ) { hal_mutex_unlock( &vnc_lock ); }

void pvm_vnc_init_funcs( void )
{
    hal_mutex_init( &vnc_lock, "VNC" );
    int i;
    for( i = 0; i < VSCREEN_WIDTH * VSCREEN_HEIGHT * 3; i++)
    {
        vnc_screen_image[i] = 34;
    }

    drv_video_vnc.screen = vnc_screen_image;
    drv_video_vnc.xsize = VSCREEN_WIDTH;
    drv_video_vnc.ysize = VSCREEN_HEIGHT;
    // TODO need it?    
    drv_video_vnc.update = &vnc_update_all; // TODO remove me, do more detailed update
#if 1
    drv_video_vnc.bitblt = &vid_bitblt_forw;
#if VIDEO_DRV_WINBLT
    drv_video_vnc.winblt = &vid_win_winblt;
#endif
    drv_video_vnc.readblt = &vid_readblt_forw;
    drv_video_vnc.bitblt_part = &vid_bitblt_part_forw;
#else
    drv_video_vnc.bitblt = &drv_video_bitblt_rev;
    drv_video_vnc.winblt = &drv_video_win_winblt_rev;
    drv_video_vnc.readblt = &drv_video_readblt_rev;
    drv_video_vnc.bitblt_part = &drv_video_bitblt_part_rev;
#endif

    drv_video_vnc.mouse_redraw_cursor = &vid_mouse_draw_deflt;
    drv_video_vnc.mouse_set_cursor = &vid_mouse_set_cursor_deflt;
    drv_video_vnc.mouse_disable = &vid_mouse_off_deflt;
    drv_video_vnc.mouse_enable = &vid_mouse_on_deflt;

    //init_ok = 1;
    switch_screen_bitblt_to_32bpp( 1 );


    //win_scr_event_loop();
}

/*
void win_scr_mk_mouse_event(int wParam, int xPos, int yPos )
{
    drv_video_vnc.mouse_x = xPos;
    drv_video_vnc.mouse_y = yPos;
    drv_video_vnc.mouse_flags = wParam;
    drv_video_vnc.mouse();

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
*/

int vnc_pvm_video_init()
{
    video_drv = &drv_video_vnc;

    drv_video_vnc.screen = 0; // Not ready yet

    printf("Starting VNC graphics driver\n" );

    pvm_vnc_init_funcs();
    //if( win_src_make_thread((void *)&pvm_win_window_thread) )        panic("can't start window thread");

    /*
    int repeat = 10000;
    while(repeat-- > 0)
    {
        hal_sleep_msec( 20 );
        //hal_sleep_msec( 100 );
        if( init_err ) break;
        if( init_ok )
        {
            scr_zbuf_init();
            //scr_zbuf_turn_upside(1);
            return 0;
        }

    }*/

//int vnc_server( void );

    hal_start_thread((void *)vnc_server, 0, 0);

    return 0;
}




