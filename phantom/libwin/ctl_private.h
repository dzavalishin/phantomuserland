/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Phantom OS team
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * UI controls private header.
 *
**/

#ifndef W_CONTROL_PRIVATE_H
#define W_CONTROL_PRIVATE_H

#ifndef VCONFIG_H
#include <video/vconfig.h>
#endif // VCONFIG_H


// -----------------------------------------------------------------------
//
// Used in pool_foreach( w->controls, ...
//
// -----------------------------------------------------------------------

struct foreach_control_param
{
    window_handle_t   w;
    ui_event_t        e;
    //control_group_t * g;   // used in w_add_to_group()
    int               gid; // ---""---
    control_t       * c;
    int               focus_flag;
    int               focus_success;
};



// -------------------------------------------------------------------
//
// Text field control specific processing
//
// -------------------------------------------------------------------


int ctl_text_field_events(control_t *cc, struct foreach_control_param *env);
void ctl_text_field_paint(window_handle_t win, control_t *cc );


// -------------------------------------------------------------------
//
// Button and menu item control specific processing
//
// -------------------------------------------------------------------

/// Process event for button or menu item
int ctl_button_events(control_t *cc, struct foreach_control_param *env);


// -------------------------------------------------------------------
//
// Scroll bar control specific processing
//
// -------------------------------------------------------------------

int ctl_scroll_bar_events(control_t *cc, struct foreach_control_param *env);
void ctl_scroll_bar_paint(window_handle_t w, control_t *cc );


// -------------------------------------------------------------------
//
// Bits to combine in control paint function
//
// -------------------------------------------------------------------

void ctl_paint_bg( window_handle_t win, control_t *cc );
void ctl_paint_border( window_handle_t win, control_t *cc );
void ctl_paint_text( window_handle_t win, control_t *cc, int shift );
/// \return X shift for text
int ctl_paint_icon( window_handle_t win, control_t *cc );

// -------------------------------------------------------------------
//
// Workers for different controls to use
//
// -------------------------------------------------------------------

/// Do assigned action - inform caller about control state change
void w_control_action(window_handle_t w, control_t *cc, ui_event_t *ie);

void ctl_img_copy_and_blend( drv_video_bitmap_t **dst, uint32_t *alloc_flag, drv_video_bitmap_t *src, window_handle_t w, control_t *cc );

//! Reset selected state for controls in group
void ctl_reset_group( window_handle_t w, int group_id );




#endif // W_CONTROL_PRIVATE_H
