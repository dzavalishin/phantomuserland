/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Basic bitmap font output.
 *
 *
**/

#include <video/window.h>
//#include <video/internal.h>
#include <video/font.h>
#include <kernel/libkern.h>

#include <assert.h>
#include <string.h>
#include <phantom_libc.h>

static __inline__ int _bpc(const drv_video_font_t *font)
{
    return font->ysize * (1 + ((font->xsize-1) / 8));
}

__inline__ const char *drv_video_font_get_char( const drv_video_font_t *font, char c )
{
    //int bpc = font->ysize * (1 + ((font->xsize-1) / 8));
    const char *cp = font->font + c * _bpc(font);
    return cp;
}

//static void font_reverse_x(drv_video_font_t *font);


// Returns -1 if can't draw, 0 if done
__inline__ int w_font_draw_char(
                                         window_handle_t win,
                                         const drv_video_font_t *font,
                                         char c, rgba_t color, rgba_t bg,
                                         int x, int y )
{
    bool p = FONT_FLAG_PROPORTIONAL & font->flags;
    const char *fcp  = drv_video_font_get_char( font, c );

    int cwidth = font->xsize;
    if(p)
    {
        int pw = *fcp++;
        cwidth = umin(ffs(pw),font->xsize);
    }

    //if(font->xsize < 0)        font_reverse_x((drv_video_font_t *)font);

    int xafter = x+font->xsize;
    int yafter = y+font->ysize;

    // One char is used for width
    //if( p ) yafter--;

    int bpcx = 1 + ((font->xsize-1) / 8);

    if( xafter <= 0 || yafter <= 0
        || x >= win->xsize || y >= win->ysize )
        return -1; // Totally clipped off

    if( x < 0 || y < 0
        || xafter > win->xsize || yafter > win->ysize )
        return -1; // Partially clipped off - skip for now

    // Completely visible

    int yc = y;
    // One char is used for width
    int fyc = p ? 1 : 0;
    for( ; yc < yafter; yc++, fyc++ )
    {
        rgba_t *wp = win->w_pixel + x + (yc*win->xsize);

        const char *fp = fcp + ((font->ysize - fyc - 1) * bpcx );
        int bc = bpcx;
        if( font->xsize < 8 ) bc--; // hack!! colibri fonts
        for(; bc >= 0; bc-- )
        {
#if 0
            int xc = 0;
            for( ; xc < xe; xc++ )
            {
                if( 0x80 & ((*fp) << xc) )
                    *wp = color;
                wp++;
            }
            fp++;
#else
            int xe = (bc == 0) ? (font->xsize % 8) : 8; // bc==1 because we decrement it later, and we need last pass
            char fb = *fp++;
            //fb <<= (8-xe);
            while(xe--)
            {
                if( 0x80 & fb )
                    *wp = color;
                else
                    if( bg.a )
                        *wp = bg;
                wp++;
                fb <<= 1;
            }
#endif
        }

    }

    return 0;
}



void w_font_draw_string(
                                           window_handle_t win,
                                           const drv_video_font_t *font,
                                           const char *s, const rgba_t color, const rgba_t bg,
                                           int x, int y )
{
    //if(font->xsize < 0)        font_reverse_x((drv_video_font_t *)font);

    int nc = strlen(s);

    while(nc--)
    {
        w_font_draw_char( win, font, *s++, color, bg, x, y );
        x += font->xsize;
    }
}





void w_font_scroll_line( window_handle_t win, const drv_video_font_t *font, rgba_t color )
{
    w_scroll_up( win, font->ysize, color );
}


static color_t cmap[] =
{
    _C_COLOR_BLACK,
    _C_COLOR_LIGHTRED,
    _C_COLOR_LIGHTGREEN,
    _C_COLOR_YELLOW,
    _C_COLOR_LIGHTBLUE,
    _C_COLOR_MAGENTA,
    _C_COLOR_CYAN,
    _C_COLOR_LIGHTGRAY //_C_COLOR_WHITE
};

void w_font_tty_string(
                                          window_handle_t win,
                                          const drv_video_font_t *font,
                                          const char *s,
                                          const rgba_t _color,
                                          const rgba_t back,
                                          int *x, int *y )
{
    rgba_t color = _color;
    int nc = strlen(s);
    //int startx = *x;

    while(nc--)
    {
        if( *s == '\n' )
        {
            if( *y <= font->ysize )
            {
                w_font_scroll_line( win, font, back );
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
        else if( *s == '\t' )
        {
            (*x)++; // or else \t\t == \t
            while( *x % 8 )
                (*x)++;
            s++;
            continue;
        }
        else if( *s == 0x1B ) // esc
        {
            s++; nc--;
            if( *s != '[' )
                continue;
            s++; nc--;

            if( *s == 0 )
                continue;

            int v = 0;
            while( isdigit(*s) )
            {
                v *= 10;
                v += (*s - '0');
                s++; nc--;
            }

            if( *s == 0 )
                continue;

            switch(*s++)
            {
            case 'm':
                v -= 30;
                if( v > 7 )
                    continue;
                color = cmap[v];
            default:
                continue;
            }


        }
        else if( w_font_draw_char( win, font, *s, color, back, *x, *y ) )
        {
            if( *y <= font->ysize )
            {
                w_font_scroll_line( win, font, back );
            }
            else
                *y -= font->ysize; // Step down a line
            *x = 0;
            w_font_draw_char( win, font, *s, color, back, *x, *y );
        }
        s++;
        *x += font->xsize;
    }

}

/*
static void font_reverse_x(drv_video_font_t *font)
{
    if(font->xsize > 0) return;
    font->xsize = -font->xsize;
    //printf("reverse font x\n");

    int bpcx = 1 + ((font->xsize-1) / 8);
    int bcount = bpcx * font->ysize;

    char *fc = font->font;
    while(bcount--)
    {
        unsigned char ic = *fc;
        unsigned char oc = 0;

        int bitc = 8;
        while(bitc--)
        {
            unsigned char bit = ic & 1;
            if(bit)             oc |= 1;

            ic >>= 1;
            oc <<= 1;
        }
        *fc++ = oc;
    }
}

*/

