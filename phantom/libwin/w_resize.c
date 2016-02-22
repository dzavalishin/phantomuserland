/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - resize. Unimpl.
 *
**/

#define DEBUG_MSG_PREFIX "win"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>

#include <video/window.h>
#include <video/internal.h>
#include <video/zbuf.h>


static void
do_window_resize( drv_video_window_t *w, int xsize, int ysize )
{
    panic("not impl");
}




void
w_resize( drv_video_window_t *w, int xsize, int ysize )
{
    rect_t oldsize;
    rect_t newsize;
    rect_t maxsize;

    //w->state &= ~WSTATE_WIN_UNCOVERED; // mark as possibly covered

    w_get_bounds( w, &oldsize );
    do_window_resize(w, xsize, ysize);
    w_get_bounds( w, &newsize );

    rect_add( &maxsize, &oldsize, &newsize );

    scr_zbuf_reset_square( maxsize.x, maxsize.y, maxsize.xsize, maxsize.ysize );

    {
    ui_event_t e;
    //struct ui_event e;
    e.type = UI_EVENT_TYPE_GLOBAL;
    e.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
    e.w.rect = maxsize;

    ev_q_put_global( &e );
    }
}
