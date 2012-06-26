/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Window title in-kernel event processor.
 *
**/

#include <assert.h>
#include <phantom_libc.h>
#include <event.h>
//#include <spinlock.h>

//#include "win_local.h"

#include <video/window.h>
#include <video/internal.h>
//#include <video/vops.h>
//#include <video/font.h>
//#include <video/button.h>


static int titleWinEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
    assert(w->w_owner);
    drv_video_window_t *mainw = w->w_owner;

    //printf("defaultWinEventProcessor e=%p, e.w=%p, w=%p", e, e->focus, w);

    switch(e->w.info)
    {
    case UI_EVENT_WIN_GOT_FOCUS:
        //w->w_owner->state |= WSTATE_WIN_FOCUSED;
        w->state |= WSTATE_WIN_FOCUSED;
        goto redecorate;

    case UI_EVENT_WIN_LOST_FOCUS:
        //w->w_owner->state &= ~WSTATE_WIN_FOCUSED;
        w->state &= ~WSTATE_WIN_FOCUSED;
        goto redecorate;

    case UI_EVENT_WIN_DESTROYED:
        break;

    case UI_EVENT_WIN_REPAINT:
        iw_winblt( w );
        break;

    case UI_EVENT_WIN_REDECORATE:
        break;

    redecorate:
        if(w->w_owner->flags & WFLAG_WIN_DECORATED)
        {
            w_lock();
            win_make_decorations(w->w_owner);
            iw_winblt_locked( w->w_owner );
            w_unlock();
        }
        break;

    case UI_EVENT_WIN_BUTTON: printf("title button %x\n", e->extra );
    {
        switch(e->extra)
        {
        case WBUTTON_SYS_ROLLUP:
            if( mainw->state & WSTATE_WIN_ROLLEDUP )
            {
                printf("roll down\n" );
                mainw->state &= ~WSTATE_WIN_ROLLEDUP;
                goto redecorate;
            }
            else
            {
                printf("roll up\n" );
                mainw->state |= WSTATE_WIN_ROLLEDUP;
                if( mainw->w_decor )
                    request_repaint_all_for_win( mainw->w_decor );
                else
                    request_repaint_all_for_win( mainw );
            }
            break;
        }
    }
    break;

    default:
        return 0;

    }

    return 1;
}

static int titleMouseEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
    //printf("titleMouseEventProcessor buttons %x, %d-%d", e->m.buttons, e->abs_x, e->abs_y);

    if( e->m.buttons == 0 )
    {
        w->state &= ~WSTATE_WIN_DRAGGED;
        return 0;
    }
    else
    {
        // Not yet dragging?
        if(! (w->state & WSTATE_WIN_DRAGGED) )
        {
            w_to_top(w->w_owner);

            // Set flag and remember position
            w->state |= WSTATE_WIN_DRAGGED;
            w->dx = w->w_owner->x - e->abs_x;
            w->dy = w->w_owner->y - e->abs_y;
            return 1;
        }

    }

    if(w->state & WSTATE_WIN_DRAGGED)
    {
        int nx = e->abs_x + w->dx;
        int ny = e->abs_y + w->dy;

        w_move( w->w_owner, nx, ny );
        return 1;
    }

    return 0;
}


int w_titleWindowEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
    //printf("!! titleWindowEventProcessor e=%p, e.w=%p, w=%p\r", e, e->focus, w);

    switch(e->type)
    {
    case UI_EVENT_TYPE_MOUSE: 	return titleMouseEventProcessor(w, e);
#if KEY_EVENTS
    //case UI_EVENT_TYPE_KEY:     return defaultKeyEventProcessor(w, e);
#endif
    case UI_EVENT_TYPE_WIN:     return titleWinEventProcessor(w, e);

    default:
        break;
    }

    return defaultWindowEventProcessor(w,e);
}

