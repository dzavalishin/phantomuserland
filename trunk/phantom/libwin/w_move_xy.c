/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - move.
 *
**/

#define DEBUG_MSG_PREFIX "win"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/debug.h>

#include <phantom_libc.h>

#include <video/window.h>
#include <video/internal.h>
#include <video/zbuf.h>


// faster, but has artefacts at top when move down
#if 0

void
drv_video_window_move( drv_video_window_t *w, int x, int y )
{
    ui_event_t e1, e2;
    rect_t oldw;
    rect_t neww;

    //w->state &= ~WSTATE_WIN_UNCOVERED; // mark as possibly covered

    e1.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
    e2.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;

    w_lock();

    oldw.x = w->x;
    oldw.y = w->y;
    oldw.xsize = w->xsize;
    oldw.ysize = w->ysize;

    // Suppose that decor overlaps us. If not - use add.
    if( w->w_decor )
    {
        oldw.x = w->w_decor->x;
        oldw.y = w->w_decor->y;
        oldw.xsize = w->w_decor->xsize;
        oldw.ysize = w->w_decor->ysize;
        scr_zbuf_reset_square( w->w_decor->x, w->w_decor->y, w->w_decor->xsize, w->w_decor->ysize );
    }
    else
        scr_zbuf_reset_square( w->x, w->y, w->xsize, w->ysize );

    w->x = x;
    w->y = y;
    win_move_decorations(w);

    neww.x = w->x;
    neww.y = w->y;
    neww.xsize = w->xsize;
    neww.ysize = w->ysize;

    // Suppose that decor overlaps us. If not - use add.
    if( w->w_decor )
    {
        neww.x = w->w_decor->x;
        neww.y = w->w_decor->y;
        neww.xsize = w->w_decor->xsize;
        neww.ysize = w->w_decor->ysize;
    }

    int o2 = rect_sub( &e1.w.rect, &e2.w.rect, &oldw, &neww );

    scr_zbuf_reset_square( e1.w.rect.x, e1.w.rect.y, e1.w.rect.xsize, e1.w.rect.ysize );
    event_q_put_global( &e1 );

    //if( o2 )
    {
        scr_zbuf_reset_square( e2.w.rect.x, e2.w.rect.y, e2.w.rect.xsize, e2.w.rect.ysize );
        event_q_put_global( &e2 );
    }

    event_q_put_win( 0, 0, UI_EVENT_WIN_REDECORATE, w );

    w_unlock();
}

#else


void
w_move( drv_video_window_t *w, int x, int y )
{
    ui_event_t e1, e2, e3;

    lprintf( "w move @ %d/%d, sz %d x %d, to %d/%d\n", w->x, w->y, w->xsize, w->ysize, x, y );

    w_lock();

    //w->state &= ~WSTATE_WIN_UNCOVERED; // mark as possibly covered

    e1.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
    e2.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
    e3.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;

    e1.w.rect.x = w->x;
    e1.w.rect.y = w->y;
    e1.w.rect.xsize = w->xsize;
    e1.w.rect.ysize = w->ysize;

#if !VIDEO_T_IN_D
    if( w->w_title )
    {
        e2.w.rect.x = w->w_title->x;
        e2.w.rect.y = w->w_title->y;
        e2.w.rect.xsize = w->w_title->xsize;
        e2.w.rect.ysize = w->w_title->ysize;
    }
#endif

    if( w->w_decor )
    {
        e3.w.rect.x = w->w_decor->x;
        e3.w.rect.y = w->w_decor->y;
        e3.w.rect.xsize = w->w_decor->xsize;
        e3.w.rect.ysize = w->w_decor->ysize;
    }


    //int ox = w->x;
    //int oy = w->x;

#if !VIDEO_T_IN_D
    if( w->w_title )
        scr_zbuf_reset_square( w->w_title->x, w->w_title->y, w->w_title->xsize, w->w_title->ysize );
#endif
    if( !(w->state & WSTATE_WIN_ROLLEDUP) )
    {
        scr_zbuf_reset_square( w->x, w->y, w->xsize, w->ysize );

        if( w->w_decor )
            scr_zbuf_reset_square( w->w_decor->x, w->w_decor->y, w->w_decor->xsize, w->w_decor->ysize );
    }

    w->x = x;
    w->y = y;
    win_move_decorations(w);


#if 1



#if !VIDEO_T_IN_D
    if( w->w_title )
        ev_q_put_global( &e2 );
#endif

    if( !(w->state & WSTATE_WIN_ROLLEDUP) )
    {
        ev_q_put_global( &e1 );

        if( w->w_decor )
            ev_q_put_global( &e3 );
    }

#else
    scr_repaint_all();
#endif

    //event_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, w );
    ev_q_put_win( 0, 0, UI_EVENT_WIN_REDECORATE, w );

    w_unlock();
}
#endif
