/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Default (software) mouse pointer drawing code. Used by drivers which have no mouse acceleration.
 *
**/

#include <stdlib.h>
#include <video/bitmap.h>
#include <video/screen.h>
#include <phantom_libc.h>

#define ms_hot_shift_x 0
//#define ms_hot_shift_y (mpointer->ysize)


static drv_video_bitmap_t *mpointer = 0;
static char mouse_is_on_screen = 0;
static drv_video_bitmap_t *screencopy = 0;
static int last_mouse_x = 0, last_mouse_y = 0;

static int mouse_on = 1;

static volatile int interlock = 0;

static void get_mouse()
{
    if(!mouse_is_on_screen) return;
    interlock++;
    scr_bitblt_ms(screencopy->pixel, last_mouse_x, last_mouse_y, mpointer->xsize, mpointer->ysize);
    mouse_is_on_screen = 0;
    interlock--;
}

static void put_mouse()
{
    if(mouse_on <= 0) return;
    if( mpointer == 0 ) return;

    if(mouse_is_on_screen) get_mouse();
    interlock++;

    int mx = video_drv->mouse_x - ms_hot_shift_x;
    int my = video_drv->mouse_y - mpointer->ysize;

    // TODO hardcoded mouse hot point
    scr_readblt_ms(screencopy->pixel, mx, my, mpointer->xsize, mpointer->ysize);
    scr_bitblt_ms(mpointer->pixel, mx, my, mpointer->xsize, mpointer->ysize);

    last_mouse_x = mx;
    last_mouse_y = my;
    mouse_is_on_screen = 1;
    interlock--;
}




void vid_mouse_draw_deflt()
{
    if( interlock || (mpointer == 0) ) return;
    put_mouse();
}

void vid_mouse_set_cursor_deflt(drv_video_bitmap_t *nc)
{
    if(screencopy) { free(screencopy); screencopy = 0; }
    mpointer = nc;
    if(mpointer)
        screencopy = malloc(drv_video_bitmap_bytes( nc->xsize, nc->ysize ) );
}

void vid_mouse_off_deflt()
{
    interlock++;
    mouse_on--;
    if(mouse_on == 0)
        get_mouse();
    interlock--;
}


void vid_mouse_on_deflt()
{
    interlock++;
    mouse_on++;
    if(mouse_on == 1)
        put_mouse();
    interlock--;
}

