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






#endif // VIDEO_H

