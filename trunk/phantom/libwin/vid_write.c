/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 *
 *
**/

#include <video/vops.h>
#include <video/screen.h>
#include <assert.h>
#include <sys/types.h>
#include <kernel/boot.h>

#include "linestart.h"

/**
 *
 * From buffer to screen.
 *
 * Magic is that screen (0,0) is top left and window (0,0)
 * is bottom left.
 *
 **/

#if VIDEO_PARTIAL_WIN_BLIT

void vid_bitblt_worker(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize, int reverse, zbuf_t zpos, u_int32_t flags)
{
    vid_bitblt_part(from, xsize, ysize, 0, 0, xpos, ypos, xsize, ysize, reverse, zpos, flags);
}

#else

void vid_bitblt_worker(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize, int reverse, zbuf_t zpos)
{
    //printf("bit blt pos (%d,%d) size (%d,%d)\n", xpos, ypos, xsize, ysize);
    assert(video_drv->screen != 0);

    // we can't do it here - mouse itself uses us!
    //drv_video_mouse_off();

    int xafter = xpos+xsize;
    int yafter = ypos+ysize;

    if(
       xafter <= 0 || yafter <= 0 ||
       xpos >= video_drv->xsize || ypos >= video_drv->ysize
      )
        return; // Totally clipped off

    // Where to start in source line
    int xshift = (xpos < 0) ? -xpos : 0;

    // Which source line to start from
    int yshift = (ypos < 0) ? -ypos : 0;

    if( yshift > 0 )
    {
        // This one is easy candy

        //printf("yshift = %d\n", yshift );
        from += xsize*yshift; // Just skip some lines;
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

    // We took it in account
    if(xpos < 0) xpos = 0;

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
            const struct rgba_t *w_start = from + ((wline*xsize) + xshift);

            zbuf_t *zb = zbuf + ( (video_drv->xsize * ((video_drv->ysize-1) - sline)) + xpos);
            // ZBUF_TOP is a special value for mouse painting. XXX hack!
            if(zpos == ZBUF_TOP) bit_mover_to_screen( (void *)s_start, w_start, xlen );
            else bit_zbmover_to_screen( (void *)s_start, w_start, zb, xlen, zpos );
        }
    }
    else
    {
        for( ; sline < yafter; sline++, wline++ )
        {
            // Screen start pos in line
            char *s_start = DRV_VIDEO_FORWARD_LINESTART(sline) + xpos*bit_mover_byte_step;
            // Window start pos in line
            const struct rgba_t *w_start = from + ((wline*xsize) + xshift);

            //zbuf_t *zb = zbuf + ((wline*xsize) + xshift);
            zbuf_t *zb = zbuf + ( (video_drv->xsize * sline) + xpos);
            // ZBUF_TOP is a special value for mouse painting. XXX hack!
            if(zpos == ZBUF_TOP) bit_mover_to_screen( (void *)s_start, w_start, xlen );
            else bit_zbmover_to_screen( (void *)s_start, w_start, zb, xlen, zpos );
        }
    }

    scr_update();

    //drv_video_mouse_on();
}

#endif














// Put part of picture to screen
void vid_bitblt_part(const rgba_t *from, int src_xsize, int src_ysize, int src_xpos, int src_ypos, int dst_xpos, int dst_ypos, int xsize, int ysize, int reverse, zbuf_t zpos, u_int32_t flags )
{
    assert(video_drv->screen != 0);

    int dst_xafter = dst_xpos+xsize;
    int dst_yafter = dst_ypos+ysize;

    if( dst_xafter >= video_drv->xsize )
    {
        xsize -= (dst_xafter - video_drv->xsize);
        //dst_xafter = video_drv->xsize;
    }

    if( dst_yafter >= video_drv->ysize )
    {
        ysize -= (dst_yafter - video_drv->ysize);
        //dst_yafter = video_drv->ysize;
    }

    if( dst_xpos < 0 )
    {
        xsize -= (-dst_xpos);
        dst_xpos = 0;
        src_xpos += (-dst_xpos);
    }

    if( dst_ypos < 0 )
    {
        ysize -= (-dst_ypos);
        dst_ypos = 0;
        src_ypos += (-dst_ypos);
    }


    int src_xafter = src_xpos+xsize;
    int src_yafter = src_ypos+ysize;


    if( src_xafter >= src_xsize )
    {
        xsize -= (src_xafter - src_xsize);
        //src_xafter = src_xsize;
    }

    if( src_yafter >= src_ysize )
    {
        ysize -= (src_yafter - src_ysize);
        //src_yafter = src_ysize;
    }

    if( src_xpos < 0 )
    {
        xsize -= (-src_xpos);
        src_xpos = 0;
        dst_xpos += (-src_xpos);
    }

    if( src_ypos < 0 )
    {
        ysize -= (-src_ypos);
        src_ypos = 0;
        dst_ypos += (-src_ypos);
    }

    if( (xsize < 0) || (ysize < 0 ) )
        return; // totally clipped off

    if( src_ypos > 0 )
        from += src_xsize*src_ypos; // Just skip some lines


    // we possibly changed xsize - recalc both
    dst_xafter = dst_xpos+xsize;
    dst_yafter = dst_ypos+ysize;
    src_xafter = src_xpos+xsize;
    src_yafter = src_ypos+ysize;


    int wline = 0; // we skipped ypos lines already
    int sline = dst_ypos;

    int noalpha = (flags & BLT_FLAG_NOZBUF) && (flags & BLT_FLAG_NOALPHA);
    // ZBUF_TOP is a special value for mouse painting. XXX hack!
    int nozbuf = (flags & BLT_FLAG_NOZBUF) || (zpos == ZBUF_TOP);

    if(nozbuf)
        scr_zbuf_reset_square_z( dst_xpos, dst_ypos, xsize, ysize, zpos );

    if(reverse)
    {

        for( ; sline < dst_yafter; sline++, wline++ )
        {
            // Screen start pos in line
            char *s_start = DRV_VIDEO_REVERSE_LINESTART(sline) + dst_xpos*bit_mover_byte_step;
            // Window start pos in line
            const struct rgba_t *w_start = from + ((wline*src_xsize) + src_xpos);

            zbuf_t *zb = zbuf + ( (video_drv->xsize * ((video_drv->ysize-1) - sline)) + dst_xpos);
            if(noalpha)   	bit_mover_to_screen_noalpha( (void *)s_start, w_start, xsize );
            else if(nozbuf)  	bit_mover_to_screen( (void *)s_start, w_start, xsize );
            else         	bit_zbmover_to_screen( (void *)s_start, w_start, zb, xsize, zpos );
        }
    }
    else
    {
        for( ; sline < dst_yafter; sline++, wline++ )
        {
            // Screen start pos in line
            char *s_start = DRV_VIDEO_FORWARD_LINESTART(sline) + dst_xpos*bit_mover_byte_step;
            // Window start pos in line
            const struct rgba_t *w_start = from + ((wline*src_xsize) + src_xpos);

            zbuf_t *zb = zbuf + ( (video_drv->xsize * sline) + dst_xpos);

            if(noalpha)   	bit_mover_to_screen_noalpha( (void *)s_start, w_start, xsize );
            else if(nozbuf)   	bit_mover_to_screen( (void *)s_start, w_start, xsize );
            else         	bit_zbmover_to_screen( (void *)s_start, w_start, zb, xsize, zpos );
        }
    }

    scr_update();
}







