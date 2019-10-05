/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Event queue mouse support.
 *
 *
**/


#define DEBUG_MSG_PREFIX "events.mouse"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <event.h>
#include "ev_private.h"


void ev_make_mouse_event( struct ui_event *e, int x, int y, int buttons )
{
    memset( e, 0, sizeof(struct ui_event) );

    e->type = UI_EVENT_TYPE_MOUSE;
    e->time = fast_time();
    e->focus= 0;

    e->abs_x = x;
    e->abs_y = y;
    e->extra = 0;

    e->m.buttons = buttons;

    static int prev_buttons = 0;

    e->m.clicked  = buttons & ~prev_buttons;
    e->m.released = ~buttons & prev_buttons;

    prev_buttons = buttons;
}


