/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Put event to main q.
 *
 *
**/

#define DEBUG_MSG_PREFIX "events"
#include <debug_ext.h>
#define debug_level_flow 4
#define debug_level_error 10
#define debug_level_info 10



#include "ev_private.h"

#include <dev/key_event.h>



void w_set_focused( window_handle_t toFocus )
{
    window_handle_t later_lost = 0;
    window_handle_t later_gain = toFocus;

    // Next win
    if(focused_window != 0)
        later_lost = focused_window;

    if( later_gain != focused_window )
    {
        focused_window = later_gain;

        if(later_lost) ev_q_put_win( 0, 0, UI_EVENT_WIN_LOST_FOCUS, later_lost );
        if(later_gain)
        {
            ev_q_put_win( 0, 0, UI_EVENT_WIN_GOT_FOCUS, later_gain );
            w_to_top(later_gain);
        }
    }
}



//! Put key event onto the main e q
void ev_q_put_key( int vkey, int ch, int modifiers )
{
    if(!ev_engine_active) return; // Just ignore


    // Process global hotkeys

    switch( vkey )
    {
    case KEY_TAB:
        LOG_FLOW( 4, "Tab shifts = %x", modifiers );
        if( UI_MOD_CTRL_DOWN(modifiers) || UI_MOD_ALT_DOWN(modifiers) )
        {
            LOG_FLOW( 3, "Next win shifts = %x", modifiers );
            w_set_focused( drv_video_next_window(focused_window) );
            return;
        }
        break;
    /*
    case KEY_F4:
        LOG_FLOW( 0, "F4 shifts = %x", modifiers );
        if( UI_MOD_CTRL_DOWN(modifiers) || UI_MOD_ALT_DOWN(modifiers) )
        {
            LOG_FLOW( 9, "Alt-F4 close win? = %x", modifiers );

        }
        break;
    */
//    case KEY_LWIN:
//    case KEY_RWIN:
//        LOG_FLOW( 0, "WIN shifts = %x", modifiers );
//        break;

    case KEY_MENU:
        LOG_FLOW( 0, "MENU shifts = %x", modifiers );
        break;

    }

    ui_event_t *e = ev_get_unused();

    e->type = UI_EVENT_TYPE_KEY;
    e->time = fast_time();
    e->focus= 0;

    e->k.vk = vkey;
    e->k.ch = ch;

    e->modifiers = modifiers;

    ev_put_event(e);
}

/*
//! Put mouse event onto the main e q
void ev_q_put_mouse( int x, int y, int buttons )
{
    if(!event_engine_active) return; // Just ignore

    ui_event_t *e = ev_get_unused();

	ev_make_mouse_event( e, x, y, buttons );

    //e->type = UI_EVENT_TYPE_MOUSE;
    //e->time = fast_time();
    //e->focus= 0;

    //e->m.buttons = buttons;
    //e->abs_x = x;
    //e->abs_y = y;

    ev_put_event(e);
}
*/

#if NEW_WINDOWS

//! Put window event onto the main e q
void ev_q_put_win( int x, int y, int info, pool_handle_t focus )
{
    if(!event_engine_active) return; // Just ignore

    ui_event_t *e = ev_get_unused();

    e->type = UI_EVENT_TYPE_WIN;
    e->time = fast_time();
    e->focus= focus;

    e->w.info = info;
    e->abs_x = x;
    e->abs_y = y;

    ev_put_event(e);
}

#else
//! Put window event onto the main e q
void ev_q_put_win( int x, int y, int info, window_handle_t focus )
{
    if(!ev_engine_active) return; // Just ignore

    ui_event_t *e = ev_get_unused();

    e->type = UI_EVENT_TYPE_WIN;
    e->time = fast_time();
    e->focus= focus;

    e->w.info = info;
    e->abs_x = x;
    e->abs_y = y;

    ev_put_event(e);
}
#endif


//! Put global event onto the main e q
void ev_q_put_global( ui_event_t *ie )
{
    if(!ev_engine_active) return; // Just ignore

    ui_event_t *e = ev_get_unused();

    *e = *ie;

    e->type = UI_EVENT_TYPE_GLOBAL;
    e->time = fast_time();
    e->focus= 0;

    ev_put_event(e);
}



void ev_q_put_any( ui_event_t *ie )
{
    if(!ev_engine_active) return; // Just ignore

    ui_event_t *e = ev_get_unused();

    *e = *ie;
    e->time = fast_time();

    ev_put_event(e);
}

