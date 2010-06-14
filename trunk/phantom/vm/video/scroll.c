/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Scrolling of window/bitmap contents
 *
 * Kernel ready: yes
 *
**/



#include "drv_video_screen.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/libkern.h>



// Scrolls up only
void drv_video_font_scroll_pixels( drv_video_window_t *win, int npix, rgba_t color)
{
    assert(npix >= 0);
    //assert(npix <= win->ysize);

    if(npix >= win->ysize)
    {
        drv_video_window_fill( win, color );
        return;
    }

    struct rgba_t *dst = win->pixel + (win->xsize * npix);
    int len = win->xsize * (win->ysize - npix);
    memmove( dst, win->pixel, len*sizeof(struct rgba_t) );
    int clrlen = win->xsize * npix;
    dst = win->pixel;
    while( clrlen-- )
    {
        *dst++ = color;
    }
}




void video_fill_line( rgba_t *pixels, int xs, rgba_t bg )
{
    while(xs--)
        *pixels++ = bg;
}


// Scroll by s pixels horizontally, positive - right, negative - left
// xs/ys - size of piece to scroll
// lstep - pixels in full hor line (distance in pixels to next line)
void video_scroll_hor( rgba_t *pixels, int xs, int ys, int lstep, int s, rgba_t bg )
{
    int sabs = abs(s);

    /*
    if(sabs > xs)
    {
        video_fill_square( pixels, xs, ys, lstep );
        return;
    }*/

    int y;
    for( y = 0; y < ys; y++, pixels += lstep )
    {
        if(sabs > xs)
        {
            continue;
            video_fill_line( pixels, xs, bg );
        }

        if( s > 0 ) // right
        {
            memmove( pixels+sabs, pixels, sabs*sizeof( rgba_t ) );
            video_fill_line( pixels, sabs, bg );
        }
        else // left
        {
            memmove( pixels, pixels+sabs, sabs*sizeof( rgba_t ) );
            video_fill_line( pixels+xs-sabs, sabs, bg );
        }

    }
}


errno_t win_scroll_hor( drv_video_window_t *w, int x, int y, int xs, int ys, int s )
{
    if( x < 0 || y < 0 || x+xs > w->xsize || y+ys > w->ysize )
        return EINVAL;

    video_scroll_hor( w->pixel+x+(y*w->xsize), xs, ys, w->xsize, s, w->bg );
    return 0;
}




