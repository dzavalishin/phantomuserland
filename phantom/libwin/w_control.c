/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * UI controls.
 *
**/

#define DEBUG_MSG_PREFIX "ui.ctl"
#include <debug_ext.h>
#define debug_level_flow 1
#define debug_level_error 10
#define debug_level_info 1


#include <phantom_types.h>
#include <phantom_libc.h>
#include <assert.h>

#include <kernel/pool.h>

#include <video/rect.h>
#include <video/window.h>
#include <video/bitmap.h>
#include <video/font.h>
#include <video/internal.h>

#include <video/control.h>

#include <dev/key_event.h>

#include "ctl_private.h"

static void ctl_reset_focus( window_handle_t w );

/**
 * 
 * Design notes.
 * 
 * We keep ref but not control pointer because we want be 
 * able to store controls in persistent memory.
 * 
**/







// -----------------------------------------------------------------------
//
// Paint bits
//
// -----------------------------------------------------------------------


static void paint_or_replicate(window_handle_t win, control_t *cc, drv_video_bitmap_t *img )
{
    if( img->xsize == 1 )
        w_replicate_hor( win, cc->r.x, cc->r.y, cc->r.xsize, img->pixel, img->ysize );
    else
        drv_video_window_draw_bitmap( win, cc->r.x, cc->r.y, img );
}

void ctl_paint_bg( window_handle_t win, control_t *cc )
{
    w_fill_rect( win, cc->bg_color, cc->r );

    if( !cc->pas_bg_image ) return;

    switch(cc->state)
    {
        default:
                            /* FALLTHROUGH */
        case cs_released:   

            if( cc->hov_bg_image && ( cc->focused || (cc->hovered == ch_hover) ) )
            {
                paint_or_replicate( win, cc, cc->hov_bg_image );
                break;
            }

            paint_or_replicate( win, cc, cc->pas_bg_image );
            break;

        case cs_pressed:    
            paint_or_replicate( win, cc, cc->act_bg_image );
            break;
    }
}

/// \return X shift for text
int ctl_paint_icon( window_handle_t win, control_t *cc )
{
    if(!cc->icon_image) return 0;

    if(cc->flags & CONTROL_FLAG_TEXT_RIGHT)
    {
        int y = (cc->r.ysize - cc->icon_image->ysize) / 2;
        // add y to both x and y to have equal dist
        w_draw_bitmap( win, cc->r.x + y, cc->r.y + y, cc->icon_image );
        return y*2 + cc->icon_image->xsize;
    }
    else
    {
        int y = (cc->r.ysize - cc->icon_image->ysize) / 2;
        // add y to x and subtract from y to have equal dist
        w_draw_bitmap( win, cc->r.x + cc->r.xsize - y - cc->icon_image->xsize, cc->r.y + y, cc->icon_image );
        return 0; // Do not shift text, it will be to the left from us
    }
    
}

void ctl_paint_text( window_handle_t win, control_t *cc, int shift )
{
    if(!cc->text) return;
    
    int t_height = 16;
    int t_ypos = (cc->r.ysize - t_height) / 2;

    w_ttfont_draw_string( win, decorations_title_font,
                          cc->text, cc->fg_color,
                          cc->r.x + t_ypos + 2 + shift, cc->r.y+t_ypos ); // +2?
    
}

void ctl_paint_border( window_handle_t win, control_t *cc )
{
    if( (cc->focused) || (cc->hovered == ch_hover) )
        w_draw_rect( win, COLOR_WHITE, cc->r );
    else 
    {
        if( ! (cc->flags & CONTROL_FLAG_NOBORDER) )
            w_draw_rect( win, cc->bg_color, cc->r );
    }
    // tmp
    //if( cc->state == cs_hover )        w_draw_rect( win, COLOR_LIGHTRED, cc->r );
}

// -----------------------------------------------------------------------
//
// Paint controls
//
// -----------------------------------------------------------------------


static void ctl_button_paint(window_handle_t win, control_t *cc )
{
    ctl_paint_bg( win, cc );
    int shift = ctl_paint_icon( win, cc );
    ctl_paint_text( win, cc, shift );
    ctl_paint_border( win, cc );
}


static void ctl_menu_item_paint(window_handle_t win, control_t *cc )
{
    ctl_paint_bg( win, cc );
    int shift = ctl_paint_icon( win, cc );
    ctl_paint_text( win, cc, shift );
    ctl_paint_border( win, cc );
    LOG_FLOW( 10, "paint menu item id %d", cc->id );
}

static void ctl_label_paint(window_handle_t win, control_t *cc )
{
    ctl_paint_bg( win, cc );
    int shift = ctl_paint_icon( win, cc );
    ctl_paint_text( win, cc, shift );
    ctl_paint_border( win, cc );
    //LOG_FLOW( 10, "paint label id %d", cc->id );
}



// -----------------------------------------------------------------------
//
// General paint
//
// -----------------------------------------------------------------------



void w_paint_control(window_handle_t w, control_t *cc )
{
    if(!(cc->flags & (CONTROL_FLAG_DISABLED|CONTROL_FLAG_NOPAINT)) ) 
    {
    switch(cc->type)
    {
        case ct_label:      ctl_label_paint( w, cc ); break;
        case ct_button:     ctl_button_paint( w, cc ); break;
        case ct_menuitem:   ctl_menu_item_paint( w, cc ); break;
        case ct_text:       ctl_text_field_paint( w, cc ); break;

        case ct_menu:       /* FALLTHROUGH */
        default:
            LOG_ERROR( 1, "unknown control type %d", cc->type);
            break;
    }
    }
    //w_update( win );
    ev_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, w );
    cc->changed = 0;
}

static void paint_changed_control(window_handle_t w, control_t *cc)
{
    if(cc->changed) w_paint_control( w, cc );
}




static errno_t do_paint_changed_control(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    control_ref_t *ref = el;    assert(ref);
    control_t *cc = ref->c;     assert(cc);
    struct foreach_control_param *env = arg;   assert(env);

    paint_changed_control( env->w, cc);
    return 0;
}

static errno_t do_repaint_control(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    control_ref_t *ref = el;    assert(ref);
    control_t *cc = ref->c;     assert(cc);
    struct foreach_control_param *env = arg;   assert(env);

    w_paint_control( env->w, cc );
    return 0;
}


static errno_t do_reset_control(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    control_ref_t *ref = el;    assert(ref);
    control_t *cc = ref->c;     assert(cc);
    struct foreach_control_param *env = arg;   assert(env);

    // Zero lower bits
    //cc->mouse_in_bits <<= 1;
    //cc->pressed_bits  <<= 1;
    cc->state = cs_released;

    // And repaint
    w_paint_control( env->w, cc );
    return 0;
}



/// Perform an action for control (button pressed, enter in text field, etc)
/// TODO release action?
void w_control_action(window_handle_t w, control_t *cc, ui_event_t *ie)
{
    int isPressed = cc->state == cs_pressed;

    if( cc->callback )
        cc->callback( w, cc );

    if( cc->c_child ) w_control_set_visible( cc->w_child, cc->c_child, isPressed );
    else if( cc->w_child ) 
    {
        //if(isPressed) w_to_top( cc->w_child ); // recursive mutex lock - TODO need event
        ev_q_put_win( 0, 0, UI_EVENT_WIN_TO_TOP, cc->w_child );
        w_set_visible( cc->w_child, isPressed );
    }

    ui_event_t e = *ie;

    e.type = UI_EVENT_TYPE_WIN;
    e.w.info = isPressed ? UI_EVENT_WIN_BUTTON_ON : UI_EVENT_WIN_BUTTON_OFF;
    e.extra = cc->id;
    e.focus = w;

    ev_q_put_any( &e );

    cc->changed = 1;
}







/// Process event for control - called for all win controls
static errno_t do_event_to_control(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    control_ref_t *ref = el;                   assert(ref);
    control_t *cc = ref->c;                    assert(cc);
    struct foreach_control_param *env = arg;   assert(env);

    if(cc->flags & (CONTROL_FLAG_DISABLED|CONTROL_FLAG_NOFOCUS) ) 
        return 0; // Event is not seen by me

    if( (env->e.type == UI_EVENT_TYPE_MOUSE) && point_in_rect( env->e.rel_x, env->e.rel_y, &cc->r ) )
    {
        if(!cc->focused)
        {
            ctl_reset_focus(env->w);
            cc->focused = 1;
            cc->changed = 1;
        }
    }

    switch( cc->type )
    {
        case ct_menuitem: /* FALLTHROUGH */
        case ct_button: 
            return ctl_button_events(cc, env);
            break;
/*
        case ct_label: // No reaction
            // TODO use CONTROL_FLAG_NOFOCUS and kill this case
            break;
*/
        case ct_text: ctl_text_field_events(cc,env);
            break;
    }

    return 0;
}





// -----------------------------------------------------------------------
//
// Events & paint entry points
//
// -----------------------------------------------------------------------


void w_paint_changed_controls(window_handle_t w)
{
    if(w->controls == 0)
        return;

    //LOG_FLOW( 1, "button check @ %d.%d buttons %x", x, y, mouseb );
    struct foreach_control_param env;
    bzero( &env, sizeof(env) );
    env.w = w;

    pool_foreach( w->controls, do_paint_changed_control, &env );
}

void w_repaint_controls(window_handle_t w)
{
    if(w->controls == 0)
        return;

    LOG_FLOW( 10, " @ %p", w );
    struct foreach_control_param env;
    bzero( &env, sizeof(env) );
    env.w = w;

    pool_foreach( w->controls, do_repaint_control, &env );
}


/**
 * 
 * Seems to be unused. 
 * 
 * \todo check me
 * 
 * 
**/
void w_reset_controls(window_handle_t w) // focus lost, mouse off window - make sure all buttons are off
{
    if(w->controls == 0)
        return;

    struct foreach_control_param env;
    bzero( &env, sizeof(env) );

    env.w = w;

    pool_foreach( w->controls, do_reset_control, &env );
}


//static int do_pass_focus(control_t *cc, struct foreach_control_param *env)
static errno_t do_pass_focus(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    control_ref_t *ref = el;                   assert(ref);
    control_t *cc = ref->c;                    assert(cc);
    struct foreach_control_param *env = arg;   assert(env);

    LOG_FLOW0( 0, "enter");

    if( cc->focused )
    {
        env->focus_flag = 1; // Pass flag to next control
        cc->focused = 0;
        cc->changed = 1;
        LOG_FLOW0( 0, "gave out");
        return 0;
    }

    // Label does not need focus, pass forward
    //if( cc->type == ct_label ) return 0; // TODO kill me
    if( cc->flags & CONTROL_FLAG_NOFOCUS) return 0;

    if( env->focus_flag )
    {
        cc->focused = 1; // Get flag from prev control
        cc->changed = 1;
        env->focus_flag = 0;
        env->focus_success = 1; // tell that someone got focus
        LOG_FLOW0( 0, "took in");
    }

    return 0;
}

#include <dev/key_event.h>

/**
 * 
 * Deliver event to controls. Mouse and focus loss events accepted.
 * 
 * \param[in] e Event to deliver.
 * 
 * \return True (non-zero) if event is consumed, and should not be delivered to containing window and other controls.
 * 
**/
int w_event_to_controls( window_handle_t w, ui_event_t *e )
{
    if(w->controls == 0)
        return  0;

    //LOG_FLOW( 1, "control event check @ %d.%d buttons %x", e->rel_x, e->rel_y, e->m.buttons );
    struct foreach_control_param env;
    bzero( &env, sizeof(env) );
    env.e = *e;
    env.w = w;

    assert( w->controls->magic == CONTROLS_POOL_MAGIC );
    int rc;

    if( (e->type == UI_EVENT_TYPE_KEY) && (e->k.vk == KEY_TAB) && UI_MOD_DOWN(e->modifiers) )
    {
        // Select next control to be focused
        env.focus_flag = 0;
        env.focus_success = 0;
        rc = pool_foreach( w->controls, do_pass_focus, &env );

        if(!env.focus_success) // No one got focus
        {
            LOG_FLOW0( 4, "no focus, bring in");
            // Select next control to be focused
            env.focus_flag = 1;
            env.focus_success = 0;
            rc = pool_foreach( w->controls, do_pass_focus, &env );
            // Do not check again
        }
        w_paint_changed_controls(w);
        return 1;
    }
    else
    {
        // Just deliver event to all of them
        rc = pool_foreach( w->controls, do_event_to_control, &env );
    }

    w_paint_changed_controls(w);
    return rc;
}






static errno_t do_clear_focus(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    control_ref_t *ref = el;                   assert(ref);
    control_t *cc = ref->c;                    assert(cc);
    struct foreach_control_param *env = arg;   assert(env);

    LOG_FLOW0( 0, "enter");

    if( cc->focused )
        cc->changed = 1;

    cc->focused = 0;
    return 0;
}


static void ctl_reset_focus( window_handle_t w )
{
    struct foreach_control_param env;
    bzero( &env, sizeof(env) );
    env.w = w;
    pool_foreach( w->controls, do_clear_focus, &env );
}
