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

#error I am deprecated

#ifndef VCONFIG_H
#include <video/vconfig.h>
#endif // VCONFIG_H

#include <kernel/pool.h>
#include <sys/types.h>
#include <stdint.h>

#include <video/control.h>

#define BUTTON_FLAG_NOPAINT   CONTROL_FLAG_NOPAINT
#define BUTTON_FLAG_NOBORDER  CONTROL_FLAG_NOBORDER

typedef void (*button_callback_t)( window_handle_t w, pool_handle_t button_handle, void *arg );

typedef struct
{
    int                 id; // kill me?

    rect_t              r;
    color_t             color;

    u_int32_t           flags;

    u_int32_t           mouse_in_bits;  // Rightmost bit is latest state. shifts left.
    u_int32_t           pressed_bits;   // Same

    drv_video_bitmap_t *bmp;
    drv_video_bitmap_t *bmp_pressed; // can be 0

    const char *        text;
    color_t             text_color;

    button_callback_t   callback;
    void *              callback_arg;
} button_t;


pool_t *create_buttons_pool(void);
void destroy_buttons_pool(pool_t *buttons);

pool_handle_t w_add_button( window_handle_t w, int id, int x, int y, drv_video_bitmap_t *bmp, drv_video_bitmap_t *pressed, int flags );
//void w_delete_button( window_handle_t w, int id );
void w_delete_button( window_handle_t w, pool_handle_t bh );

void w_button_set_text( window_handle_t w, pool_handle_t bh, const char *text, color_t text_color );


void w_paint_changed_buttons(window_handle_t w);
void w_repaint_buttons(window_handle_t w);
void w_reset_buttons(window_handle_t w); // focus lost, mouse off window - make sure all buttons are off
void w_check_button( window_handle_t w, ui_event_t *e );

void w_button_set_callback( window_handle_t w, pool_handle_t button_handle, button_callback_t cb, void *arg );


#endif // VIDEO_BUTTONS_H
