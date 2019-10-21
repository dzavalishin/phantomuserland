/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - top/bottom.
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

#include <kernel/snap_sync.h>



void w_to_bottom(drv_video_window_t *w)
{
    w_lock();

    w->state &= ~WSTATE_WIN_UNCOVERED; // mark as possibly covered

    queue_remove(&allwindows, w, drv_video_window_t *, chain);
    queue_enter_first(&allwindows, w, drv_video_window_t *, chain);

    if( w->w_decor )
    {
        queue_remove(&allwindows, w->w_decor, drv_video_window_t *, chain);
        queue_enter_first(&allwindows, w->w_decor, drv_video_window_t *, chain);
    }

    if( w->w_title )
    {
        queue_remove(&allwindows, w->w_title, drv_video_window_t *, chain);
        queue_enter_first(&allwindows, w->w_title, drv_video_window_t *, chain);
    }

    drv_video_window_rezorder_all();

    w_unlock();

    //scr_repaint_all();
    scr_repaint_win( w );
}



void w_to_top(drv_video_window_t *w)
{
    w_reset_notification( w ); // If some notifications are in task bar, reset 'em

    w_lock();

    w->state &= ~WSTATE_WIN_UNCOVERED; // mark as possibly covered

    if( w->w_decor )
    {
        queue_remove(&allwindows, w->w_decor, drv_video_window_t *, chain);
        //queue_enter_first(&allwindows, w->w_decor, drv_video_window_t *, chain);
    }

    if( w->w_title )
    {
        queue_remove(&allwindows, w->w_title, drv_video_window_t *, chain);
        //queue_enter_first(&allwindows, w->w_title, drv_video_window_t *, chain);
    }


    queue_remove(&allwindows, w, drv_video_window_t *, chain);
#if 0
    queue_enter(&allwindows, w, drv_video_window_t *, chain);
#else
    if( (w->flags & WFLAG_WIN_ONTOP) || (queue_empty(&allwindows)) )
    {
        // Trivial case - our window has 'ontop' flag too and can be topmost, or queue is empty
        if( w->w_decor ) queue_enter(&allwindows, w->w_decor, drv_video_window_t *, chain);
        if( w->w_title ) queue_enter(&allwindows, w->w_title, drv_video_window_t *, chain);
        queue_enter(&allwindows, w, drv_video_window_t *, chain);
    }
    else
    {
        // Our window has no 'ontop' flag and must go under topmost ones and queue is not empty
        drv_video_window_t *iw;
        queue_iterate_back(&allwindows, iw, drv_video_window_t *, chain)
        {
            if( ! (iw->flags & WFLAG_WIN_ONTOP) )
            {
                // Found window with no WFLAG_WIN_ONTOP flag - come on top of it
                queue_enter_after(&allwindows, iw, w, drv_video_window_t *, chain);
                if( w->w_decor ) queue_enter_after(&allwindows, iw, w->w_decor, drv_video_window_t *, chain);
                if( w->w_title ) queue_enter_after(&allwindows, iw, w->w_title, drv_video_window_t *, chain);
                goto inserted;
            }
        }

        // must go to most bottom pos?? near to unreal...
        LOG_ERROR0( 0, "insert at bottom");
        queue_enter_first(&allwindows, w, drv_video_window_t *, chain);
        if( w->w_decor ) queue_enter_first(&allwindows, w->w_decor, drv_video_window_t *, chain);
        if( w->w_title ) queue_enter_first(&allwindows, w->w_title, drv_video_window_t *, chain);
    }
inserted:
#endif
    drv_video_window_rezorder_all();

    w_unlock();

    //scr_repaint_all();
    scr_repaint_win( w );
}

/// TODO: find way to not to use vm_lock_persistent_memory()
/// UNLOCKED: call from lock only
/// Is one of topmost - i.e. covered only by WFLAG_WIN_ONTOP ones
int iw_is_top(drv_video_window_t *w)
{
    w_assert_lock();

    vm_lock_persistent_memory();

    drv_video_window_t *iw;
    queue_iterate_back(&allwindows, iw, drv_video_window_t *, chain)
    {
        if( iw == w ) 
        {
            vm_unlock_persistent_memory();
            return 1;
        }

        if( ! (iw->flags & WFLAG_WIN_ONTOP) )
            break;
    }

    vm_unlock_persistent_memory();
    return 0;
}


/// Is one of topmost - i.e. covered only by WFLAG_WIN_ONTOP ones
int w_is_top(drv_video_window_t *w)
{
    w_lock();
    int ret = iw_is_top(w);
    w_unlock();
    return ret;
}