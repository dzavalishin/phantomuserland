/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Video subsystem.
 *
**/

#ifndef DRV_VIDEO_SCREEN_H
#define DRV_VIDEO_SCREEN_H

#warning obsolete

#include <phantom_types.h>
#include <queue.h>

/**
 * \ingroup Video
 * \defgroup Video Graphics subsystem
 * @{
**/


#include <video/rect.h>
#include <video/font.h>
#include <video/color.h>
#include <video/bitmap.h>
#include <video/window.h>
#include <video/vops.h>
#include <video/zbuf.h>
//#include <video/screen.h>

#include <event.h>


#define VIDEO_ZBUF 1











#ifndef VIDEO_H
#include <video.h>
#endif // VIDEO_H


#endif // DRV_VIDEO_SCREEN_H

