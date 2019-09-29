/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Phantom OS team
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Bitmap fonts.
 *
**/

#ifndef FONT_H
#define FONT_H

#ifndef VCONFIG_H
#include <video/vconfig.h>
#endif // VCONFIG_H

#include <video/window.h>

#define FONT_FLAG_NONE                 0
#define FONT_FLAG_PROPORTIONAL         (1<<0)

typedef struct drv_video_font_t
{
    int         xsize;
    int 		ysize;
    char *      font;
    int 		flags;
} drv_video_font_t;

extern struct drv_video_font_t         drv_video_16x16_font;
extern struct drv_video_font_t         drv_video_8x16ant_font;
extern struct drv_video_font_t         drv_video_8x16bro_font;
extern struct drv_video_font_t         drv_video_8x16cou_font;
extern struct drv_video_font_t         drv_video_8x16med_font;
extern struct drv_video_font_t         drv_video_8x16rom_font;
extern struct drv_video_font_t         drv_video_8x16san_font;
extern struct drv_video_font_t         drv_video_8x16scr_font;

extern struct drv_video_font_t         drv_video_kolibri1_font;
extern struct drv_video_font_t         drv_video_kolibri2_font;

extern struct drv_video_font_t         drv_video_gallant12x22_font;
extern struct drv_video_font_t         drv_video_freebsd_font;



// ------------------------------------------------------------------------
// Output
// ------------------------------------------------------------------------


void 	w_font_draw_string(
                                           window_handle_t win,
                                           const drv_video_font_t *font,
                                           const char *s, 
                                           const rgba_t color,
                                           const rgba_t bg,
                                           int x, int y );

void 	w_font_scroll_line(
                                           window_handle_t win,
                                           const drv_video_font_t *font, rgba_t color );

//void 	drv_video_font_scroll_pixels( drv_video_window_t *win, int npix, rgba_t color);

// returns new x position
void 	w_font_tty_string(
                                          drv_video_window_t *win,
                                          const struct drv_video_font_t *font,
                                          const char *s,
                                          const rgba_t color,
                                          const rgba_t back,
                                          int *x, int *y );


/// truetype fonts - in progress
void w_ttfont_draw_string(
                          window_handle_t win,
                          //const drv_video_font_t *font,
                          const char *s, const rgba_t color, const rgba_t bg,
                          int x, int y );




#if CONF_TRUETYPE

// ------------------------------------------------------------------------
// TrueType support
// ------------------------------------------------------------------------


//#include <ft2build.h>
//#include FT_FREETYPE_H


//extern FT_Library ftLibrary;




#endif // CONF_TRUETYPE



#endif // FONT_H
