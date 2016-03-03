/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests - video
 *
 *
**/

#define DEBUG_MSG_PREFIX "test"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>
#include <phantom_libc.h>
#include <errno.h>
#include "test.h"

#include <video/window.h>
#include <video/screen.h>
#include <event.h>

#ifndef LOG_FLOW0
#define LOG_FLOW0 SHOW_FLOW0
#endif

int do_test_video(const char *test_parm)
{
    (void) test_parm;

    int i;
    ui_event_t e1;

    memset( &e1, 0, sizeof(e1) );

    LOG_FLOW0( 0, "start video test" );

    //window_handle_t w = 0;

    //color_t console_fg = COLOR_LIGHTGRAY;
    color_t console_bg = COLOR_BLACK;

    int xsize = 100, ysize = 100;
    int cw_x = 100, cw_y = 100;

    window_handle_t wb = drv_video_window_create( xsize+100, ysize+100,
                        cw_x-50, cw_y-50, COLOR_LIGHTRED, "Test BG Window", WFLAG_WIN_DECORATED );


    window_handle_t w = drv_video_window_create( xsize, ysize,
                        cw_x, cw_y, console_bg, "Test FG Window", WFLAG_WIN_DECORATED );

    w_to_top( w );

    w_set_title( wb, "Paint Test"  );

    e1.type = UI_EVENT_TYPE_GLOBAL;
    e1.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;

    e1.w.rect.x = 0;
    e1.w.rect.y = 0;

    e1.w.rect.xsize = scr_get_xsize();
    e1.w.rect.ysize = scr_get_ysize();

    for( i = 0; i < 300; i++ )
    {
#if USE_ZBUF_SHADOW
        scr_zbuf_request_reset_square( e1.w.rect.x, e1.w.rect.y, e1.w.rect.xsize, e1.w.rect.ysize );
#else
        scr_zbuf_reset_square( e1.w.rect.x, e1.w.rect.y, e1.w.rect.xsize, e1.w.rect.ysize );
#endif
        ev_q_put_global( &e1 );
        //hal_sleep_msec( 15 ); // or else painter combines requests
        hal_sleep_msec( 105 ); // or else painter combines requests
        //hal_sleep_msec( 1005 ); // or else painter combines requests
    }

    w_set_title( wb, "Move Test"  );


    for( i = 0; i < 100; i++ )
    {
        w_move( w, 110, 110 );
        w_move( w, 101, 101 );
        w_move( w, 99, 99 );
    }


    drv_video_window_free( wb );
    drv_video_window_free( w );

    LOG_FLOW0( 0, "end video test" );

    return 0;
}


