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



#define DEBUG_MSG_PREFIX "wevent"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <video.h>
#include <assert.h>
#include <phantom_libc.h>
#include <event.h>
#include <spinlock.h>
#include <wtty.h>

#include <threads.h>


#include "win_local.h"


#define KEY_EVENTS 1



static int defaultMouseEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
    //printf("defaultMouseEventProcessor buttons %x, %d-%d\r", e->m.buttons, e->abs_x, e->abs_y);

    if( e->m.buttons )
        drv_video_window_to_top(w->w_owner ? w->w_owner : w);

    return 0;
}

#if KEY_EVENTS
static int defaultKeyEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
    wtty_t *wt;
    if( 0 != t_get_ctty( w->owner, &wt ) )
        return 0;


    // Skip key release events
    if(e->modifiers & UI_MODIFIER_KEYUP)
        return 1;

    char ch = e->k.ch;

    // This char is shift?
    if( ch == 0 )
        return 1;

    if( e->modifiers & (UI_MODIFIER_SHIFT|UI_MODIFIER_CAPSLOCK) )
    {
        // TODO not a best place. implement keymaps in keybd driver
        switch(ch)
        {
        case '1': ch = '!'; break;
        case '2': ch = '@'; break;
        case '3': ch = '#'; break;
        case '4': ch = '$'; break;
        case '5': ch = '%'; break;
        case '6': ch = '^'; break;
        case '7': ch = '&'; break;
        case '8': ch = '*'; break;
        case '9': ch = '('; break;
        case '0': ch = ')'; break;

        case '-': ch = '_'; break;
        case '=': ch = '+'; break;
        case '`': ch = '~'; break;
        case '[': ch = '{'; break;
        case ']': ch = '}'; break;
        case '\\': ch = '|'; break;
        case ';': ch = ':'; break;
        case '\'': ch = '"'; break;
        case ',': ch = '<'; break;
        case '.': ch = '>'; break;
        case '/': ch = '?'; break;
        default:
            ch = toupper(ch);
            break;
        }
    }

    errno_t err = wtty_putc_nowait(wt, ch );
    if(err == ENOMEM)
    {
        SHOW_ERROR0( 1, "Window keyb buffer overflow" );
        //printf( "Window keyb buffer overflow" );
    }
    else if(err)
    {
        SHOW_ERROR( 1, "Window putc error %d", err );
        //printf( "Window putc error %d", err );
    }
    return 1;
}
#endif

static int defaultWinEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
	//printf("defaultWinEventProcessor e=%p, e.w=%p, w=%p", e, e->focus, w);
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
        _drv_video_winblt( w );
        break;

    case UI_EVENT_WIN_REDECORATE:
    redecorate:
        if(w->flags & WFLAG_WIN_DECORATED)
        {
            w_lock();
            win_make_decorations(w);
            _drv_video_winblt_locked( w );
            w_unlock();
        }
        break;

    default:
        return 0;

    }

    return 1;
}

int defaultWindowEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
	//printf("defaultWindowEventProcessor e=%p, e.w=%p, w=%p", e, e->focus, w);

    switch(e->type)
    {
    case UI_EVENT_TYPE_MOUSE: 	return defaultMouseEventProcessor(w, e); 
#if KEY_EVENTS
    case UI_EVENT_TYPE_KEY:     return defaultKeyEventProcessor(w, e); 
#endif
    case UI_EVENT_TYPE_WIN:     return defaultWinEventProcessor(w, e); 
    }

    return 0;
}


