/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Video subsystem public interfaces
 * 
 *
 *
**/

#ifndef VIDEO_H
#define VIDEO_H

#ifndef DRV_VIDEO_SCREEN_H
#include <drv_video_screen.h>
#endif // DRV_VIDEO_SCREEN_H

#include <phantom_types.h>
#include <queue.h>

int get_screen_xsize(void);
int get_screen_ysize(void);
int get_screen_bpp(void);

void w_request_async_repaint( rect_t *r );



// ------------------------------------------------------------------------
// Window interface - old, to be killed
// ------------------------------------------------------------------------

// malloc value to create drv_video_window_t
static __inline__ int drv_video_window_bytes( int xsize, int ysize ) { return (sizeof(rgba_t) * xsize * ysize) + sizeof(drv_video_window_t); }
static __inline__ int drv_video_bitmap_bytes( int xsize, int ysize ) { return (sizeof(rgba_t) * xsize * ysize) + sizeof(drv_video_bitmap_t); }


// dynamic allocation
drv_video_window_t *drv_video_window_create(int xsize, int ysize, int x, int y, rgba_t bg, const char* title );
// free dynamically allocated window
void 	drv_video_window_free(drv_video_window_t *w);

// init for statically allocated ones
void 	drv_video_window_init( drv_video_window_t *w, int xsize, int ysize, int x, int y, rgba_t bg );
// destroy for statically allocated ones
void 	drv_video_window_destroy(drv_video_window_t *w);



void 	drv_video_window_to_top(drv_video_window_t *w);
void 	drv_video_window_to_bottom(drv_video_window_t *w);


void    drv_video_window_clear( drv_video_window_t *win );
void    drv_video_window_fill( drv_video_window_t *win, rgba_t color );

void 	drv_video_window_fill_rect( drv_video_window_t *win, rgba_t color, rect_t r );

void	drv_video_window_pixel( drv_video_window_t *w, int x, int y, rgba_t color );


void    drv_video_window_draw_bitmap( drv_video_window_t *w, int x, int y, drv_video_bitmap_t *bmp );




void drv_video_window_draw_line( drv_video_window_t *w,
                                 int x1, int y1, int x2, int y2, rgba_t c);

void drv_video_window_fill_ellipse( drv_video_window_t *w,
                                 int x,int y,int lx,int ly,
                                 rgba_t c);

void drv_video_window_fill_box( drv_video_window_t *w,
                                 int x,int y,int lx,int ly,
                                 rgba_t c);

void drv_video_window_draw_box( drv_video_window_t *w,
                                 int x,int y,int lx,int ly,
                                 rgba_t c);


void drv_video_window_move( drv_video_window_t *w, int x, int y );

//! Warning - caller must check buffer size!
void drv_video_window_resize( drv_video_window_t *w, int xsize, int ysize );

void drv_video_window_set_title( drv_video_window_t *w,  const char *title );

void drv_video_window_get_bounds( drv_video_window_t *w, rect_t *out );



void 	drv_video_font_draw_string(
                                           drv_video_window_t *win,
                                           const drv_video_font_t *font,
                                           const char *s, const rgba_t color,
                                           int x, int y );
void 	drv_video_font_scroll_line(
                                           drv_video_window_t *win,
                                           const struct drv_video_font_t *font, rgba_t color );

void 	drv_video_font_scroll_pixels( drv_video_window_t *win, int npix, rgba_t color);

// returns new x position
void 	drv_video_font_tty_string(
                                          drv_video_window_t *win,
                                          const struct drv_video_font_t *font,
                                          const char *s,
                                          const rgba_t color,
                                          const rgba_t back,
                                          int *x, int *y );



// ------------------------------------------------------------------------
// Kernel video stuff
// ------------------------------------------------------------------------

// Used in window object restart func to re-enter window to OS all win q
void drv_video_window_enter_allwq( drv_video_window_t *w);


// Redirect console output to this window. TODO: input?
extern void phantom_init_console_window(void);
void phantom_stop_console_window(void);




// ------------------------------------------------------------------------
// pixel processing
// ------------------------------------------------------------------------


#endif // VIDEO_H

