/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: yes (needs cleanup and, possibly, data structures modifiation)
 *
 *
**/

#include "drv_video_screen.h"
#include <assert.h>
#include <string.h>
//#include <mem.h>

static __inline__ int _bpc(const drv_video_font_t *font)
{
    return font->ysize * (1 + ((font->xsize-1) / 8));
}

__inline__ const char *drv_video_font_get_char( const drv_video_font_t *font, char c )
{
    //int bpc = font->ysize * (1 + ((font->xsize-1) / 8));
    return font->font + c * _bpc(font);
}

// Returns -1 if can't draw, 0 if done
__inline__ int drv_video_font_draw_char(
                                         drv_video_window_t *win,
                                         const drv_video_font_t *font,
                                         char c, rgba_t color,
                                         int x, int y )
{
    const char *fcp  = drv_video_font_get_char( font, c );

    int xafter = x+font->xsize;
    int yafter = y+font->ysize;

    int bpcx = 1 + ((font->xsize-1) / 8);

    if( xafter <= 0 || yafter <= 0
        || x >= win->xsize || y >= win->ysize )
        return -1; // Totally clipped off

    if( x < 0 || y < 0
        || xafter > win->xsize || yafter > win->ysize )
        return -1; // Partially clipped off - skip for now

    //color.a = 0xFF;
    //color.r = color.g = color.b = 0;

    // Completely visible

    int yc = y;
    int fyc = 0;
    for( ; yc < yafter; yc++, fyc++ )
    {
        rgba_t *wp = win->pixel + x + (yc*win->xsize);

        //wp[0] = wp[1] = wp[2] = wp[3] = color;

        const char *fp = fcp + ((font->ysize - fyc) * bpcx );
        int bc = bpcx;
        for(; bc >= 0; bc-- )
        {
            int xc = 0;
            int xe = (bc == 0) ? (font->xsize % 8) : 8;
            for( ; xc < xe; xc++ )
            {
                if( 0x80 & ((*fp) << xc) )
                    *wp = color;
                wp++;
            }
            fp++;
        }

    }

    return 0;
}



__inline__ void drv_video_font_draw_string(
                                           drv_video_window_t *win,
                                           const drv_video_font_t *font,
                                           char *s, const rgba_t color,
                                           int x, int y )
{
    int nc = strlen(s);

    while(nc--)
    {
        drv_video_font_draw_char( win, font, *s++, color, x, y );
        x += font->xsize;
    }
}

void 	drv_video_font_scroll_pixels( drv_video_window_t *win, int npix, rgba_t color)
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

void 	drv_video_font_scroll_line(
                                           drv_video_window_t *win,
                                           const drv_video_font_t *font, rgba_t color )
{
    drv_video_font_scroll_pixels( win, font->ysize, color );
}


__inline__ void drv_video_font_tty_string(
                                          drv_video_window_t *win,
                                          const drv_video_font_t *font,
                                          const char *s,
                                          const rgba_t color,
                                          const rgba_t back,
                                          int *x, int *y )
{
    int nc = strlen(s);
    //int startx = *x;

    while(nc--)
    {
        if( *s == '\n' )
        {
            if( *y <= font->ysize )
            {
                drv_video_font_scroll_line( win, font, back );
            }
            else
                *y -= font->ysize; // Step down a line
            *x = 0;
            s++;
            continue;
        }
        else if( *s == '\r' )
        {
            *x = 0;
            s++;
            continue;
        }
        else if( drv_video_font_draw_char( win, font, *s, color, *x, *y ) )
        {
            if( *y <= font->ysize )
            {
                drv_video_font_scroll_line( win, font, back );
            }
            else
                *y -= font->ysize; // Step down a line
            *x = 0;
            drv_video_font_draw_char( win, font, *s, color, *x, *y );
        }
        s++;
        *x += font->xsize;
    }

}





