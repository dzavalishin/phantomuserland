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







// negative magic produces negative handles
//#define CONTROLS_POOL_MAGIC ('b' << 24 | 0xFFEEAA)
#define CONTROLS_POOL_MAGIC ('b')

struct checkb
{
    window_handle_t w;
    ui_event_t      e;
};



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


static void paint_button(window_handle_t win, control_t *cc )
{
    //bool pressed = cc->state == cs_pressed;

    paint_bg( win, cc );

    if(! (cc->flags & CONTROL_FLAG_NOBORDER))
    {
        if( (cc->state = cs_pressed) || (cc->state = cs_hover) )
            w_draw_rect( win, COLOR_WHITE, cc->r );
    }

    // tmp
    //if( cc->state == cs_hover )        w_draw_rect( win, COLOR_LIGHTRED, cc->r );


    if(cc->text)
    {
        int t_height = 16;
        int t_ypos = (cc->r.ysize - t_height) / 2;

        w_ttfont_draw_string( win, decorations_title_font,
                              cc->text, cc->fg_color,
                              cc->r.x+t_ypos+2, cc->r.y+t_ypos ); // +2?
    }

}


static void paint_menu_item(window_handle_t win, control_t *cc )
{
    //bool pressed = cc->state == cs_pressed;
    /*
    w_fill_rect( win, cc->bg_color, cc->r );

    if( cc->bg_image )
    {
        //drv_video_window_draw_bitmap( win, cc->r.x, cc->r.y, pressed ? cc->fg_image : cc->bg_image ); // left
        //drv_video_window_draw_bitmap( win, cc->r.x, cc->r.y, pressed ? cc->fg_image : cc->bg_image ); // right

        drv_video_bitmap_t *img = pressed ? cc->fg_image : cc->bg_image;

        if( img->xsize == 1 )
            w_replicate_hor( win, cc->r.x, cc->r.y, cc->r.xsize, img->pixel, img->ysize );
        else
            drv_video_window_draw_bitmap( win, cc->r.x, cc->r.y, img );
    }*/
    paint_bg( win, cc );

    if(! (cc->flags & CONTROL_FLAG_NOBORDER))
    {
        if( (cc->state = cs_pressed) || (cc->state = cs_hover) )
            w_draw_rect( win, COLOR_WHITE, cc->r );
    }

    if(cc->text)
    {
        int t_height = 16;
        int t_ypos = (cc->r.ysize - t_height) / 2;

        w_ttfont_draw_string( win, decorations_title_font,
                              cc->text, cc->fg_color,
                              cc->r.x+t_ypos+2, cc->r.y+t_ypos ); // +2?
    }

    LOG_FLOW( 1, "paint menu item id %d", cc->id );
}




// -----------------------------------------------------------------------
//
// General paint
//
// -----------------------------------------------------------------------



static void paint_control(window_handle_t w, control_t *cc )
{
    if(!(cc->flags & (CONTROL_FLAG_DISABLED|CONTROL_FLAG_NOPAINT)) ) 
    {
    switch(cc->type)
    {
        case ct_button:     paint_button( w, cc ); break;
        case ct_menu:       /* FALLTHROUGH */
        case ct_menuitem:   paint_menu_item( w, cc ); break;
        case ct_text:       //

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
    if(cc->changed) paint_control( w, cc );
}



/*
static errno_t kill_button_by_id(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    control_t *cc = el;
    int kill_id = (int)arg;

    if( cc->id == kill_id )
    {
        pool_release_el( pool, handle );
        pool_destroy_el( pool, handle );
    }
    return 0;
}
*/

/*
static errno_t kill_any_button(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) el;
    (void) arg;
    pool_release_el( pool, handle );
    pool_destroy_el( pool, handle );
    return 0;
}
*/

static errno_t do_paint_changed_control(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    control_ref_t *ref = el;    assert(ref);
    control_t *cc = ref->c;     assert(cc);
    struct checkb *env = arg;   assert(env);

    paint_changed_control( env->w, cc);
    return 0;
}

static errno_t do_repaint_control(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    control_ref_t *ref = el;    assert(ref);
    control_t *cc = ref->c;     assert(cc);
    struct checkb *env = arg;   assert(env);

    paint_control( env->w, cc );
    return 0;
}


static errno_t do_reset_control(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    control_ref_t *ref = el;    assert(ref);
    control_t *cc = ref->c;     assert(cc);
    struct checkb *env = arg;   assert(env);

    // Zero lower bits
    //cc->mouse_in_bits <<= 1;
    //cc->pressed_bits  <<= 1;
    cc->state = cs_released;

    // And repaint
    paint_control( env->w, cc );
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
    else if( cc->w_child ) w_set_visible( cc->w_child, isPressed );

    ui_event_t e = *ie;

    e.type = UI_EVENT_TYPE_WIN;
    e.w.info = isPressed ? UI_EVENT_WIN_BUTTON_ON : UI_EVENT_WIN_BUTTON_OFF;
    e.extra = cc->id;
    e.focus = w;

    ev_q_put_any( &e );

    cc->changed = 1;
}

/// Process event for controls
static errno_t do_check_control(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    control_ref_t *ref = el;    assert(ref);
    control_t *cc = ref->c;     assert(cc);
    struct checkb *env = arg;   assert(env);

// TODO this is processing for button or menu item, need switch
    //cc->mouse_in_bits <<= 1;
    //cc->pressed_bits  <<= 1;
    if(cc->flags & CONTROL_FLAG_DISABLED) 
        return 0;

    ui_event_t e = env->e;

    if( (e.type == UI_EVENT_TYPE_WIN) && (e.w.info == UI_EVENT_WIN_LOST_FOCUS) )
    {
        if(e.focus != env->w)
            return 0;

        if(cc->state == cs_hover) // TODO separate hover status from pressed status
        {
            cc->state = cs_released;
            cc->changed = 1;
        }
        return 0;
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
            return 0;
        }

        if(env->e.m.released & 0x1) // First button release only
        {
            if( !(cc->flags & CONTROL_FLAG_TOGGLE) )
                cc->state = cs_released;

            w_control_action(env->w, cc, &e);
            return 0;
        }

        if(cc->state == cs_released ) // TODO separate hover status from pressed status
        {
            cc->state = cs_hover;
            cc->changed = 1;
        }
        return 0;
    }

    if( (e.type == UI_EVENT_TYPE_MOUSE) && !point_in_rect( e.rel_x, e.rel_y, &cc->r ) )
    {
        if(cc->state == cs_hover) // TODO separate hover status from pressed status
        {
            cc->state = cs_released;
            cc->changed = 1;
        }
        return 0;
    }

    return 0;
}







// --------------------------------------------------------
//
// Pool
//
// --------------------------------------------------------

pool_t *create_controls_pool(void)
{
    pool_t *controls;

    controls = create_pool();
    controls->flag_autoclean = 1;
    controls->flag_autodestroy = 1;

    controls->magic = CONTROLS_POOL_MAGIC;

    return controls;
}

void destroy_controls_pool(pool_t *controls)
{
    assert( controls->magic == CONTROLS_POOL_MAGIC );
    destroy_pool(controls);
}

// -----------------------------------------------------------------------
//
// Events
//
// -----------------------------------------------------------------------


void w_paint_changed_controls(window_handle_t w)
{
    if(w->controls == 0)
        return;

    //LOG_FLOW( 1, "button check @ %d.%d buttons %x", x, y, mouseb );
    struct checkb env;
    bzero( &env, sizeof(env) );
    env.w = w;

    pool_foreach( w->controls, do_paint_changed_control, &env );
}

void w_repaint_controls(window_handle_t w)
{
    if(w->controls == 0)
        return;

    LOG_FLOW( 1, " @ %p", w );
    struct checkb env;
    bzero( &env, sizeof(env) );
    env.w = w;

    pool_foreach( w->controls, do_repaint_control, &env );
}

void w_reset_controls(window_handle_t w) // focus lost, mouse off window - make sure all buttons are off
{
    if(w->controls == 0)
        return;

    struct checkb env;
    bzero( &env, sizeof(env) );

    env.w = w;

    pool_foreach( w->controls, do_reset_control, &env );
}

void w_check_controls( window_handle_t w, ui_event_t *e )
{
    if(w->controls == 0)
        return;

    //LOG_FLOW( 1, "button check @ %d.%d buttons %x", e->rel_x, e->rel_y, e->m.buttons );
    struct checkb env;
    env.e = *e;
    env.w = w;

    assert( w->controls->magic == CONTROLS_POOL_MAGIC );
    pool_foreach( w->controls, do_check_control, &env );
    w_paint_changed_controls(w);
}


// -----------------------------------------------------------------------
//
// General controls management
//
// -----------------------------------------------------------------------


/*
void w_delete_control( window_handle_t w, int id )
{
    if(w->controls == 0)
        return;

    assert( w->controls->magic == CONTROLS_POOL_MAGIC );

    pool_foreach( w->controls, kill_button_by_id, (void *)id );
}
*/

void w_delete_control( window_handle_t w, pool_handle_t ch )
{
    pool_release_el( w->controls, ch );
    pool_destroy_el( w->controls, ch );
}


// -----------------------------------------------------------------------
//
// Getters and setters
//
// -----------------------------------------------------------------------


void w_control_set_text( window_handle_t w, pool_handle_t ch, const char *text, color_t text_color )
{
    if(w->controls == 0)        return;
    assert( w->controls->magic == CONTROLS_POOL_MAGIC );
    control_ref_t *ref = pool_get_el( w->controls, ch );

    if( !ref )
    {
        LOG_ERROR0( 1, "can't get control" );
        return;
    }

    control_t *cc = ref->c;
    assert(cc);

    //const char *old = cc->text;
    //cc->text = strdup(text);
    //if( old ) free((void *)old);

    strlcpy( cc->buffer, text, sizeof(cc->buffer) );
    cc->text = cc->buffer;

    cc->fg_color = text_color;
    paint_button( w, cc );

    pool_release_el( w->controls, ch );
}


void w_control_set_callback( window_handle_t w, pool_handle_t ch, control_callback_t cb, void *arg )
{
    if(w->controls == 0)        return;
    assert( w->controls->magic == CONTROLS_POOL_MAGIC );
    control_ref_t *ref = pool_get_el( w->controls, ch );
    assert(ref);
    control_t *cc = ref->c;
    assert(cc);

    cc->callback = cb;
    cc->callback_arg = arg;

    pool_release_el( w->controls, ch );
}



// -----------------------------------------------------------------------
//
// Creation and restart
//
// -----------------------------------------------------------------------


static control_handle_t control_to_pool( window_handle_t w, control_t *cc )
{
    assert(cc);

    // TODO take some mutex
    if(w->controls == 0)
        w->controls = create_controls_pool();

    // Just add to pool
    control_handle_t ch = pool_create_el( w->controls, calloc( 1, sizeof(control_ref_t) ) );
    if( ch < 0 )
    {
        free(cc);
        //LOG_ERROR0( 0, "out of buttons" );
        return INVALID_POOL_HANDLE;
    }

    control_ref_t *ref = pool_get_el( w->controls, ch );
    assert(ref);
    ref->c = cc;

    return ch;
}

static void w_add_to_group( window_handle_t w, control_t *cc )
{
    cc->group = 0;
    cc->next_in_group = 0;

    if( !cc->group_id ) return;

    // TODO take some mutex

    //control_group_t *   group;          //< Group we belong, if any
    //struct control *    next_in_group;  //< linked list of controls in group - radio or menu

}

static void w_image_defaults( window_handle_t w, control_t *cc )
{
    if( cc->pas_bg_image || cc->act_bg_image )
    {
        // Fill missing ones
        if(!cc->pas_bg_image) cc->pas_bg_image = cc->act_bg_image;
        if(!cc->act_bg_image) cc->act_bg_image = cc->pas_bg_image;

        // No. If we have no hober image we must know
        //if(!cc->ho_image) cc->ho_image = cc->bg_image;
    }
    
    if(cc->pas_bg_image)
    {
        if( 0 == cc->r.xsize ) cc->r.xsize = cc->pas_bg_image->xsize;
        if( 0 == cc->r.ysize ) cc->r.ysize = cc->pas_bg_image->ysize;
    }
}

static void w_clean_internal_state( window_handle_t w, control_t *cc )
{
    cc->group = 0;
    cc->next_in_group = 0;

    cc->state = cs_released;
    cc->focused = 0;        //< Selected in window
    cc->changed = 1;

    memset( cc->buffer, 0, sizeof(cc->buffer) );

    if(cc->text)
    {
        strlcpy( cc->buffer, cc->text, sizeof(cc->buffer) ); // TODO if buffer is nou enough?
        cc->text = cc->buffer;
    }
}

control_handle_t w_restart_control_persistent( window_handle_t w, control_t *cc )
{

    w_add_to_group(w,cc);
    w_image_defaults( w, cc );

    // Make sure caller will reassign
    cc->c_child = 0;
    cc->w_child = 0;
    cc->callback = 0;
    cc->callback_arg = 0;

    paint_control( w, cc );

    pool_handle_t ch = control_to_pool( w, cc );
    if(INVALID_POOL_HANDLE != ch) pool_release_el( w->controls, ch );
    return ch;
}

control_handle_t w_add_control_persistent(window_handle_t w, control_t *cc )
{
    w_clean_internal_state( w, cc );
    return w_restart_control_persistent( w, cc );
}

control_handle_t w_add_control( window_handle_t w, control_t *c )
{
    control_t *cc = calloc( 1, sizeof(control_t) );
    if( 0 == cc )
    {
        //LOG_ERROR0( 0, "out of buttons" );
        return INVALID_POOL_HANDLE;
    }

    *cc = *c; // Copy all settings

    w_clean_internal_state( w, cc );
    w_add_to_group( w, cc );
    w_image_defaults( w, cc );

    paint_control( w, cc );

    pool_handle_t ch = control_to_pool( w, cc );
    if(INVALID_POOL_HANDLE != ch) pool_release_el( w->controls, ch );
    return ch;
}


/// Same as w_add_control(), but you can pass list of controls linked by next_in_group field.
void w_add_controls( window_handle_t w, control_t *c )
{
    control_t *next = c->next_in_group; // w_add_conrtrol will use or clear it
    w_add_control( w, c );
    w_add_controls( w, next );
}

void w_clear_control( control_t *c )
{
    memset( c, 0, sizeof(control_t) );
}


// -----------------------------------------------------------------------
//
// Getters/Setters
//
// -----------------------------------------------------------------------


void w_control_set_visible( window_handle_t w, control_handle_t ch, int visible )
{
    if(w->controls == 0)        return;
    assert( w->controls->magic == CONTROLS_POOL_MAGIC );
    control_ref_t *ref = pool_get_el( w->controls, ch );

    if( !ref )
    {
        LOG_ERROR0( 1, "can't get control" );
        return;
    }

    control_t *cc = ref->c;
    assert(cc);

    cc->flags |= CONTROL_FLAG_DISABLED;
    if( visible ) cc->flags &= ~CONTROL_FLAG_DISABLED;    

    pool_release_el( w->controls, ch );
}


void w_control_set_children( window_handle_t w, control_handle_t ch, window_handle_t w_child, control_handle_t c_child )
{
    if(w->controls == 0)        return;
    assert( w->controls->magic == CONTROLS_POOL_MAGIC );
    control_ref_t *ref = pool_get_el( w->controls, ch );

    if( !ref )
    {
        LOG_ERROR0( 1, "can't get control" );
        return;
    }

    control_t *cc = ref->c;
    assert(cc);

    cc->c_child = c_child;
    cc->w_child = w_child;

    pool_release_el( w->controls, ch );
}

void w_control_set_flags( window_handle_t w, control_handle_t ch, int toSet, int toReset )
{
    if(w->controls == 0)        return;
    assert( w->controls->magic == CONTROLS_POOL_MAGIC );
    control_ref_t *ref = pool_get_el( w->controls, ch );

    if( !ref )
    {
        LOG_ERROR0( 1, "can't get control" );
        return;
    }

    control_t *cc = ref->c;
    assert(cc);

    cc->flags |= toSet;
    cc->flags &= ~toReset;

    pool_release_el( w->controls, ch );
}

// -----------------------------------------------------------------------
//
// Shortcuts for typical cases
//
// -----------------------------------------------------------------------

extern drv_video_bitmap_t menu_selected_center_bmp;
extern drv_video_bitmap_t menu_normal_center_bmp;

control_handle_t w_add_menu_item( window_handle_t w, int id, int x, int y, int xsize, const char*text, color_t text_color )
{
    control_t cb;
    w_clear_control( &cb );

    cb.type = ct_menuitem;

    cb.id = id;
    cb.group_id = (0xFF << 16) + 0; // Make some default group ID, so that all of them will stick to group

    cb.r.x = x;
    cb.r.y = y;
    cb.r.xsize = xsize;

    cb.flags = CONTROL_FLAG_NOBORDER;

    cb.text = text;
    cb.fg_color = text_color;

    cb.pas_bg_image = &menu_normal_center_bmp;
    cb.act_bg_image = &menu_selected_center_bmp;
    cb.hov_bg_image = &menu_selected_center_bmp;

    return w_add_control( w, &cb );
}

