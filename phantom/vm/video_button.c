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
#include <video/button.h>
//#include <video/vops.h>

// negative magic produces negative handles
//#define BUTTONS_POOL_MAGIC ('b' << 24 | 0xFFEEAA)
#define BUTTONS_POOL_MAGIC ('b')

struct checkb
{
    window_handle_t w;
    ui_event_t      e;
};




static void paint_button(window_handle_t win, button_t *cb)
{
    drv_video_window_fill_rect( win, cb->color, cb->r );

    if( cb->bmp && ! (cb->flags & BUTTON_FLAG_NOPAINT) )
    {
#if 1
        drv_video_window_draw_bitmap( win, cb->r.x, cb->r.y, cb->bmp );

#else
        bitmap2bitmap(
                            win->pixel, win->xsize, win->ysize, cb->r.x, cb->r.y,
                            cb->pixels, cb->r.xsize, cb->r.ysize, 0, 0, cb->r.xsize, cb->r.ysize );
#endif
    }

    if(! (cb->flags & BUTTON_FLAG_NOBORDER))
    {
        if( cb->mouse_in_bits & 1 )
            drv_video_window_draw_rect( win, COLOR_WHITE, cb->r );
    }

    //drv_video_window_update( win );
    event_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, win );
}

static void paint_changed_button(window_handle_t win, button_t *cb)
{
    if( (cb->mouse_in_bits & 1) == ( (cb->mouse_in_bits>>1) & 1 ) )
        return;

    paint_button( win, cb );
}

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



static errno_t do_check_button(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    button_t *cb = el;
    struct checkb *env = arg;

    cb->mouse_in_bits <<= 1;

    if( point_in_rect( env->e.rel_x, env->e.rel_y, &cb->r ) )
    {
        //SHOW_FLOW( 1, "button @ %d.%d in range id %x", env->e.rel_x, env->e.rel_y, cb->id );
        cb->mouse_in_bits |= 1;

        if(env->e.m.buttons)
        {
            ui_event_t e = env->e;

            e.type = UI_EVENT_TYPE_WIN;
            e.w.info = UI_EVENT_WIN_BUTTON;
            e.extra = cb->id;

            event_q_put_win( e.rel_x, e.rel_y, UI_EVENT_WIN_BUTTON, env->w );
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


void w_add_button( window_handle_t w, int id, int x, int y, drv_video_bitmap_t *bmp, int flags )
{
    // TODO take some mutex
    if(w->buttons == 0)
        w->buttons = create_buttons_pool();

    pool_handle_t bh = pool_create_el( w->buttons, calloc( 1, sizeof(button_t) ) );
    if( bh < 0 )
    {
        //SHOW_ERROR0( 0, "out of buttons" );
        return;
    }

    button_t *cb = pool_get_el( w->buttons, bh );

    if( !cb )
    {
        //SHOW_ERROR0( 0, "can't get just created button??" );
        return;
    }

    cb->id = id;

    cb->r.x = x;
    cb->r.y = y;

    cb->r.xsize = bmp->xsize;
    cb->r.ysize = bmp->ysize;

    cb->flags = flags;

    cb->color = COLOR_RED;
    cb->bmp = bmp;

    paint_button( w, cb );

    pool_release_el( w->buttons, bh );
}

void w_delete_button( window_handle_t w, int id )
{
    if(w->buttons == 0)
        return;

    assert( w->buttons->magic == BUTTONS_POOL_MAGIC );

    pool_foreach( w->buttons, kill_button_by_id, (void *)id );
}





