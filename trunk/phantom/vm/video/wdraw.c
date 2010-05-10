/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - painting.
 *
 *
**/


#include <drv_video_screen.h>
//#include <video.h>
#include <assert.h>
#include <phantom_libc.h>
#include <event.h>
#include <spinlock.h>




void
drv_video_window_fill( drv_video_window_t *win, rgba_t color )
{
    int i = (win->xsize * win->ysize) - 1;
    for( ; i >= 0; i-- )
        win->pixel[i] = color;
}


void
drv_video_window_clear( drv_video_window_t *win )
{
    drv_video_window_fill( win, COLOR_BLACK );
}


void
drv_video_window_fill_rect( drv_video_window_t *w, rgba_t color, rect_t r )
{
    if( rect_win_bounds( &r, w ) )
        return;

    int yp = r.y + r.ysize - 1;
    for( ; yp >= r.y; yp-- )
    {
        rgba_t *dst = w->pixel + yp*w->xsize + r.x;
        rgba2rgba_replicate( dst, &color, r.xsize );
    }
}


void
drv_video_window_draw_bitmap( drv_video_window_t *w, int x, int y, drv_video_bitmap_t *bmp )
{
    bitmap2bitmap(
                  w->pixel, w->xsize, w->ysize, x, y,
                  bmp->pixel, bmp->xsize, bmp->ysize, 0, 0,
                  bmp->xsize, bmp->ysize
                 );

}

