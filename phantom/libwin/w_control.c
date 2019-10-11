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
//#define BUTTONS_POOL_MAGIC ('b' << 24 | 0xFFEEAA)
#define BUTTONS_POOL_MAGIC ('b')

struct checkb
{
    window_handle_t w;
    ui_event_t      e;
};




static void paint_button(window_handle_t win, control_t *cc )
{
    bool pressed = !! (cc->pressed_bits & 1);

    w_fill_rect( win, cc->bg_color, cc->r );

    if( cc->bg_image && ! (cc->flags & CONTROL_FLAG_NOPAINT) )
    {
        drv_video_window_draw_bitmap( win, cc->r.x, cc->r.y, pressed ? cc->fg_image : cc->bg_image );
    }

    if(! (cc->flags & CONTROL_FLAG_NOBORDER))
    {
        if( cc->mouse_in_bits & 1 )
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

    //w_update( win );
    ev_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, win );
}

static void paint_changed_button(window_handle_t win, control_t *cc)
{
    if(
       ((cc->mouse_in_bits & 1) == ( (cc->mouse_in_bits>>1) & 1 ))
        &&
       ((cc->pressed_bits & 1) == ( (cc->pressed_bits>>1) & 1 ))
      )
        return;

    paint_button( win, cc );
}



// -----------------------------------------------------------------------
//
// General paint
//
// -----------------------------------------------------------------------



static void paint_control(window_handle_t w, control_t *cc )
{
    switch(cc->type)
    {
        case ct_button:     paint_button( w, cc ); return;
        case ct_text:       //
        case ct_menu:       //
        case ct_submenu:    // 

        default:
            LOG_ERROR( 1, "unknown control type %d", cc->type);
            break;
    }
}

static void paint_changed_control(window_handle_t w, control_t *cc)
{
    switch(cc->type)
    {
        case ct_button:     paint_changed_button( w, cc ); return;
        case ct_text:       //
        case ct_menu:       //
        case ct_submenu:    //

        default:
            // Ignore
            break;
    }
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

    control_t *cc = el;
    struct checkb *env = arg;

    paint_changed_control( env->w, cc);
    return 0;
}

static errno_t do_repaint_control(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    control_t *cc = el;
    struct checkb *env = arg;

    paint_control( env->w, cc );
    return 0;
}


static errno_t do_reset_control(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    control_t *cc = el;
    struct checkb *env = arg;

    // Zero lower bits
    cc->mouse_in_bits <<= 1;
    cc->pressed_bits  <<= 1;

    // And repaint
    paint_control( env->w, cc );
    return 0;
}


static errno_t do_check_control(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    control_t *cc = el;
    struct checkb *env = arg;

    cc->mouse_in_bits <<= 1;
    cc->pressed_bits  <<= 1;

    if( point_in_rect( env->e.rel_x, env->e.rel_y, &cc->r ) )
    {
        //LOG_FLOW( 1, "button @ %d.%d in range id %x", env->e.rel_x, env->e.rel_y, cc->id );
        cc->mouse_in_bits |= 1;

        if(env->e.m.clicked & 0x1) // First button click only
        {
            cc->pressed_bits |= 1;

            ui_event_t e = env->e;

            e.type = UI_EVENT_TYPE_WIN;
            e.w.info = UI_EVENT_WIN_BUTTON;
            e.extra = cc->id;
            e.focus = env->w;

            ev_q_put_any( &e );

            if( cc->callback )
                cc->callback( env->w, cc );
        }
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

    controls->magic = BUTTONS_POOL_MAGIC;

    return controls;
}

void destroy_controls_pool(pool_t *controls)
{
    assert( controls->magic == BUTTONS_POOL_MAGIC );
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

    struct checkb env;

    //LOG_FLOW( 1, "button check @ %d.%d buttons %x", x, y, mouseb );

    bzero( &env, sizeof(env) );

    env.w = w;

    pool_foreach( w->controls, do_paint_changed_control, &env );
}

void w_repaint_controls(window_handle_t w)
{
    if(w->controls == 0)
        return;

    struct checkb env;

    //LOG_FLOW( 1, "button check @ %d.%d buttons %x", x, y, mouseb );

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

    struct checkb env;

    //LOG_FLOW( 1, "button check @ %d.%d buttons %x", e->rel_x, e->rel_y, e->m.buttons );

    env.e = *e;
    env.w = w;

    assert( w->controls->magic == BUTTONS_POOL_MAGIC );

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

    assert( w->controls->magic == BUTTONS_POOL_MAGIC );

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
    assert( w->controls->magic == BUTTONS_POOL_MAGIC );
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
    assert( w->controls->magic == BUTTONS_POOL_MAGIC );
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
    if( cc->bg_image || cc->fg_image || cc->ho_image )
    {
        // Fill missing ones
        if(!cc->bg_image) cc->bg_image = cc->fg_image ? cc->fg_image : cc->ho_image;
        if(!cc->fg_image) cc->fg_image = cc->bg_image ? cc->bg_image : cc->ho_image;

        if(!cc->ho_image) cc->ho_image = cc->bg_image;
    }
    
    if(cc->bg_image)
    {
        if( 0 == cc->r.xsize ) cc->r.xsize = cc->bg_image->xsize;
        if( 0 == cc->r.ysize ) cc->r.ysize = cc->bg_image->ysize;
    }
}

static void w_clean_internal_state( window_handle_t w, control_t *cc )
{
    cc->group = 0;
    cc->next_in_group = 0;

    cc->state = cs_default;
    cc->focused = 0;        //< Selected in window

    cc->mouse_in_bits = 0;  // TODO KILLME
    cc->pressed_bits = 0;   // TODO KILLME

    memset( cc->buffer, 0, sizeof(cc->buffer) );

    if(cc->text)
        strlcpy( cc->buffer, cc->text, sizeof(cc->buffer) ); // TODO if buffer is nou enough?

    cc->text = cc->buffer;
}

control_handle_t w_restart_control_persistent( window_handle_t w, control_t *cc )
{

    w_add_to_group(w,cc);
    w_image_defaults( w, cc );

    // Make sure caller will reassign
    cc->child = 0;
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


