/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Scrolling of bitmap contents
 *
**/



//#include "drv_video_screen.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/libkern.h>
#include <video/color.h>
//#include <video/internal.h>



void rgba_fill_line( rgba_t *pixels, int xs, rgba_t bg )
{
    while(xs--)
        *pixels++ = bg;
}


// Scroll by s pixels horizontally, positive - right, negative - left
// xs/ys - size of piece to scroll
// lstep - pixels in full hor line (distance in pixels to next line)
void rgba_scroll_hor( rgba_t *pixels, int xs, int ys, int lstep, int s, rgba_t bg )
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
            rgba_fill_line( pixels, xs, bg );
            continue;
        }

        if( s > 0 ) // right
        {
            memmove( pixels+sabs, pixels, sabs*sizeof( rgba_t ) );
            rgba_fill_line( pixels, sabs, bg );
        }
        else // left
        {
            memmove( pixels, pixels+sabs, sabs*sizeof( rgba_t ) );
            rgba_fill_line( pixels+xs-sabs, sabs, bg );
        }

    }
}




