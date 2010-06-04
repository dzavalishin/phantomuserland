/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Video subsystem public interfaces
 * 
 *
 *
**/

#ifndef VIDEO_H
#define VIDEO_H

#ifndef DRV_VIDEO_SCREEN_H
#include <drv_video_screen.h>
#endif // DRV_VIDEO_SCREEN_H

#include <phantom_types.h>
#include <queue.h>


//! Convert HSI color to RGB one
rgba_t Hsi2Rgb(double H, double S, double I );


void drv_video_window_draw_line( drv_video_window_t *w,
                                 int x1, int y1, int x2, int y2, rgba_t c);

void drv_video_window_fill_ellipse( drv_video_window_t *w,
                                 int x,int y,int lx,int ly,
                                 rgba_t c);

void drv_video_window_fill_box( drv_video_window_t *w,
                                 int x,int y,int lx,int ly,
                                 rgba_t c);

void drv_video_window_draw_box( drv_video_window_t *w,
                                 int x,int y,int lx,int ly,
                                 rgba_t c);


void drv_video_window_move( drv_video_window_t *w, int x, int y );

void drv_video_window_get_bounds( drv_video_window_t *w, rect_t *out );


#endif // VIDEO_H

