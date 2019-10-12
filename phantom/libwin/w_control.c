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
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


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

static void paint_bg( window_handle_t win, control_t *cc )
{
    w_fill_rect( win, cc->bg_color, cc->r );

    if( !cc->pas_bg_image ) return;

    switch(cc->state)
    {
        case cs_hover:      if( cc->hov_bg_image )
                            {
                                //w_draw_bitmap( win, cc->r.x, cc->r.y, cc->hov_bg_image );
                                paint_or_replicate( win, cc, cc->hov_bg_image );
                                break;
                            }
                            /* FALLTHROUGH */
        default:
                            /* FALLTHROUGH */
        case cs_released:   
            //w_draw_bitmap( win, cc->r.x, cc->r.y, cc->pas_bg_image );
            paint_or_replicate( win, cc, cc->pas_bg_image );
            break;

        case cs_pressed:    
            //w_draw_bitmap( win, cc->r.x, cc->r.y, cc->act_bg_image );
            paint_or_replicate( win, cc, cc->act_bg_image );
            break;
    }
}

/// \return X shift for text
static int paint_icon( window_handle_t win, control_t *cc )
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

static void paint_text( window_handle_t win, control_t *cc, int shift )
{
    if(!cc->text) return;
    
    int t_height = 16;
    int t_ypos = (cc->r.ysize - t_height) / 2;

    w_ttfont_draw_string( win, decorations_title_font,
                          cc->text, cc->fg_color,
                          cc->r.x + t_ypos + 2 + shift, cc->r.y+t_ypos ); // +2?
    
}

static void paint_border( window_handle_t win, control_t *cc )
{
    if(! (cc->flags & CONTROL_FLAG_NOBORDER))
    {
        if( (cc->state == cs_pressed) || (cc->state == cs_hover) )
            w_draw_rect( win, COLOR_WHITE, cc->r );
    }
    // tmp
    //if( cc->state == cs_hover )        w_draw_rect( win, COLOR_LIGHTRED, cc->r );
}

// -----------------------------------------------------------------------
//
// Paint controls
//
// -----------------------------------------------------------------------


static void paint_button(window_handle_t win, control_t *cc )
{
    paint_bg( win, cc );
    int shift = paint_icon( win, cc );
    paint_text( win, cc, shift );
    paint_border( win, cc );
}


static void paint_menu_item(window_handle_t win, control_t *cc )
{
    paint_bg( win, cc );
    int shift = paint_icon( win, cc );
    paint_text( win, cc, shift );
    paint_border( win, cc );
    LOG_FLOW( 10, "paint menu item id %d", cc->id );
}

static void paint_label(window_handle_t win, control_t *cc )
{
    paint_bg( win, cc );
    int shift = paint_icon( win, cc );
    paint_text( win, cc, shift );
    paint_border( win, cc );
    //LOG_FLOW( 10, "paint label id %d", cc->id );
}


static void ctl_text_shift_for_cursor( control_t *cc )
{
    if( cc->cursor_shift < cc->vis_shift )
        cc->vis_shift = cc->cursor_shift;

    if( cc->cursor_shift > (cc->vis_shift + cc->vis_len) )
        cc->vis_shift = cc->cursor_shift - cc->vis_len;
}

// TODO rewrite all this in UTF code points, not bytes!
// Add ttf render func for UTF

static void find_visible_text_len( control_t *cc )
{
    int vis_width_pixels = 0;
    // TODO we must cut off not byte but UTF-8 char
    while(1) {
        rect_t r;

        if( cc->vis_len > cc->str_len - cc->vis_shift )
            cc->vis_len = cc->str_len - cc->vis_shift;

        w_ttfont_string_size( decorations_title_font,
                          cc->buffer + cc->vis_shift, cc->vis_len,
                           &r );

        if (vis_width_pixels <= cc->r.xsize )
            break;

        // sanity
        if( cc->vis_len <= 1 )
            break;
    }     
}

static void paint_text_ctl(window_handle_t win, control_t *cc )
{
    paint_bg( win, cc );    
    
    int t_height = 16;
    int t_ypos = (cc->r.ysize - t_height) / 2;

    ctl_text_shift_for_cursor( cc );
    find_visible_text_len( cc );
    ctl_text_shift_for_cursor( cc ); // Width could change

    size_t cursor_x_pos = 0;

    w_ttfont_draw_string_ext( win, decorations_title_font,
                          cc->buffer+cc->vis_shift, cc->vis_len,
                          cc->fg_color,
                          cc->r.x + t_ypos + 2, cc->r.y+t_ypos,
                          &cursor_x_pos, cc->cursor_shift ); // +2?
    
    //w_draw_v_line( win, COLOR_GREEN, cursor_x_pos, cc->r.y, cc->r.ysize );
    w_draw_line( win, cursor_x_pos, cc->r.y, cursor_x_pos, cc->r.y+cc->r.ysize, COLOR_LIGHTGREEN );

    paint_border( win, cc );
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
        case ct_label:      paint_label( w, cc ); break;
        case ct_button:     paint_button( w, cc ); break;
        case ct_menu:       /* FALLTHROUGH */
        case ct_menuitem:   paint_menu_item( w, cc ); break;
        case ct_text:       paint_text_ctl( w, cc ); break;

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
static void w_control_action(window_handle_t w, control_t *cc, ui_event_t *ie)
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
static void do_button_events(control_t *cc, struct foreach_control_param *env)
{
    if(cc->flags & CONTROL_FLAG_DISABLED) 
        return;

    ui_event_t e = env->e;

    if( (e.type == UI_EVENT_TYPE_WIN) && (e.w.info == UI_EVENT_WIN_LOST_FOCUS) )
    {
        if(e.focus != env->w)
            return;

        if(cc->state == cs_hover) // TODO separate hover status from pressed status
        {
            cc->state = cs_released;
            cc->changed = 1;
        }
        return;
    }

    if( (e.type == UI_EVENT_TYPE_MOUSE) && point_in_rect( e.rel_x, e.rel_y, &cc->r ) )
    {
        //LOG_FLOW( 1, "button @ %d.%d in range id %x", env->e.rel_x, env->e.rel_y, cc->id );
        //cc->mouse_in_bits |= 1;

        if(env->e.m.clicked & 0x1) // First button click only
        {
            if( cc->flags & CONTROL_FLAG_TOGGLE)
                cc->state = (cc->state == cs_pressed) ? cs_released : cs_pressed; // toggle
            else
                cc->state = cs_pressed;

            w_control_action(env->w, cc, &env->e);
            return;
        }

        if(env->e.m.released & 0x1) // First button release only
        {
            if( !(cc->flags & CONTROL_FLAG_TOGGLE) )
                cc->state = cs_released;

            w_control_action(env->w, cc, &e);
            return;
        }

        if(cc->state == cs_released ) // TODO separate hover status from pressed status
        {
            cc->state = cs_hover;
            cc->changed = 1;
        }
        return;
    }

    if( (e.type == UI_EVENT_TYPE_MOUSE) && !point_in_rect( e.rel_x, e.rel_y, &cc->r ) )
    {
        if(cc->state == cs_hover) // TODO separate hover status from pressed status
        {
            cc->state = cs_released;
            cc->changed = 1;
        }
        return;
    }
}


/// Process event for control - called for all win controls
static errno_t do_check_control(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    control_ref_t *ref = el;                   assert(ref);
    control_t *cc = ref->c;                    assert(cc);
    struct foreach_control_param *env = arg;   assert(env);

    switch( cc->type )
    {
        case ct_menuitem: /* FALLTHROUGH */
        case ct_button: 
            do_button_events(cc, env);
            break;

        case ct_label: // No reaction
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

    LOG_FLOW( 1, " @ %p", w );
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


/**
 * 
 * Deliver event to controls. Mouse and focus loss events accepted.
 * 
 * \param[in] e Event to deliver.
 * 
**/
void w_check_controls( window_handle_t w, ui_event_t *e )
{
    if(w->controls == 0)
        return;

    //LOG_FLOW( 1, "button check @ %d.%d buttons %x", e->rel_x, e->rel_y, e->m.buttons );
    struct foreach_control_param env;
    env.e = *e;
    env.w = w;

    assert( w->controls->magic == CONTROLS_POOL_MAGIC );
    pool_foreach( w->controls, do_check_control, &env );
    w_paint_changed_controls(w);
}






