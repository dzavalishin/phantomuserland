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

    u_int32_t   mouse_in_bits;

    //int         npixels;
    //rgba_t      *pixels;

    drv_video_bitmap_t *bmp;

} button_t;


pool_t *create_buttons_pool(void);
void destroy_buttons_pool(pool_t *buttons);

void w_add_button( window_handle_t w, int id, int x, int y, drv_video_bitmap_t *bmp, int flags );
void w_delete_button( window_handle_t w, int id );

void w_paint_changed_buttons(window_handle_t w);
void w_repaint_buttons(window_handle_t w);
void w_check_button( window_handle_t w, ui_event_t *e );


#endif // VIDEO_BUTTONS_H
