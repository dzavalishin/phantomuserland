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



/*
#define DEBUG_MSG_PREFIX "wevent"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10
*/

#include <video.h>
#include <assert.h>
#include <phantom_libc.h>
#include <event.h>
#include <spinlock.h>
#include <wtty.h>

#include <threads.h>


#include "win_local.h"


#define KEY_EVENTS 1



static void defaultMouseEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
}

#if KEY_EVENTS
static void defaultKeyEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
    struct phantom_thread * t = w->owner;

    if( t == 0 )
    {
        //SHOW_ERROR0( 1, "Key event for unowned window" );
        //printf( "Key event for unowned window" );
        return;
    }

    //wtty_t *wt = t->ctty;
    wtty_t *wt = get_thread_ctty( t );

	// Skip key release events
	if(e->modifiers & UI_MODIFIER_KEYUP)
		return;

    errno_t err = wtty_putc_nowait(wt, e->k.ch );
    if(err == ENOMEM)
    {
        //SHOW_ERROR0( 1, "Window keyb buffer overflow" );
        printf( "Window keyb buffer overflow" );
    }
    else if(err)
    {
        //SHOW_ERROR( 1, "Window putc error %d", err );
        printf( "Window putc error %d", err );
    }

}
#endif

static void defaultWinEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
	printf("defaultWinEventProcessor e=%p, e.w=%p, w=%p", e, e->focus, w);
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

void defaultWindowEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
	printf("defaultWindowEventProcessor e=%p, e.w=%p, w=%p", e, e->focus, w);

    switch(e->type)
    {
    case UI_EVENT_TYPE_MOUSE: 	defaultMouseEventProcessor(w, e); break;
#if KEY_EVENTS
    case UI_EVENT_TYPE_KEY:     defaultKeyEventProcessor(w, e); break;
#endif
    case UI_EVENT_TYPE_WIN:     defaultWinEventProcessor(w, e); break;
    }
}


