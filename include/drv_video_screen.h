/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Video subsystem.
 *
**/

#ifndef DRV_VIDEO_SCREEN_H
#define DRV_VIDEO_SCREEN_H

#include <phantom_types.h>
#include <queue.h>

#include <video/rect.h>
#include <video/font.h>
#include <video/color.h>
#include <video/bitmap.h>
#include <video/window.h>
#include <video/vops.h>
#include <video/zbuf.h>

#include <event.h>

#define VIDEO_ZBUF 1





//! Switch video bitblt functions, used to read/write videomem, to 32 bit mode. Default is 24 bit mode.
void switch_screen_bitblt_to_32bpp(void);









extern drv_video_bitmap_t 		close_bmp; // Window close button
extern drv_video_bitmap_t 		pin_bmp; // Window pin button



// malloc value to create drv_video_window_t
static __inline__ int drv_video_window_bytes( int xsize, int ysize ) { return (sizeof(rgba_t) * xsize * ysize) + sizeof(drv_video_window_t); }
static __inline__ int drv_video_bitmap_bytes( int xsize, int ysize ) { return (sizeof(rgba_t) * xsize * ysize) + sizeof(drv_video_bitmap_t); }

void 	drv_video_window_repaint_all(void);



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

struct data_area_4_bitmap;

int drv_video_ppm_load( drv_video_bitmap_t **to, void *from );
int drv_video_string2bmp( struct data_area_4_bitmap *bmp, void *_from );


struct drv_video_screen_t
{
    const char  *name; // Driver name

    int         xsize;
    int 	ysize;
    int 	bpp;

    int 	mouse_x;
    int         mouse_y;
    int         mouse_flags;

    char        *screen;

    // Probe/init/finish

    int         (*probe) (void); // Returns true if hardware present, sets xsize/ysize.
    int         (*start) (void); // Start driver, switch to graphics mode.
    int         (*stop)  (void); // Stop driver, switch to text mode. Can be called in unstable kernel state, keep it simple.

    // Main interface

    void 	(*update) (void);

//#if VIDEO_ZBUF
    void 	(*bitblt) (const rgba_t *from, int xpos, int ypos, int xsize, int ysize, zbuf_t zpos);
    void 	(*winblt) (const drv_video_window_t *from, int xpos, int ypos, zbuf_t zpos);
//#else
//    void 	(*bitblt) (const rgba_t *from, int xpos, int ypos, int xsize, int ysize);
//    void 	(*winblt) (const drv_video_window_t *from, int xpos, int ypos);
//#endif
    void 	(*readblt) ( rgba_t *to, int xpos, int ypos, int xsize, int ysize);

    // Callbacks - to be filled by OS before driver init - BUG - kill!

    void        (*mouse)  (void); // mouse activity detected - callback from driver

    // Mouse cursor

    void        (*redraw_mouse_cursor)(void);
    void        (*set_mouse_cursor)(drv_video_bitmap_t *cursor);
    void        (*mouse_disable)(void);
    void        (*mouse_enable)(void);

};

extern struct drv_video_screen_t        drv_video_win32;

extern struct drv_video_screen_t        *video_drv;

extern void drv_video_null(void);


drv_video_bitmap_t *      drv_video_get_default_mouse_bmp(void);


extern void drv_video_bitblt_forw(const rgba_t *from, int xpos, int ypos, int xsize, int ysize, zbuf_t zpos );
extern void drv_video_bitblt_rev(const rgba_t *from, int xpos, int ypos, int xsize, int ysize, zbuf_t zpos );
extern void drv_video_win_winblt(const drv_video_window_t *from, int xpos, int ypos, zbuf_t zpos);
extern void drv_video_win_winblt_rev(const drv_video_window_t *from, int xpos, int ypos, zbuf_t zpos);


void drv_video_readblt_forw( rgba_t *to, int xpos, int ypos, int xsize, int ysize);
void drv_video_readblt_rev( rgba_t *to, int xpos, int ypos, int xsize, int ysize);


// RGB videospace access workers
void drv_video_bitblt_reader(rgba_t *to, int xpos, int ypos, int xsize, int ysize, int reverse);
void drv_video_bitblt_worker(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize, int reverse, zbuf_t zpos);









// Default mouse implementation to be used by video drivers if no
// hardware impl exists
void drv_video_set_mouse_cursor_deflt(drv_video_bitmap_t *nc);
void drv_video_draw_mouse_deflt(void);
void drv_video_mouse_off_deflt(void);
void drv_video_mouse_on_deflt(void);



void drv_video_window_preblit( drv_video_window_t *w ) __attribute__((__deprecated__));


void mouse_disable_p(struct drv_video_screen_t *video_drv, int xpos, int ypos, int xsize, int ysize );
void mouse_enable_p(struct drv_video_screen_t *video_drv, int xpos, int ypos, int xsize, int ysize );

#define drv_video_update() video_drv->update()
#define drv_video_readblt(from, xpos, ypos, xsize,ysize) ( video_drv->mouse_disable(), video_drv->readblt(from, xpos, ypos, xsize,ysize), video_drv->mouse_enable() )


#define drv_video_bitblt(from, xpos, ypos, xsize, ysize, zpos)  ( \
    mouse_disable_p(video_drv, xpos, ypos, xsize, ysize), \
    video_drv->bitblt(from, xpos, ypos, xsize, ysize, zpos), \
    mouse_enable_p(video_drv, xpos, ypos, xsize, ysize ) )

#define drv_video_winblt(from)                                  ( \
    mouse_disable_p(video_drv, (from)->x, (from)->y, (from)->xsize, (from)->ysize ), \
    video_drv->winblt(from, (from)->x, (from)->y, (from)->z), \
    mouse_enable_p(video_drv, (from)->x, (from)->y, (from)->xsize, (from)->ysize ) )


// These are special for mouse pointer code - they're not try to disable mouse
#define drv_video_readblt_ms(from, xpos, ypos, xsize,ysize) video_drv->readblt(from, xpos, ypos, xsize,ysize )

#define drv_video_bitblt_ms(from, xpos, ypos, xsize, ysize) video_drv->bitblt(from, xpos, ypos, xsize, ysize, 0xFF)

#define drv_video_set_mouse_cursor(nc) 		video_drv->set_mouse_cursor(nc)
#define drv_video_draw_mouse()            	video_drv->redraw_mouse_cursor()
#define drv_video_mouse_off()             	video_drv->mouse_disable()
#define drv_video_mouse_on()              	video_drv->mouse_enable()






/**
 *
 * Replicates src to dest. src has one horiz. line of srcSize pixels.
 * nSteps is number of replication steps vertically.
 *
**/

void replicate2window_ver( drv_video_window_t *dest, int destX, int destY,
                       int nSteps, const rgba_t *src, int srcSize );


/**
 *
 * Replicates src to dest. src has one vert. line of srcSize pixels.
 * nSteps is number of times to draw src horizontally.
 *
**/

void replicate2window_hor( drv_video_window_t *dest, int destX, int destY,
                       int nSteps, const rgba_t *src, int srcSize );


void window_basic_border( drv_video_window_t *dest, const rgba_t *src, int srcSize );




void video_zbuf_init(void);
void video_zbuf_reset(void);
void video_zbuf_reset_square(int x, int y, int xsize, int ysize );
void video_zbuf_reset_square_z(int x, int y, int xsize, int ysize, u_int8_t zpos );
void video_zbuf_reset_win( drv_video_window_t *w );

int video_zbuf_check( int linpos, u_int8_t zpos );

// Set z order for window according to its position in all win q
void drv_video_window_rezorder_all(void);



#ifndef VIDEO_H
#include <video.h>
#endif // VIDEO_H


#endif // DRV_VIDEO_SCREEN_H

