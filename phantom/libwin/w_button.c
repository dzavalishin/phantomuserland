#if 0 // controls
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

// negative magic produces negative handles
//#define BUTTONS_POOL_MAGIC ('b' << 24 | 0xFFEEAA)
#define BUTTONS_POOL_MAGIC ('b')

struct checkb
{
    window_handle_t w;
    ui_event_t      e;
};




static void paint_button(window_handle_t win, button_t *cb )
{
    bool pressed = !! (cb->pressed_bits & 1);

    w_fill_rect( win, cb->color, cb->r );

    if( cb->bmp && ! (cb->flags & BUTTON_FLAG_NOPAINT) )
    {
        drv_video_window_draw_bitmap( win, cb->r.x, cb->r.y, pressed ? cb->bmp_pressed : cb->bmp );
    }

    if(! (cb->flags & BUTTON_FLAG_NOBORDER))
    {
        if( cb->mouse_in_bits & 1 )
            w_draw_rect( win, COLOR_WHITE, cb->r );
    }

    if(cb->text)
    {

#if CONF_TRUETYPE
        int t_height = 16;
        int t_ypos = (cb->r.ysize - t_height) / 2;
        w_ttfont_draw_string( win, decorations_title_font,
                              cb->text, cb->text_color,
                              cb->r.x+t_ypos+2, cb->r.y+t_ypos ); // +2?
#else
        int t_height = 16;
        int t_ypos = (cb->r.ysize - t_height) / 2;
        w_font_draw_string( win, &drv_video_8x16san_font,
                            cb->text, cb->text_color,
                            COLOR_TRANSPARENT,
                            cb->r.x+t_ypos+2, cb->r.y+t_ypos ); // +2?
#endif
    }

    //w_update( win );
    ev_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, win );
}

static void paint_changed_button(window_handle_t win, button_t *cb)
{
    if(
       ((cb->mouse_in_bits & 1) == ( (cb->mouse_in_bits>>1) & 1 ))
        &&
       ((cb->pressed_bits & 1) == ( (cb->pressed_bits>>1) & 1 ))
      )
        return;

    paint_button( win, cb );
}

/*
static errno_t kill_button_by_id(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    button_t *cb = el;
    int kill_id = (int)arg;

    if( cb->id == kill_id )
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

static errno_t do_paint_changed_button(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    button_t *cb = el;
    struct checkb *env = arg;

    paint_changed_button( env->w, cb);
    return 0;
}

static errno_t do_repaint_button(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    button_t *cb = el;
    struct checkb *env = arg;

    paint_button( env->w, cb );
    return 0;
}


static errno_t do_reset_button(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    button_t *cb = el;
    struct checkb *env = arg;

    // Zero lower bits
    cb->mouse_in_bits <<= 1;
    cb->pressed_bits  <<= 1;

    // And repaint
    paint_button( env->w, cb );
    return 0;
}


static errno_t do_check_button(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    button_t *cb = el;
    struct checkb *env = arg;

    cb->mouse_in_bits <<= 1;
    cb->pressed_bits  <<= 1;

    if( point_in_rect( env->e.rel_x, env->e.rel_y, &cb->r ) )
    {
        //SHOW_FLOW( 1, "button @ %d.%d in range id %x", env->e.rel_x, env->e.rel_y, cb->id );
        cb->mouse_in_bits |= 1;

        if(env->e.m.clicked & 0x1) // First button click only
        {
            cb->pressed_bits |= 1;

            ui_event_t e = env->e;

            e.type = UI_EVENT_TYPE_WIN;
            e.w.info = UI_EVENT_WIN_BUTTON;
            e.extra = cb->id;
            e.focus = env->w;

            ev_q_put_any( &e );

            if( cb->callback )
                cb->callback( env->w, handle, cb->callback_arg );
        }
    }

    return 0;
}







// --------------------------------------------------------
// Interface
// --------------------------------------------------------

pool_t *create_buttons_pool(void)
{
    pool_t *buttons;

    buttons = create_pool();
    buttons->flag_autoclean = 1;
    buttons->flag_autodestroy = 1;

    buttons->magic = BUTTONS_POOL_MAGIC;

    return buttons;
}

void destroy_buttons_pool(pool_t *buttons)
{
    assert( buttons->magic == BUTTONS_POOL_MAGIC );
    destroy_pool(buttons);
}


void w_paint_changed_buttons(window_handle_t w)
{
    if(w->buttons == 0)
        return;

    struct checkb env;

    //SHOW_FLOW( 1, "button check @ %d.%d buttons %x", x, y, mouseb );

    bzero( &env, sizeof(env) );

    env.w = w;

    pool_foreach( w->buttons, do_paint_changed_button, &env );
}

void w_repaint_buttons(window_handle_t w)
{
    if(w->buttons == 0)
        return;

    struct checkb env;

    //SHOW_FLOW( 1, "button check @ %d.%d buttons %x", x, y, mouseb );

    bzero( &env, sizeof(env) );

    env.w = w;

    pool_foreach( w->buttons, do_repaint_button, &env );
}

void w_reset_buttons(window_handle_t w) // focus lost, mouse off window - make sure all buttons are off
{
    if(w->buttons == 0)
        return;

    struct checkb env;
    bzero( &env, sizeof(env) );

    env.w = w;

    pool_foreach( w->buttons, do_reset_button, &env );
}

void w_check_button( window_handle_t w, ui_event_t *e )
{
    if(w->buttons == 0)
        return;

    struct checkb env;

    //SHOW_FLOW( 1, "button check @ %d.%d buttons %x", e->rel_x, e->rel_y, e->m.buttons );

    env.e = *e;
    env.w = w;

    assert( w->buttons->magic == BUTTONS_POOL_MAGIC );

    pool_foreach( w->buttons, do_check_button, &env );

    w_paint_changed_buttons(w);
}


pool_handle_t w_add_button( window_handle_t w, int id, int x, int y, drv_video_bitmap_t *bmp, drv_video_bitmap_t *pressed, int flags )
{
    // TODO take some mutex
    if(w->buttons == 0)
        w->buttons = create_buttons_pool();

    pool_handle_t bh = pool_create_el( w->buttons, calloc( 1, sizeof(button_t) ) );
    if( bh < 0 )
    {
        //SHOW_ERROR0( 0, "out of buttons" );
        return INVALID_POOL_HANDLE;
    }

    button_t *cb = pool_get_el( w->buttons, bh );

    if( !cb )
    {
        //SHOW_ERROR0( 0, "can't get just created button??" );
        return INVALID_POOL_HANDLE;
    }

    cb->id = id;

    cb->r.x = x;
    cb->r.y = y;

    cb->r.xsize = bmp->xsize;
    cb->r.ysize = bmp->ysize;

    cb->flags = flags;

    cb->color = COLOR_RED;
    cb->bmp = bmp;
    cb->bmp_pressed = pressed ? pressed : bmp;

    paint_button( w, cb );

    pool_release_el( w->buttons, bh );

    return bh;
}

/*
void w_delete_button( window_handle_t w, int id )
{
    if(w->buttons == 0)
        return;

    assert( w->buttons->magic == BUTTONS_POOL_MAGIC );

    pool_foreach( w->buttons, kill_button_by_id, (void *)id );
}
*/

void w_delete_button( window_handle_t w, pool_handle_t bh )
{
    pool_release_el( w->buttons, bh );
    pool_destroy_el( w->buttons, bh );
}



void w_button_set_text( window_handle_t w, pool_handle_t bh, const char *text, color_t text_color )
{
    if(w->buttons == 0)        return;
    assert( w->buttons->magic == BUTTONS_POOL_MAGIC );
    button_t *cb = pool_get_el( w->buttons, bh );

    if( !cb )
    {
        SHOW_ERROR0( 0, "can't get just button" );
        return;
    }

    const char *old = cb->text;
    cb->text = strdup(text);
    if( old ) free((void *)old);

    cb->text_color = text_color;
    paint_button( w, cb );

    pool_release_el( w->buttons, bh );
}


void w_button_set_callback( window_handle_t w, pool_handle_t button_handle, button_callback_t cb, void *arg )
{
    if(w->buttons == 0)        return;
    assert( w->buttons->magic == BUTTONS_POOL_MAGIC );
    button_t *b = pool_get_el( w->buttons, button_handle );

    if( !b )
    {
        SHOW_ERROR0( 0, "can't get just button" );
        return;
    }

    b->callback = cb;
    b->callback_arg = arg;

    pool_release_el( w->buttons, button_handle );
}



#endif
