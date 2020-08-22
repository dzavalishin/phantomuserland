/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - window order.
 *
 *
**/
#define DEBUG_MSG_PREFIX "win"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <assert.h>
#include <phantom_libc.h>
//#include <event.h>
//#include <spinlock.h>

//#include <hal.h>
//#include <kernel/init.h>
//#include "win_local.h"
#include <video/window.h>
#include <video/zbuf.h>
#include <video/internal.h>




/*!
 *
 * Reset Z order for all the windows.
 *
 */
void drv_video_window_rezorder_all(void)
{
    drv_video_window_t *w;

    w_assert_lock();

    int next_z = 0;
    queue_iterate(&allwindows, w, drv_video_window_t *, chain)
    {
        // Actually start with 2, leave 0 for background and 1 for decorations of bottom win
        next_z += 2;

        w->state &= ~WSTATE_WIN_UNCOVERED; // mark all windows as possibly covered

        if( w->w_owner != 0 )
            continue;

        if( w->z == next_z+1 )
            continue;

        w->z = next_z+1;

#if !VIDEO_T_IN_D
        if( 0 != w->w_title )
        {
            w->w_title->z = next_z;
            scr_zbuf_reset_win( w->w_title );
        }
#endif

        if( 0 != w->w_decor )
        {
            w->w_decor->z = next_z;
            scr_zbuf_reset_win( w->w_decor );
        }

        scr_zbuf_reset_win( w ); // TODO rezet z buf in event too?
    }

    int top_one = 1;
    // From top to bottom
    queue_iterate_back(&allwindows, w, drv_video_window_t *, chain)
    {
        // Don't touch children (decor & title)
        if( w->w_owner == 0 )
        {
            if(top_one)
            {
                top_one = 0;
                w->state |= WSTATE_WIN_UNCOVERED; // mark top one as uncovered
                if(w->w_decor)
                    w->w_decor->state |= WSTATE_WIN_UNCOVERED; // and its decor subwin too
            }

            ev_q_put_win( 0, 0, UI_EVENT_WIN_REDECORATE, w );
        }
    }
}





window_handle_t drv_video_next_window(window_handle_t curr)
{
    int reasonable_tries = 1000;
    window_handle_t next = curr;

    w_lock();

    if(next == 0)
        next = (window_handle_t)queue_first(&allwindows);

    do {

        LOG_FLOW( 3, "next = %x", (int)next );

        if(reasonable_tries-- <= 0)
        {
            LOG_ERROR0( 0, "loop?");
            w_unlock();
            return curr;
        }

        next = (window_handle_t)queue_next(&next->chain);
        if(queue_end(&allwindows, (void *)next))
            next = (window_handle_t)queue_first(&allwindows);

        // I'm decoration, don't choose me
        if( next->w_owner )
        {
            LOG_FLOW( 4, "next = %x has owner", (int)next );
            continue;
        }

        // Don't choose unvisible ones
        if( !(next->state & WSTATE_WIN_VISIBLE) )
        {
            LOG_FLOW( 4, "next = %x not visible", (int)next );
            continue;
        }

        // No focus means no focus
        if( next->flags & (WFLAG_WIN_NOFOCUS|WFLAG_WIN_NOKEYFOCUS) )
        {
            LOG_FLOW( 4, "next = %x nofocus", (int)next );
            continue;
        }

        if( next->flags & WFLAG_WIN_NOTINALL )
        {
            LOG_ERROR0( 0, "WFLAG_WIN_NOTINALL in allw");
            continue;
        }

        w_unlock();
        return next;
    } while(next != curr);

    w_unlock();
    // I'm alone here
    return curr;
}


