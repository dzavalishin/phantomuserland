/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system helpers: client window default event processor.
 *
 *
**/



#define DEBUG_MSG_PREFIX "wevent"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <video/window.h>
#include <video/internal.h>

#include <assert.h>
#include <phantom_libc.h>
#include <event.h>
#include <wtty.h>

#include <threads.h>





static int defaultMouseEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
    //printf("defaultMouseEventProcessor buttons %x, %d-%d\r", e->m.buttons, e->abs_x, e->abs_y);

    if( (e->m.clicked & UI_MOUSE_BTN_RIGHT) && (w->context_menu != 0) )
    {
        LOG_FLOW0(5, "have context right click");
        ev_q_put_win( 0, 0, UI_EVENT_WIN_TO_TOP, w->context_menu );
        ev_q_put_win( e->abs_x, e->abs_y, UI_EVENT_WIN_MOVE, w->context_menu );
        w_set_visible( w->context_menu, 1 );
    }

    if( e->m.buttons )
        w_to_top(w->w_owner ? w->w_owner : w);

    return 0;
}

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
#if 0 // have it in keybd driver
    if( e->modifiers & UI_MODIFIER_CTRL )
    {
        ch &= 0x1F; // Control char
    }
    else if( e->modifiers & (UI_MODIFIER_SHIFT|UI_MODIFIER_CAPSLOCK) )
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
#endif
    errno_t err = wtty_putc_nowait(wt, ch );
    if(err == ENOMEM)
    {
        LOG_ERROR0( 1, "Window keyb buffer overflow" );
    }
    else if(err)
    {
        LOG_ERROR( 1, "Window putc error %d", err );
    }
    return 1;
}


static int defaultWinEventProcessor( drv_video_window_t *w, struct ui_event *e )
{
    //drv_video_window_t *mainw = w->w_owner ? drv_video_window_t *w : w;
    //printf("defaultWinEventProcessor e=%p, e.w=%p, w=%p", e, e->focus, w);
    switch(e->w.info)
    {
    case UI_EVENT_WIN_GOT_FOCUS:
        w->state |= WSTATE_WIN_FOCUSED;
        goto redecorate;

    case UI_EVENT_WIN_LOST_FOCUS:
        w->state &= ~WSTATE_WIN_FOCUSED;
        if(WIN_HAS_FLAG(w,WFLAG_WIN_HIDE_ON_FOCUS_LOSS))
            w_set_visible( w, 0 );
        goto redecorate;

    case UI_EVENT_WIN_DESTROYED:
        break;

    case UI_EVENT_WIN_REPAINT:
        iw_winblt( w );
        break;

    case UI_EVENT_WIN_REDECORATE:
    redecorate:
        if(w->flags & WFLAG_WIN_DECORATED)
        {
            w_lock();
            win_make_decorations(w);
            iw_winblt_locked( w );
            w_unlock();
        }
        break;

    case UI_EVENT_WIN_BUTTON_ON: 
        //printf("client win button %x\n", e->extra );
    break;

    case UI_EVENT_WIN_TO_TOP:    w_to_top( w ); break;
    case UI_EVENT_WIN_TO_BOTTOM: w_to_bottom( w ); break;

    case UI_EVENT_WIN_MOVE:      w_move( w, e->abs_x, e->abs_y ); break;

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
    case UI_EVENT_TYPE_KEY:     return defaultKeyEventProcessor(w, e); 
    case UI_EVENT_TYPE_WIN:     return defaultWinEventProcessor(w, e); 
    }

    return 0;
}


