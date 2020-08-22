/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - background window.
 *
**/
#define DEBUG_MSG_PREFIX "win.bg"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <video/screen.h>
#include <video/window.h>
#include <video/internal.h>
#include <stdio.h>

#if VIDEO_NEW_BG_WIN

static window_handle_t back_win = 0;


void w_bg_win_init(void)
{
    if(back_win)
    {
        LOG_ERROR0( 0, "BG win inited twice" );
        return;
    }

    //back_win = drv_video_window_create( scr_get_xsize(), scr_get_ysize(), 0, 0, COLOR_BLACK, "Background", WFLAG_WIN_DECORATED );
    back_win = drv_video_window_create( scr_get_xsize(), scr_get_ysize(), 0, 0, COLOR_BLACK, "Background", WFLAG_WIN_NOFOCUS );

    //back_win->flags &= ~WFLAG_WIN_DECORATED;
    //back_win->flags |= WFLAG_WIN_NOFOCUS;

    w_clear(back_win);
    w_to_bottom(back_win);
}

window_handle_t w_get_bg_window(void)
{
    return back_win;
}

#endif
