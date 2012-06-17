/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Phantom OS team
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Buttons.
 *
**/

#ifndef VIDEO_BUTTONS_H
#define VIDEO_BUTTONS_H

#ifndef VCONFIG_H
#include <video/vconfig.h>
#endif // VCONFIG_H

#include <kernel/pool.h>
#include <sys/types.h>

#define BUTTON_FLAG_NOPAINT	(1<<0)
#define BUTTON_FLAG_NOBORDER	(1<<1)

typedef struct
{
    int         id;

    rect_t      r;
    color_t     color;

    u_int32_t   flags;

    u_int32_t   mouse_in_bits;  // Rightmost bit is latest state. shifts left.
    u_int32_t   pressed_bits;   // Same

    //int         npixels;
    //rgba_t      *pixels;

    drv_video_bitmap_t *bmp;
    drv_video_bitmap_t *bmp_pressed; // can be 0

    const char *text;
    color_t     text_color;

} button_t;


pool_t *create_buttons_pool(void);
void destroy_buttons_pool(pool_t *buttons);

pool_handle_t w_add_button( window_handle_t w, int id, int x, int y, drv_video_bitmap_t *bmp, drv_video_bitmap_t *pressed, int flags );
void w_delete_button( window_handle_t w, int id );

void w_button_set_text( window_handle_t w, pool_handle_t bh, const char *text, color_t text_color );


void w_paint_changed_buttons(window_handle_t w);
void w_repaint_buttons(window_handle_t w);
void w_reset_buttons(window_handle_t w); // focus lost, mouse off window - make sure all buttons are off
void w_check_button( window_handle_t w, ui_event_t *e );


#endif // VIDEO_BUTTONS_H
