/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * UI controls: Scroll bar
 *
**/

#define DEBUG_MSG_PREFIX "ui.ctl.txt"
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
#include <video/vops.h>

#include <video/builtin_bitmaps.h>

//#include <dev/key_event.h>

#include "ctl_private.h"

// TODO CONTROL_FLAG_READONLY, CONTROL_FLAG_ALT_FG, CONTROL_FLAG_ALT_BG


// -----------------------------------------------------------------------
//
// Local helpers
//
// -----------------------------------------------------------------------



// -----------------------------------------------------------------------
//
// Main code
//
// -----------------------------------------------------------------------


void ctl_scroll_bar_paint(window_handle_t w, control_t *cc )
{
    int fg_xgap = 4; // gap beween bed and bar itself
    int fg_xshift = 3; // shift beween bed and bar itself
    int fg_yshift = -3; // shift beween bed and bar itself

    //ctl_paint_bg( w, cc );    

    {
    int alt_bg = cc->flags & CONTROL_FLAG_ALT_BG;

    drv_video_bitmap_t *bg_m = alt_bg ? &scrollbar_bed_middle_bmp : &scrollbar_bed_middle_narrow_bmp;

    int bg_width = cc->r.xsize;
    int bg_lside_w = scrollbar_bed_left_bmp.xsize;
    int bg_rside_w = scrollbar_bed_right_bmp.xsize;
    int bg_mid_w = bg_width - bg_lside_w - bg_rside_w;
        
    if(bg_mid_w < 0) bg_mid_w = 0; // wrong - too small
    
    w_draw_bitmap( w, cc->r.x, cc->r.y, &scrollbar_bed_left_bmp );
    w_draw_bitmap( w, cc->r.x + bg_lside_w + bg_mid_w, cc->r.y, &scrollbar_bed_right_bmp );

    // Easy way - just replicate one column of bg
    //w_replicate_hor( w, cc->r.x + bg_lside_w, cc->r.y, bg_mid_w, bg_m->pixel, bg_m->ysize );
    // TODO replicate one of two BG pics, choose with CONTROL_FLAG_ALT_BG 
    int mid_cur_x = cc->r.x + bg_lside_w;
    while( bg_mid_w > 0 )
    {
        int step_w = bg_mid_w;
        if( step_w > bg_m->xsize )
            step_w = bg_m->xsize;

        if(step_w == bg_m->xsize)
            w_draw_bitmap( w, mid_cur_x, cc->r.y, bg_m ); // wrong, clip x size
        else
        {
            bitmap2bitmap(
                        w->w_pixel, w->xsize, w->ysize, mid_cur_x, cc->r.y,
                        bg_m->pixel, bg_m->xsize, bg_m->ysize, 0, 0,
                        step_w, bg_m->ysize
                        );            
        }
        

        bg_mid_w -= step_w;
        mid_cur_x += step_w;
    }

    }

    if( (cc->value >= 0  ) && (cc->value_width > 0) )
    {
    int alt_fg = cc->flags & CONTROL_FLAG_ALT_FG;

    drv_video_bitmap_t *fg_l = alt_fg ? &scrollbar_orange_bar_left_bmp : &scrollbar_green_bar_left_bmp;
    drv_video_bitmap_t *fg_r = alt_fg ? &scrollbar_orange_bar_right_bmp : &scrollbar_green_bar_right_bmp;
    drv_video_bitmap_t *fg_m = alt_fg ? &scrollbar_orange_bar_middle_bmp : &scrollbar_green_bar_middle_bmp;

    int fg_width = cc->r.xsize - fg_xgap * 2;
    int fg_lside_w = fg_l->xsize;
    int fg_rside_w = fg_r->xsize;
    int fg_mid_w_max = fg_width - fg_lside_w - fg_rside_w + 1; // dunno why +1, but it fits

    if(fg_mid_w_max < 0) fg_mid_w_max = 0;

    int valsize = cc->maxval - cc->minval;
    // recalc for current value
    // Pixels to shift right the bar for curr value
    int pos_shift = (cc->value - cc->minval) * fg_mid_w_max / valsize;
    // Pixels of bar width
    int fg_mid_w_actual = fg_mid_w_max * cc->value_width / valsize;

    // TODO blend
    w_draw_bitmap( w, cc->r.x + fg_xshift + pos_shift, cc->r.y + fg_yshift, fg_l );
    w_draw_bitmap( w, cc->r.x + fg_xshift + fg_lside_w + fg_mid_w_actual + pos_shift, cc->r.y + fg_yshift, fg_r );

    // TODO blend - make w_replicate_blend_hor
    // Replicate one column of fg
    w_replicate_hor( w, cc->r.x + fg_xshift + fg_lside_w + pos_shift, cc->r.y + fg_yshift, fg_mid_w_actual, fg_m->pixel, fg_m->ysize );

    // TODO only if controllable - no CONTROL_FLAG_READONLY
    // TODO on both sides of long bar?
    if( (!alt_fg) && !(cc->flags & (CONTROL_FLAG_READONLY|CONTROL_FLAG_NOFOCUS)) )
        w_draw_blend_bitmap( w, cc->r.x + fg_xshift + pos_shift + 3, cc->r.y + fg_yshift + 13, &scroll_bar_green_handle_bmp );
    }

    // Cursor
    //if(cc->focused || (cc->hovered == ch_hover))
    
    ctl_paint_border( w, cc );
    //LOG_FLOW( 10, "paint label id %d", cc->id );
}



/// Process event for text edit field
int ctl_scroll_bar_events(control_t *cc, struct foreach_control_param *env)
{
    ui_event_t e = env->e;

    // Not focused - ignore key events
    if( !cc->focused ) return 0;

    if( (e.type == UI_EVENT_TYPE_KEY) && UI_MOD_DOWN(e.modifiers) )
    {
    }

    return 0; // do not eat event
}

