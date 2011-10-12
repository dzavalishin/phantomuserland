/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Phantom OS team
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Bitmaps.
 *
**/

#ifndef BITMAP_H
#define BITMAP_H

#include <video/color.h>

typedef struct drv_video_bitmap
{
    int             xsize;
    int             ysize;
    rgba_t          pixel[];
} drv_video_bitmap_t;

typedef struct drv_video_cursor
{
    int             hotx;
    int             hoty;

    drv_video_bitmap_t bitmap;
} drv_video_cursor_t;



extern drv_video_bitmap_t 		close_bmp; // Window close button
extern drv_video_bitmap_t 		pin_bmp; // Window pin button

drv_video_bitmap_t *      drv_video_get_default_mouse_bmp(void);


#endif //BITMAP_H
