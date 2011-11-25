/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Read pixels from screen
 *
**/

#include <video/vops.h>
#include <video/screen.h>
#include <assert.h>
#include <sys/types.h>

#include "linestart.h"


/**
 *
 * From screen to buffer.
 *
 * Magic is that screen (0,0) is top left and window (0,0)
 * is bottom left.
 *
 **/

void vid_bitblt_reader(struct rgba_t *to, int xpos, int ypos, int xsize, int ysize, int reverse)
{
    //printf("bit blt pos (%d,%d) size (%d,%d)\n", xpos, ypos, xsize, ysize);
    assert(video_drv->screen != 0);

    int xafter = xpos+xsize;
    int yafter = ypos+ysize;

    if( xafter <= 0 || yafter <= 0
        || xpos >= video_drv->xsize || ypos >= video_drv->ysize )
        return; // Totally clipped off

    // Where to start in source line
    int xshift = (xpos < 0) ? -xpos : 0;

    // Which source line to start from
    int yshift = (ypos < 0) ? -ypos : 0;

    if( yshift > 0 )
    {
        // This one is easy candy

        //printf("yshift = %d\n", yshift );
        to += xsize*yshift; // Just skip some lines;
        ysize -= yshift; // Less lines to go
        ypos += yshift;
        assert(ypos == 0);
        yshift = 0;
    }
    assert(yshift == 0);
    assert(xshift >= 0);


    //printf("xshift = %d\n", xshift );

    // xlen is how many pixels to move for each line
    int xlen = xsize;
    if( xafter > video_drv->xsize )
    {
        xlen -= (xafter - video_drv->xsize);
    }
    xlen -= xshift;
    assert(xlen > 0);
    assert(xlen <= xsize);
    assert(xlen <= video_drv->xsize);

    if( yafter > video_drv->ysize )
    {
        yafter = video_drv->ysize;
    }

    //char *lowest_line = ((drv_video_screen.ysize -1) - ypos) + drv_video_screen.screen;


    int sline = ypos;
    int wline = 0;


    //printf("xlen = %d, sline = %d yafter=%d \n", xlen, sline, yafter );

    if(reverse)
    {

        for( ; sline < yafter; sline++, wline++ )
        {
            // Screen start pos in line
            char *s_start = DRV_VIDEO_REVERSE_LINESTART(sline) + xpos*bit_mover_byte_step;
            // Window start pos in line
            struct rgba_t *w_start = to + ((wline*xsize) + xshift);

            //rgba2rgb_move( (void *)s_start, w_start, xlen );
            bit_mover_from_screen( w_start, (void *)s_start, xlen );
        }
    }
    else
    {
        for( ; sline < yafter; sline++, wline++ )
        {
            // Screen start pos in line
            char *s_start = DRV_VIDEO_FORWARD_LINESTART(sline) + xpos*bit_mover_byte_step;
            // Window start pos in line
            struct rgba_t *w_start = to + ((wline*xsize) + xshift);

            //rgba2rgb_move( (void *)s_start, w_start, xlen );
            bit_mover_from_screen( w_start, (void *)s_start, xlen );
        }
    }

}
