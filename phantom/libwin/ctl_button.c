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
#include <video/builtin_bitmaps.h>
#include "ctl_private.h"


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


control_handle_t w_add_radio_button( window_handle_t w, int id, int group_id, int x, int y )
{
    control_t cb;
    w_clear_control( &cb );

    cb.type = ct_button;

    cb.id = id;
    cb.group_id = group_id;

    cb.r.x = x;
    cb.r.y = y;

    cb.flags = CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE;

    cb.fg_color = COLOR_RED;

    cb.pas_bg_image = &checkbox_round_off_a_x30_bmp; // to get size
    cb.act_bg_image = &checkbox_round_on_a_x30_bmp;
    cb.hov_bg_image = 0;

    //ctl_img_copy_and_blend( &cb.pas_bg_image, &cb.pas_bg_alloc, &checkbox_round_off_a_x30_bmp, w, &cb );
    //ctl_img_copy_and_blend( &cb.act_bg_image, &cb.act_bg_alloc, &checkbox_round_on_a_x30_bmp, w, &cb );

    control_handle_t bh = w_add_control( w, &cb );

    w_control_set_background( w, bh, &checkbox_round_off_a_x30_bmp, &checkbox_round_on_a_x30_bmp, 0 );

    return bh;
}














/// Click or button press which must turn us on or toggle
static void ctl_button_click_on(window_handle_t w, control_t *cc, ui_event_t *e)
{
    if( cc->group_id != 0 )
        ctl_reset_group( w, cc->group_id );

    if( cc->flags & CONTROL_FLAG_TOGGLE)
        cc->state = (cc->state == cs_pressed) ? cs_released : cs_pressed; // toggle
    else
        cc->state = cs_pressed;

    w_control_action(w, cc, e);
}


/// Click or button press which must turn us of or be ignored
static void ctl_button_click_off(window_handle_t w, control_t *cc, ui_event_t *e)
{
    if( cc->group_id != 0 ) return;

    if( !(cc->flags & CONTROL_FLAG_TOGGLE) )
        {
            cc->state = cs_released;
            w_control_action(w, cc, e);
        }
}

/// Process event for button or menu item
int ctl_button_events(control_t *cc, struct foreach_control_param *env)
{
    ui_event_t e = env->e;

    if( (e.type == UI_EVENT_TYPE_KEY) && (cc->focused) )
    {
        switch( e.k.ch )
        {
            case KEY_ENTER:
            /* FALLTHROUGH */
            case ' ':
                if( e.modifiers & UI_MODIFIER_KEYUP)  
                    ctl_button_click_off(env->w, cc, &env->e);
                else                                  
                    ctl_button_click_on(env->w, cc, &env->e);
        }
    }

    if( (e.type == UI_EVENT_TYPE_WIN) && (e.w.info == UI_EVENT_WIN_LOST_FOCUS) )
    {
        if(e.focus != env->w)
            return 0;

        if(cc->hovered == ch_hover) // TODO separate hover status from pressed status
        {
            cc->hovered = ch_normal;
            cc->changed = 1;
        }
        return 0; // Don't consume
    }
    
    // Event is for us
    if( (e.type == UI_EVENT_TYPE_MOUSE) && point_in_rect( e.rel_x, e.rel_y, &cc->r ) )
    {
        //LOG_FLOW( 1, "button @ %d.%d in range id %x", env->e.rel_x, env->e.rel_y, cc->id );
        //cc->mouse_in_bits |= 1;

        if(env->e.m.clicked & 0x1) // First button click only
        {
/*            
            if( cc->flags & CONTROL_FLAG_TOGGLE)
                cc->state = (cc->state == cs_pressed) ? cs_released : cs_pressed; // toggle
            else
                cc->state = cs_pressed;

            w_control_action(env->w, cc, &env->e);
*/
            ctl_button_click_on( env->w, cc, &env->e );
            return 1; // Consume
        }

        if(env->e.m.released & 0x1) // First button release only
        {
            /*if( !(cc->flags & CONTROL_FLAG_TOGGLE) )
            {
                cc->state = cs_released;
                w_control_action(env->w, cc, &e);
            }*/
            ctl_button_click_off( env->w, cc, &env->e );
            return 1; // Consume
        }

        if(cc->hovered == ch_normal )
        {
            cc->hovered = ch_hover;
            cc->changed = 1;
        }
        return 0; // Do not eat events on just hover - prevents others from loosing hover status
    }

    // Not ours, but take in account
    if( (e.type == UI_EVENT_TYPE_MOUSE) && !point_in_rect( e.rel_x, e.rel_y, &cc->r ) )
    {
        if(cc->hovered == ch_hover) // TODO separate hover status from pressed status
        {
            cc->hovered = ch_normal;
            cc->changed = 1;
        }
        return 0; // Do not consume
    }

    return 0; // Not ours
}
