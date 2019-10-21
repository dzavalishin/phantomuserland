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

// Kills kernel - needs big stack (256 kb or more), calls malloc - must
// be called out of spinlock, trapno 0: Divide error, error 00000000 EIP 1f5ae0: _gray_render_line

#define TTF_TTY 0

//! Bytes per char
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
    // font char ptr
    const char *fcp  = drv_video_font_get_char( font, c );

#if 0
    // char width
    int cwidth = font->xsize;
    if(p)
    {
        int pw = *fcp++;
        cwidth = umin(ffs(pw),font->xsize);
    }
#endif
    //if(font->xsize < 0)        font_reverse_x((drv_video_font_t *)font);

    int xafter = x+font->xsize;
    int yafter = y+font->ysize;

    // One char is used for width
    //if( p ) yafter--;

    int bpcx = 1 + ((font->xsize-1) / 8); // Bytes per Character for X coord (for example, 2 bytes for 16 pixels wide)

    if( xafter <= 0 || yafter <= 0
        || x >= win->xsize || y >= win->ysize )
        return -1; // Totally clipped off

    if( x < 0 || y < 0
        || xafter >= win->xsize || yafter >= win->ysize )
        return -1; // Partially clipped off - skip for now

    // Completely visible

    int yc = y; // (screen) Y Coord
    // One char is used for width
    int fyc = p ? 1 : 0; // Font Y Coord
    for( ; yc < yafter; yc++, fyc++ )
    {
        // Write pointer
        rgba_t *wp = win->w_pixel + x + (yc*win->xsize);

        // Font pointer
        const char *fp = fcp + ((font->ysize - fyc - 1) * bpcx );

        int bc = bpcx; // Byte Count (bytes to put per line)
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
            char fb = *fp++; // Font Byte
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
    _C_COLOR_BLACK,       // 0
    _C_COLOR_LIGHTRED,
    _C_COLOR_LIGHTGREEN,  // 2
    _C_COLOR_YELLOW,
    _C_COLOR_LIGHTBLUE,   // 4
    _C_COLOR_MAGENTA,
    _C_COLOR_CYAN,        // 6
    _C_COLOR_LIGHTGRAY //_C_COLOR_WHITE
};

static void set_ansi_color( int v, int *bright, rgba_t *fg, rgba_t *bg, rgba_t def_fg, rgba_t def_bg )
{
    // Special cases
    switch( v )
    {
    case 0: // defaults
        *fg = def_fg;
        *bg = def_bg;
        return;

    case 7: // reverse video
        *fg = def_bg;
        *bg = def_fg;
        return;

    case 1: // bold/bright
    case 3: // standout
        *bright = 1;
        return;

    }

    // FG
    if( (v >= 30) && (v <= 39) )
    {
        v -= 30;

        if( v > 7 )
        {
            *fg = def_fg;
            return;
        }
        else
        {
            *fg = cmap[v];
            return;
        }
    }

    // BG
    if( (v >= 40) && (v <= 49) )
    {
        v -= 40;

        if( v > 7 )
        {
            *bg = def_bg;
            return;
        }
        else
        {
            *bg = cmap[v];
            return;
        }
    }
}



void w_font_tty_string(
                                          window_handle_t win,
                                          const drv_video_font_t *font,
                                          const char *s,
                                          const rgba_t _color,
                                          const rgba_t _back,
                                          int *x, int *y )
{
    rgba_t color = _color;
    rgba_t back  = _back;
    //int nc = strlen(s);
    int bright = 0;
    //int startx = *x;

#if TTF_TTY
    // temp
    font_handle_t ttfont = w_get_system_mono_font();

    unsigned utf_getc()
    {
        return 0;
    }
#endif

    for( ; *s ; s++ )
    {
        if( *s == '\n' )
        {
            if( *y <= font->ysize )
                w_font_scroll_line( win, font, back );
            else
                *y -= font->ysize; // Step down a line
            *x = 0;
            continue;
        }
        else if( *s == '\r' )
        {
            *x = 0;
            continue;
        }
        else if( *s == '\t' )
        {
            (*x)++; // or else \t\t == \t
            while( *x % 8 )
                (*x)++;
            continue;
        }
        else if( *s == 0x1B ) // esc
        {

            s++; 
            // checks for '*s == 0' too
            if( *s != '[' )                continue;

            s++; 
            if( *s == 0 )                  continue;
            int v = 0;
            while( isdigit(*s) )
            {
                v *= 10;
                v += (*s - '0');
                s++; 
            }

            if( *s == 0 )                continue;
            switch(*s)
            {
            case 'm':  
                set_ansi_color( v, &bright, &color, &back, _color, _back );
                continue;

            default:
                continue;
            }

        }
#if TTF_TTY
        if( (*x + 8 > win->xsize) && (*y <= 16) )
        {
            w_scroll_up( win, 16, back );
            *y -= 16; // Step down a line
            *x = 0;
        }

        w_ttfont_draw_char( win, ttfont, s, color, *x, *y );
        *x += 8;

#else

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
        *x += font->xsize;
#endif
    }

#if TTF_TTY
    w_release_tt_font( ttfont );
#endif
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


/* TODO

Set Attribute Mode	<ESC>[{attr1};...;{attrn}m

    Sets multiple display attribute settings. The following lists standard attributes:

    0	Reset all attributes
    1	Bright
    2	Dim
    4	Underscore	
    5	Blink
    7	Reverse
    8	Hidden

    22 normal
    23 no-standout
    24 no-underline
    25 no-blink
    27 no-reverse

    	Foreground Colours
    30	Black
    31	Red
    32	Green
    33	Yellow
    34	Blue
    35	Magenta
    36	Cyan
    37	White

    39 default foreground

    	Background Colours
    40	Black
    41	Red
    42	Green
    43	Yellow
    44	Blue
    45	Magenta
    46	Cyan
    47	White

    49 default background

Cursor Controls:
    ESC[#;#H or ESC[#;#f Moves cusor to line #, column #
    ESC[#A Moves cursor up # lines
    ESC[#B Moves cursor down # lines
    ESC[#C Moves cursor forward # spaces
    ESC[#D Moves cursor back # spaces
    ESC[#;#R Reports current cursor line & column
    ESC[s Saves cursor position for recall later
    ESC[u Return to saved cursor position


    ESC[2J Clear screen and home cursor
    ESC[K Clear to end of line


    ESC[ ... 38;2;<r>;<g>;<b> ... m Select RGB foreground color
    ESC[ ... 48;2;<r>;<g>;<b> ... m Select RGB background color

*/

