/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system mouse on/off on paint.
 *
 *
**/

#define DEBUG_MSG_PREFIX "win"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <video/window.h>
#include <video/internal.h>
#include <video/screen.h>

// If mouse_intersects is used we loose mouse sooner or later. Usually sooner.

/*
static int mouse_intersects(struct drv_video_screen_t *video_drv, int xpos, int ypos, int xsize, int ysize )
{
    rect_t r;

    r.x = xpos;
    r.y = ypos;
    r.xsize = xsize;
    r.ysize = ysize;

    rect_t m;

    m.x = video_drv->mouse_x;
    m.y = video_drv->mouse_y;
    m.xsize = 16; // BUG mouse size hardcode
    m.ysize = 16;

    rect_t o;

    return rect_mul( &o, &r, &m );
}

*/


void scr_mouse_disable_p(struct drv_video_screen_t *video_drv, int xpos, int ypos, int xsize, int ysize )
{
//    if( mouse_intersects(video_drv, xpos, ypos, xsize, ysize ) )
        video_drv->mouse_disable();
}


void scr_mouse_enable_p(struct drv_video_screen_t *video_drv, int xpos, int ypos, int xsize, int ysize )
{
//    if( mouse_intersects(video_drv, xpos, ypos, xsize, ysize ) )
        video_drv->mouse_enable();
}

