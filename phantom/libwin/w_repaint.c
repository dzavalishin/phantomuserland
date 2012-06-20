/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - repaint.
 *
 *
**/

#define DEBUG_MSG_PREFIX "win"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/debug.h>

#include <assert.h>
#include <phantom_libc.h>
//#include <event.h>
//#include <spinlock.h>

//#include <hal.h>
//#include <kernel/init.h>
//#include "win_local.h"
#include <video/window.h>
#include <video/screen.h>
#include <video/internal.h>


#if !SCREEN_UPDATE_THREAD
#warning no repaint thread
void scr_repaint_all(void)
{
    // redraw all here, or ask some thread to do that
    drv_video_window_t *w;

    w_lock();

    queue_iterate(&allwindows, w, drv_video_window_t *, chain)
    {
        _drv_video_winblt_locked( w );
        if(w->flags & WFLAG_WIN_DECORATED)
            win_make_decorations(w);
    }
    w_unlock();
}
#endif



// todo in screen coordinates, wtodo in win
static void repaint_win_part( drv_video_window_t *w, rect_t *wtodo, rect_t *todo  )
{

    if( (w->state & WSTATE_WIN_ROLLEDUP) || (!(w->state & WSTATE_WIN_VISIBLE)) )
        return;
/*
    lprintf( "repaint_win_part w @ %d/%d, sz %d*%d, part @ %d/%d, sz %d*%d\n",
        w->x, w->y, w->xsize, w->ysize, 
        wtodo->x, wtodo->y, wtodo->xsize, wtodo->ysize
    );
*/
    int dst_xpos = wtodo->x + w->x; // todo->x
    int dst_ypos = wtodo->y + w->y; // todo->y

    u_int32_t flags;
    win2blt_flags( &flags, w );

/*
    //assert(video_drv->bitblt_part);
#warning assert(video_drv->bitblt_part)
    if(0 == video_drv->bitblt_part)
        video_drv->bitblt_part = vid_bitblt_part_rev;
*/

    scr_mouse_disable_p(video_drv, todo->x, todo->y, todo->xsize, todo->ysize);

    // TODO wrong - use video driver's func pointer
    video_drv->bitblt_part(
                          w->r_pixel,
                          w->xsize, w->ysize,
                          wtodo->x, wtodo->y,
                          dst_xpos, dst_ypos,
                          wtodo->xsize, wtodo->ysize,
                          w->z, flags);

    scr_mouse_enable_p(video_drv, todo->x, todo->y, todo->xsize, todo->ysize);

}


void w_repaint_screen_part( drv_video_window_t *w, rect_t *todo )
{
    rect_t wtodo = *todo;
    wtodo.x -= w->x;
    wtodo.y -= w->y;

    repaint_win_part( w, &wtodo, todo );
}


#if !VIDEO_NEW_PAINTER
void repaint_all_for_square( rect_t *todo )
{
    // redraw all here, or ask some thread to do that
    drv_video_window_t *w;

    w_lock();
    scr_mouse_disable_p(video_drv, todo->x, todo->y, todo->xsize, todo->ysize );

    //queue_iterate(&allwindows, w, drv_video_window_t *, chain)
    queue_iterate_back(&allwindows, w, drv_video_window_t *, chain)
    {
        rect_t wtodo = *todo;
        wtodo.x -= w->x;
        wtodo.y -= w->y;

        if( !rect_win_bounds( &wtodo, w ) )
        {
            repaint_win_part( w, &wtodo, todo );
            //_drv_video_winblt_locked( w );
            if(w->flags & WFLAG_WIN_DECORATED)
            {
                //win_make_decorations(w);
                win_draw_decorations(w);
            }
        }
    }

    scr_mouse_enable_p(video_drv, todo->x, todo->y, todo->xsize, todo->ysize );
    w_unlock();
}
#endif

void request_repaint_all_for_square( rect_t *todo )
{
    ui_event_t e;

    e.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
    e.w.rect = *todo;

    ev_q_put_global( &e );
}


void request_repaint_all_for_win( drv_video_window_t *w )
{
    scr_zbuf_reset_win( w );

    rect_t wsize;

    w_get_bounds( w, &wsize );
    request_repaint_all_for_square( &wsize );
}


