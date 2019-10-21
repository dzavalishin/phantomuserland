/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * UI controls: Creation and deletion
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
#include <video/builtin_bitmaps.h>

#include "ctl_private.h"

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

#if 0 // crashes
static errno_t do_check_group(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    control_ref_t *ref = el;    assert(ref);
    control_t *cc = ref->c;     assert(cc);
    struct foreach_control_param *env = arg;   assert(env);

    LOG_FLOW( 5, "w %p", env->w );

    //control_group_t g
    if( cc->group && (env->gid == cc->group_id) ) 
    {
        env->g = cc->group;
        return 1; // Nonzero to stop search
    }

    return 0;
}

static errno_t do_set_group(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    control_ref_t *ref = el;    assert(ref);
    control_t *cc = ref->c;     assert(cc);
    struct foreach_control_param *env = arg;   assert(env);

    LOG_FLOW( 5, "w %p", env->w );

    if( env->gid == cc->group_id ) 
    {
        // Set group
        cc->group = env->g;
        // Link in list
        cc->next_in_group = env->c;
        env->c = cc;
    }

    return 0;
}


static void w_add_to_group( window_handle_t w, control_t *cc )
{
    cc->group = 0;
    cc->next_in_group = 0;

    if( !cc->group_id ) return;

    // TODO take some mutex

    //control_group_t *   group;          //< Group we belong, if any
    //struct control *    next_in_group;  //< linked list of controls in group - radio or menu

    //LOG_FLOW( 1, "button check @ %d.%d buttons %x", x, y, mouseb );
    struct foreach_control_param env;
    bzero( &env, sizeof(env) );
    env.w = w;
    env.g = 0;
    env.c = 0;
    env.gid = cc->group_id;

    // look for existing control_group_t
    pool_foreach( w->controls, do_check_group, &env );

    if( env.g == 0 )
    {
        // No other 
        env.g = calloc( 1, sizeof(control_group_t) );
        assert(env.g);
    }

    env.w = w;
    //env.g has object
    env.c = 0; // end of list
    env.gid = cc->group_id;
    
    // set and link
    pool_foreach( w->controls, do_set_group, &env );

    env.g->siblings = env.c; // start of list
    // TODO need test!
}

#endif    



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
    //cc->group = 0;
    //cc->next_in_group = 0;

    cc->w = w;
    cc->state = cs_released;
    cc->hovered = ch_normal;
    cc->focused = 0;        //< Selected in window
    cc->changed = 1;

    if( 0 == memcmp( &cc->bg_color, &((color_t){0, 0, 0, 0}), sizeof(color_t) ) )
        cc->bg_color = (color_t){ .r = 237, .g = 235, .b = 232, .a = 0xFF };

    memset( cc->buffer, 0, sizeof(cc->buffer) );

    if(cc->text)
    {
        strlcpy( cc->buffer, cc->text, sizeof(cc->buffer) ); // TODO if buffer is nou enough?
        cc->text = cc->buffer;
    }
}

control_handle_t w_restart_control_persistent( window_handle_t w, control_t *cc )
{

    //w_add_to_group(w,cc);
    w_image_defaults( w, cc );

    // Make sure caller will reassign
    cc->c_child = 0;
    cc->w_child = 0;
    cc->callback = 0;
    cc->callback_arg = 0;

    w_paint_control( w, cc );

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
    //w_add_to_group( w, cc );
    w_image_defaults( w, cc );

    w_paint_control( w, cc );

    pool_handle_t ch = control_to_pool( w, cc );
    if(INVALID_POOL_HANDLE != ch) pool_release_el( w->controls, ch );
    return ch;
}

/*
/// Same as w_add_control(), but you can pass list of controls linked by next_in_group field.
void w_add_controls( window_handle_t w, control_t *c )
{
    control_t *next = c->next_in_group; // w_add_conrtrol will use or clear it
    w_add_control( w, c );
    w_add_controls( w, next );
}*/

void w_clear_control( control_t *c )
{
    memset( c, 0, sizeof(control_t) );
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

control_handle_t w_add_label_ext( window_handle_t w, int x, int y, int xsize, int ysize, 
    const char *text, color_t text_color, 
    drv_video_bitmap_t *bg, uint32_t flags )
{
    control_t cb;
    w_clear_control( &cb );

    cb.type = ct_label;

    cb.r.x = x;
    cb.r.y = y;
    cb.r.xsize = xsize;
    cb.r.ysize = ysize;

    cb.flags = CONTROL_FLAG_NOBORDER|CONTROL_FLAG_NOFOCUS|flags;

    cb.text = text;
    cb.fg_color = text_color;

    cb.pas_bg_image = bg;
    //cb.pas_bg_image = &menu_normal_center_bmp;
    //cb.act_bg_image = &menu_selected_center_bmp;
    //cb.hov_bg_image = &menu_selected_center_bmp;

    return w_add_control( w, &cb );
}


control_handle_t w_add_label( window_handle_t w, int x, int y, int xsize, int ysize, const char *text, color_t text_color )
{
    return w_add_label_ext( w, x, y, xsize, ysize, text, text_color, &menu_normal_center_bmp, 0 );
}

control_handle_t w_add_label_transparent( window_handle_t w, int x, int y, int xsize, int ysize, const char *text, color_t text_color )
{
    return w_add_label_ext( w, x, y, xsize, ysize, text, text_color, 0, CONTROL_FLAG_NOBACKGROUND );
}





// TODO remove xy size, add backg pic
control_handle_t w_add_text_field( window_handle_t w, int x, int y, int xsize, int ysize, const char *text, color_t text_color )
{
    control_t cb;
    w_clear_control( &cb );

    cb.type = ct_text;

    cb.r.x = x;
    cb.r.y = y;
    cb.r.xsize = xsize;
    cb.r.ysize = ysize;

    cb.flags = CONTROL_FLAG_NOBORDER;

    cb.text = text;
    cb.fg_color = text_color;

    cb.str_len = strnlen( text, 1024 ); // TODO define and describe
    cb.vis_len = cb.str_len;

    cb.pas_bg_image = &text_field_x200_bmp;
    //cb.pas_bg_image = &menu_normal_center_bmp;
    //cb.act_bg_image = &menu_selected_center_bmp;
    //cb.hov_bg_image = &menu_selected_center_bmp;

    return w_add_control( w, &cb );
}


control_handle_t w_add_checkbox( window_handle_t w, int x, int y )
{
    control_handle_t bh = w_add_button( w, 0, x, y, 
            &checkbox_square_off_a_x30_bmp, &checkbox_square_on_a_x30_bmp, CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE );
    w_control_set_background( w, bh, &checkbox_square_off_a_x30_bmp, &checkbox_square_on_a_x30_bmp, 0 );
    return bh;
}



control_handle_t w_add_scrollbar_ext( window_handle_t w, int x, int y, int xsize, int ysize, int minval, int maxval, uint32_t flags )
{
    control_t cb;
    w_clear_control( &cb );

    cb.type = ct_scrollbar;

    cb.r.x = x;
    cb.r.y = y;
    cb.r.xsize = xsize;
    cb.r.ysize = ysize;

    cb.flags = flags|CONTROL_FLAG_NOBORDER;

    cb.minval = minval;
    cb.maxval = maxval;
    cb.value = maxval / 2;
    cb.value_width = (maxval-minval) / 4;

    return w_add_control( w, &cb );
}


control_handle_t w_add_scrollbar( window_handle_t w, int x, int y, int xsize, int ysize, int maxval )
{
    return w_add_scrollbar_ext( w, x, y, xsize, ysize, 0, maxval, 0 );
}



control_handle_t w_add_switch( window_handle_t w, int x, int y )
{
    control_handle_t ch = w_add_button( w, 0, x, y, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_on_bmp, CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE );
    w_control_set_background( w, ch, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_on_bmp, 0 );

    return ch;
}


// -----------------------------------------------------------------------
//
// Deletion
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


// -----------------------------------------------------------------------
//
// Menu window
//
// -----------------------------------------------------------------------


window_handle_t w_create_menu_window( int x, int y, int xsize, int ysize)
{
    window_handle_t menu = drv_video_window_create( 
        x, y, xsize, ysize,
        COLOR_WHITE, 
        "Menu", WFLAG_WIN_ONTOP|WFLAG_WIN_NOKEYFOCUS|WFLAG_WIN_HIDE_ON_FOCUS_LOSS );

    return menu;
}

