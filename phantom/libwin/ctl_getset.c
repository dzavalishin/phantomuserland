/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * UI controls: Getters and setters
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
#include <video/vops.h>

#include <video/control.h>


#define GET_CONTROL \
    if(w->controls == 0)        return;                   \
    assert( w->controls->magic == CONTROLS_POOL_MAGIC );  \
    control_ref_t *ref = pool_get_el( w->controls, ch );  \
                                                          \
    if( !ref )                                            \
    {                                                     \
        LOG_ERROR0( 1, "can't get control" );             \
        return;                                           \
    }                                                     \
                                                          \
    control_t *cc = ref->c;                               \
    assert(cc);                                           \


#define RELEASE_CONTROL pool_release_el( w->controls, ch );

// -----------------------------------------------------------------------
//
// Getters/Setters
//
// -----------------------------------------------------------------------


void w_control_set_visible( window_handle_t w, control_handle_t ch, int visible )
{
    GET_CONTROL

    cc->flags |= CONTROL_FLAG_DISABLED;
    if( visible ) cc->flags &= ~CONTROL_FLAG_DISABLED;    

    RELEASE_CONTROL
}


/**
 * 
 * Set children to be turned on/off on control state change.
 * 
 * 
 * 
 * 
**/
void w_control_set_children( window_handle_t w, control_handle_t ch, window_handle_t w_child, control_handle_t c_child )
{
    GET_CONTROL

    cc->c_child = c_child;
    cc->w_child = w_child;

    RELEASE_CONTROL
}


/**
 * 
 * Change flags for control.
 * 
 * \param[in] toSet Flags to set
 * \param[in] toReset Flags to clear
 * 
**/
void w_control_set_flags( window_handle_t w, control_handle_t ch, int toSet, int toReset )
{
    GET_CONTROL

    cc->flags |= toSet;
    cc->flags &= ~toReset;

    RELEASE_CONTROL
}



void w_control_set_icon( window_handle_t w, control_handle_t ch, drv_video_bitmap_t *icon )
{
    GET_CONTROL

    cc->icon_image = icon;
    w_paint_control( w, cc );

    RELEASE_CONTROL
}


void copy_and_blend( drv_video_bitmap_t **dst, uint32_t *alloc_flag, drv_video_bitmap_t *src, window_handle_t w, control_t *cc )
{
    drv_video_bitmap_t *bg;

    if( (*alloc_flag) && (*dst) )
    {
        free( *dst );
        *dst = 0;
        *alloc_flag = 0;
    }

    if( src == 0 ) goto verbatim;

    errno_t rc = w_duplicate_bitmap( &bg, src);
    if( rc )
    {
verbatim:
        *dst = src;
        *alloc_flag = 0;
        return;
    }
    else
    {
        w_blend_bg_to_bitmap( bg, w, cc->r.x, cc->r.y );
        *dst = bg;
        *alloc_flag = 1;
        return;
    }

}


void w_control_set_background( 
    window_handle_t w, control_handle_t ch, 
    drv_video_bitmap_t *normal,
    drv_video_bitmap_t *pressed,
    drv_video_bitmap_t *hover  )
{
    GET_CONTROL
    
    // TODO who deletes bitmaps?
/*
    rc = w_duplicate_bitmap( &bg, normal );
    if( rc )
        cc->pas_bg_image = normal;
    else
    {
        //w_blend_bg_to_bitmap( bg, w, cc->r.x, cc->r.y );
        cc->pas_bg_image = bg;
    }

    rc = w_duplicate_bitmap( &bg, pressed );
    assert(rc == 0);
    //w_blend_bg_to_bitmap( bg, w, cc->r.x, cc->r.y );
    cc->act_bg_image = bg;

    rc = w_duplicate_bitmap( &bg, hover );
    assert(rc == 0);
    //w_blend_bg_to_bitmap( bg, w, cc->r.x, cc->r.y );
    cc->hov_bg_image = bg;
*/

    copy_and_blend( &cc->pas_bg_image, &cc->pas_bg_alloc, normal, w, cc );
    copy_and_blend( &cc->act_bg_image, &cc->act_bg_alloc, pressed, w, cc );
    copy_and_blend( &cc->hov_bg_image, &cc->hov_bg_alloc, hover, w, cc );

    //cc->pas_bg_image = normal;
    //cc->act_bg_image = pressed,
    //cc->hov_bg_image = hover;

    // TODO w_image_defaults( w, cc ); ?
    w_paint_control( w, cc );
    
    RELEASE_CONTROL
}



void w_control_set_text( window_handle_t w, pool_handle_t ch, const char *text, color_t text_color )
{
    GET_CONTROL

    //const char *old = cc->text;
    //cc->text = strdup(text);
    //if( old ) free((void *)old);

    strlcpy( cc->buffer, text, sizeof(cc->buffer) ); // TODO wchar_t
    cc->text = cc->buffer;

    cc->fg_color = text_color;
    w_paint_control( w, cc );

    RELEASE_CONTROL
}

void w_control_get_text( window_handle_t w, control_handle_t ch, char *text_buf, size_t buf_size )
{
    GET_CONTROL    
    strlcpy( text_buf, cc->buffer, buf_size ); // TODO wchar_t
    w_paint_control( w, cc );
    RELEASE_CONTROL
}



void w_control_set_state( window_handle_t w, control_handle_t ch, int selected ) //< Is checkbox checked or switch turned on?
{
    GET_CONTROL
    cc->state = selected ? cs_pressed : cs_released;    
    w_paint_control( w, cc );
    RELEASE_CONTROL
}


void w_control_get_state( window_handle_t w, control_handle_t ch, int *ret ) //< Is checkbox checked or switch turned on?
{
    GET_CONTROL
    if( ret ) *ret = cc->state == cs_pressed;
    //w_paint_control( w, cc );
    RELEASE_CONTROL
}


/**
 * 
 * \brief Set callback.
 * 
 * Will be called on control state change or, if control has flags:
 * 
 * CONTROL_FLAG_CALLBACK_HOVER - Call callback on mouse over
 * CONTROL_FLAG_CALLBACK_KEY - Call callback on any key press
 * 
**/
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

    RELEASE_CONTROL
}
