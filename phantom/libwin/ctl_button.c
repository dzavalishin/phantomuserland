/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Simple buttons.
 *
**/

#define DEBUG_MSG_PREFIX "button"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <video/control.h>


control_handle_t w_add_button( window_handle_t w, int id, int x, int y, drv_video_bitmap_t *released, drv_video_bitmap_t *pressed, int flags )
{
    control_t cb;
    w_clear_control( &cb );

    cb.type = ct_button;

    cb.id = id;
    cb.group_id = 0;

    cb.r.x = x;
    cb.r.y = y;

    cb.flags = flags;

    cb.fg_color = COLOR_RED;

    cb.pas_bg_image = released;
    cb.act_bg_image = pressed;
    cb.hov_bg_image = 0;

    return w_add_control( w, &cb );
}

