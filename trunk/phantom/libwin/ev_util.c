/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Event system helpers.
 *
 *
**/


#define DEBUG_MSG_PREFIX "events"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>
#include <kernel/debug.h>
#include "ev_private.h"


void ev_log( int level, ui_event_t *e )
{
    if( level > debug_level_flow )
        return;


    switch(e->type)
    {
    case UI_EVENT_TYPE_WIN:
    case UI_EVENT_TYPE_GLOBAL:
        lprintf( "ev WIN abs %d:%d\n", e->abs_x, e->abs_y, e->w.rect.xsize, e->w.rect.ysize );
        break;

    case UI_EVENT_TYPE_KEY:
        lprintf( "ev KEY ch %c vk %x\n", e->k.ch, e->k.vk );
        break;

    case UI_EVENT_TYPE_MOUSE:
        lprintf( "ev MOU abs %d:%d bt %x\n", e->abs_x, e->abs_y, e->m.buttons );
    }

}

