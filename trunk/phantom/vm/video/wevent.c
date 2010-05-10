/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system helpers.
 *
 *
**/


#include <video.h>
#include <assert.h>
#include <phantom_libc.h>
#include <event.h>
#include <spinlock.h>




static void defaultMouseEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
}

static void defaultKeyEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
}

static void defaultWinEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
    switch(e->w.info)
    {
    case UI_EVENT_WIN_GOT_FOCUS:
        w->state |= WSTATE_WIN_FOCUSED;
        goto redecorate;

    case UI_EVENT_WIN_LOST_FOCUS:
        w->state &= ~WSTATE_WIN_FOCUSED;
        goto redecorate;

    case UI_EVENT_WIN_DESTROYED:
        break;

    case UI_EVENT_WIN_REPAINT:
        drv_video_winblt( w );
        break;

    case UI_EVENT_WIN_REDECORATE:
    redecorate:
        if(w->flags & WFLAG_WIN_DECORATED)
        {
            win_make_decorations(w);
            drv_video_winblt( w );
        }
        break;


    }
}

void defaultWindowEventProcessor( drv_video_window_t *w )
{
    struct ui_event e;
    int got = drv_video_window_get_event( w, &e, 0 );

    if(!got)
        return;

    switch(e.type)
    {
    case UI_EVENT_TYPE_MOUSE: 	defaultMouseEventProcessor(w, &e); break;
    case UI_EVENT_TYPE_KEY:     defaultKeyEventProcessor(w, &e); break;
    case UI_EVENT_TYPE_WIN:     defaultWinEventProcessor(w, &e); break;
    }
}


