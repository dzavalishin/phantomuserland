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


// Find pointer to the line on screen. As addresses go from left to right in
// both screen and sys coords, this is the only strange calculation here.

// Zeroth line will have index of (scr y size - 1), right?

#define DRV_VIDEO_REVERSE_LINESTART(ypos) ( (video_drv->xsize * ((video_drv->ysize -1) - ypos) ) * bit_mover_byte_step + video_drv->screen)


#define DRV_VIDEO_FORWARD_LINESTART(ypos) ( (video_drv->xsize * ypos) * bit_mover_byte_step + video_drv->screen)


// movers. default to 24bpp
// void rgba2rgba_zbmove( struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos )
static void (*bit_zbmover_to_screen)( void *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos ) = (void *)rgba2rgb_zbmove;
static void (*bit_mover_to_screen)( void *dest, const struct rgba_t *src, int nelem ) = (void *)rgba2rgb_move;
static void (*bit_mover_from_screen)( struct rgba_t *dest, void *src, int nelem ) = (void *)rgb2rgba_move;
static int      bit_mover_byte_step = 3;


void switch_screen_bitblt_to_32bpp( int use32bpp )
{
    if(use32bpp)
    {
        bit_zbmover_to_screen = (void *)rgba2rgba_zbmove;
        bit_mover_to_screen   = (void *)rgba2rgba_move;
        bit_mover_from_screen = (void *)rgba2rgba_24_move;
#if defined(ARCH_ia32) && 0
        if(ARCH_HAS_FLAG(ARCH_IA32_SSE2))
        {
            bit_zbmover_to_screen = (void *)auto_rgba2rgba_zbmove;

            printf("use sse bitblt mover\n");
        }
#endif
        bit_mover_byte_step = 4;
        video_drv->bpp = 32;
    }
    else
    {
        bit_zbmover_to_screen = (void *)rgba2rgb_zbmove;
        bit_mover_to_screen   = (void *)rgba2rgb_move;
        bit_mover_from_screen = (void *)rgb2rgba_move;

        bit_mover_byte_step = 3;
        video_drv->bpp = 24;
    }
}

/**
 *
 * From buffer to screen.
 *
 * Magic is that screen (0,0) is top left and window (0,0)
 * is bottom left.
 *
 **/

#if 1

void drv_video_bitblt_worker(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize, int reverse, zbuf_t zpos)
{
    drv_video_bitblt_part(from, xsize, ysize, 0, 0, xpos, ypos, xsize, ysize, reverse, zpos);
}

#else

void drv_video_bitblt_worker(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize, int reverse, zbuf_t zpos)
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

    drv_video_update();

    //drv_video_mouse_on();
}

#endif














// Put part of picture to screen
void drv_video_bitblt_part(const rgba_t *from, int src_xsize, int src_ysize, int src_xpos, int src_ypos, int dst_xpos, int dst_ypos, int xsize, int ysize, int reverse, zbuf_t zpos)
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
    }

    if( dst_ypos < 0 )
    {
        ysize -= (-dst_ypos);
        dst_ypos = 0;
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
    }

    if( src_ypos < 0 )
    {
        ysize -= (-src_ypos);
        src_ypos = 0;
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


    //printf("xlen = %d, sline = %d dst_yafter=%d \n", xlen, sline, dst_yafter );

    if(reverse)
    {

        for( ; sline < dst_yafter; sline++, wline++ )
        {
            // Screen start pos in line
            char *s_start = DRV_VIDEO_REVERSE_LINESTART(sline) + dst_xpos*bit_mover_byte_step;
            // Window start pos in line
            const struct rgba_t *w_start = from + ((wline*src_xsize) + src_xpos);

            zbuf_t *zb = zbuf + ( (video_drv->xsize * ((video_drv->ysize-1) - sline)) + dst_xpos);
            // ZBUF_TOP is a special value for mouse painting. XXX hack!
            if(zpos == ZBUF_TOP) bit_mover_to_screen( (void *)s_start, w_start, xsize );
            else bit_zbmover_to_screen( (void *)s_start, w_start, zb, xsize, zpos );
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

            //zbuf_t *zb = zbuf + ((wline*xsize) + xshift);
            zbuf_t *zb = zbuf + ( (video_drv->xsize * sline) + dst_xpos);
            // ZBUF_TOP is a special value for mouse painting. XXX hack!
            if(zpos == ZBUF_TOP) bit_mover_to_screen( (void *)s_start, w_start, xsize );
            else bit_zbmover_to_screen( (void *)s_start, w_start, zb, xsize, zpos );
        }
    }
}



/**
 *
 * From screen to buffer.
 *
 * Magic is that screen (0,0) is top left and window (0,0)
 * is bottom left.
 *
 **/

void drv_video_bitblt_reader(struct rgba_t *to, int xpos, int ypos, int xsize, int ysize, int reverse)
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



















