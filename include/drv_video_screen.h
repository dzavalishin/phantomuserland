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

#ifndef DRV_VIDEO_SCREEN_H
#define DRV_VIDEO_SCREEN_H

#include <phantom_types.h>

// Windows 'screen' driver works in BGR format :(
#define BGR 1

#define VIDEO_ZBUF 0



typedef u_int8_t zbuf_t;
extern zbuf_t *zbuf;

void video_zbuf_init();
void video_zbuf_reset();
void video_zbuf_reset_square(int x, int y, int xsize, int ysize );
void video_zbuf_reset_square_z(int x, int y, int xsize, int ysize, u_int8_t zpos );
int video_zbuf_check( int linpos, u_int8_t zpos );









typedef struct rgb_t
{
#if BGR
    unsigned char       b;
    unsigned char       g;
    unsigned char       r;
#else
    unsigned char       r;
    unsigned char       g;
    unsigned char       b;
#endif
} rgb_t;


typedef struct rgba_t
{
#if BGR
    unsigned char       b;
    unsigned char       g;
    unsigned char       r;
#else
    unsigned char       r;
    unsigned char       g;
    unsigned char       b;
#endif
    unsigned char       a;      // Alpha
} rgba_t;




extern rgba_t COLOR_BLACK;
extern rgba_t COLOR_WHITE;

extern rgba_t COLOR_LIGHTGRAY;
extern rgba_t COLOR_DARKGRAY;
extern rgba_t COLOR_YELLOW;
extern rgba_t COLOR_LIGHTRED;
extern rgba_t COLOR_RED;
extern rgba_t COLOR_BROWN;
extern rgba_t COLOR_BLUE;
extern rgba_t COLOR_CYAN;
extern rgba_t COLOR_LIGHTBLUE;
extern rgba_t COLOR_GREEN;
extern rgba_t COLOR_LIGHTGREEN;
extern rgba_t COLOR_MAGENTA;
extern rgba_t COLOR_LIGHTMAGENTA;

typedef struct drv_video_bitmap
{
    int         	xsize;
    int 		ysize;
    rgba_t       pixel[];
} drv_video_bitmap_t;


typedef struct rect
{
    int x, y;
    int xsize, ysize;
} rect_t;


#define WIN_DECORATED           0x00000001

typedef struct drv_video_window
{
    int         	xsize; // physical
    int 		ysize;

    int                 x, y, z; // On screen

    int                 generation; // used to redraw self and borders on global events
    int                 flags;

    int                 li, ti, ri, bi; // insets

    rgba_t       	bg; // background color

    // bitmap itself
    rgba_t       	pixel[];
} drv_video_window_t;

// returns nonzero if rect is out of window
int rect_win_bounds( rect_t *r, drv_video_window_t *w );


typedef struct drv_video_font_t
{
    int         	xsize;
    int 		ysize;
    char *              font;
} drv_video_font_t;

extern struct drv_video_font_t         drv_video_16x16_font;
extern struct drv_video_font_t         drv_video_8x16ant_font;
extern struct drv_video_font_t         drv_video_8x16bro_font;
extern struct drv_video_font_t         drv_video_8x16cou_font;
extern struct drv_video_font_t         drv_video_8x16med_font;
extern struct drv_video_font_t         drv_video_8x16rom_font;
extern struct drv_video_font_t         drv_video_8x16san_font;
extern struct drv_video_font_t         drv_video_8x16scr_font;



// malloc value to create drv_video_window_t
static __inline__ int drv_video_window_bytes( int xsize, int ysize ) { return (sizeof(rgba_t) * xsize * ysize) + sizeof(drv_video_window_t); }
static __inline__ int drv_video_bitmap_bytes( int xsize, int ysize ) { return (sizeof(rgba_t) * xsize * ysize) + sizeof(drv_video_bitmap_t); }

void drv_video_window_update_generation(void);


drv_video_window_t *drv_video_window_create(int xsize, int ysize, int x, int y, rgba_t bg );
// for statically allocated ones
void drv_video_window_init( drv_video_window_t *w, int xsize, int ysize, int x, int y, rgba_t bg );
void drv_video_window_free(drv_video_window_t *w);


void    drv_video_window_clear( drv_video_window_t *win );
void    drv_video_window_fill( drv_video_window_t *win, rgba_t color );

void 	drv_video_window_fill_rect( drv_video_window_t *win, rgba_t color, rect_t r );

void 	drv_video_font_draw_string(
                                           drv_video_window_t *win,
                                           const struct drv_video_font_t *font,
                                           char *s, const rgba_t color,
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

#if VIDEO_ZBUF
    void 	(*bitblt) (const rgba_t *from, int xpos, int ypos, int xsize, int ysize, zbuf_t zpos);
    void 	(*winblt) (const drv_video_window_t *from, int xpos, int ypos, zbuf_t zpos);
#else
    void 	(*bitblt) (const rgba_t *from, int xpos, int ypos, int xsize, int ysize);
    void 	(*winblt) (const drv_video_window_t *from, int xpos, int ypos);
#endif
    void 	(*readblt) (const rgba_t *to, int xpos, int ypos, int xsize, int ysize);

    // Callbacks - to be filled by OS before driver init - BUG - kill!

    void        (*mouse)  (void); // mouse activity detected - callback from driver

    // Mouse cursor

    void        (*redraw_mouse_cursor)();
    void        (*set_mouse_cursor)(drv_video_bitmap_t *cursor);
    void        (*mouse_disable)();
    void        (*mouse_enable)();

};

extern struct drv_video_screen_t        drv_video_win32;

extern struct drv_video_screen_t        *video_drv;

extern void drv_video_null();


drv_video_bitmap_t *      drv_video_get_default_mouse_bmp();


#if VIDEO_ZBUF

extern void drv_video_bitblt_forw(const rgba_t *from, int xpos, int ypos, int xsize, int ysize, zbuf_t zpos );
extern void drv_video_bitblt_rev(const rgba_t *from, int xpos, int ypos, int xsize, int ysize, zbuf_t zpos );
extern void drv_video_win_winblt(const drv_video_window_t *from, int xpos, int ypos, zbuf_t zpos);
extern void drv_video_win_winblt_rev(const drv_video_window_t *from, int xpos, int ypos, zbuf_t zpos);

#else

extern void drv_video_bitblt_forw(const rgba_t *from, int xpos, int ypos, int xsize, int ysize);
extern void drv_video_bitblt_rev(const rgba_t *from, int xpos, int ypos, int xsize, int ysize);
extern void drv_video_win_winblt(const drv_video_window_t *from, int xpos, int ypos);
extern void drv_video_win_winblt_rev(const drv_video_window_t *from, int xpos, int ypos);

#endif

void drv_video_readblt_forw( rgba_t *to, int xpos, int ypos, int xsize, int ysize);
void drv_video_readblt_rev( rgba_t *to, int xpos, int ypos, int xsize, int ysize);


// RGB videospace access workers
//void drv_video_bitblt_worker(const rgba_t *from, int xpos, int ypos, int xsize, int ysize, int reverse);
void drv_video_bitblt_reader(rgba_t *to, int xpos, int ypos, int xsize, int ysize, int reverse);

#if VIDEO_ZBUF
void drv_video_bitblt_worker(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize, int reverse, zbuf_t zpos);
#else
void drv_video_bitblt_worker(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize, int reverse);
#endif





void rgba2rgb_move( struct rgb_t *dest, const rgba_t *src, int nelem );
void rgba2rgba_move( rgba_t *dest, const rgba_t *src, int nelem );
void rgb2rgba_move( rgba_t *dest, const struct rgb_t *src, int nelem );
void int565_to_rgba_move( rgba_t *dest, const short int *src, int nelem );

void rgba2rgba_replicate( rgba_t *dest, const rgba_t *src, int nelem );


void rgba2rgb_zbmove( struct rgb_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos  );
void rgb2rgba_zbmove( struct rgba_t *dest, const struct rgb_t *src, zbuf_t *zb, int nelem, zbuf_t zpos );
void rgba2rgba_zbmove( struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos );
void rgba2rgba_zbreplicate( struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos );
void int565_to_rgba_zbmove( struct rgba_t *dest, const short int *src, zbuf_t *zb, int nelem, zbuf_t zpos );



void bitmap2bitmap(
		rgba_t *dest, int destWidth, int destHeight, int destX, int destY,
		const rgba_t *src, int srcWidth, int srcHeight, int srcX, int srcY,
		int moveWidth, int moveHeight
);




// Default mouse implementation to be used by video drivers if no
// hardware impl exists
void drv_video_set_mouse_cursor_deflt(drv_video_bitmap_t *nc);
void drv_video_draw_mouse_deflt(void);
void drv_video_mouse_off_deflt(void);
void drv_video_mouse_on_deflt(void);



void drv_video_window_preblit( drv_video_window_t *w );



//void drv_video_bitblt(const char *from, int xpos, int ypos, int xsize, int ysize);
#define drv_video_update() video_drv->update()
#define drv_video_readblt(from, xpos, ypos, xsize,ysize) ( video_drv->mouse_disable(), video_drv->readblt(from, xpos, ypos, xsize,ysize), video_drv->mouse_enable() )

#if VIDEO_ZBUF
#define drv_video_bitblt(from, xpos, ypos, xsize, ysize, zpos)  ( video_drv->mouse_disable(), video_drv->bitblt(from, xpos, ypos, xsize, ysize, zpos), video_drv->mouse_enable() )
#define drv_video_winblt(from)                                  ( video_drv->mouse_disable(), drv_video_window_preblit(from), video_drv->winblt(from, (from)->x, (from)->y, (from)->z), video_drv->mouse_enable() )
#else
#define drv_video_bitblt(from, xpos, ypos, xsize,ysize)  ( video_drv->mouse_disable(), video_drv->bitblt(from, xpos, ypos, xsize,ysize), video_drv->mouse_enable() )
#define drv_video_winblt(from)                           ( video_drv->mouse_disable(), drv_video_window_preblit(from), video_drv->winblt(from, (from)->x, (from)->y), video_drv->mouse_enable() )
#endif

// These are special for mouse pointer code - they're not try to disable mouse
#define drv_video_readblt_ms(from, xpos, ypos, xsize,ysize) video_drv->readblt(from, xpos, ypos, xsize,ysize )
#if VIDEO_ZBUF
#define drv_video_bitblt_ms(from, xpos, ypos, xsize, ysize) video_drv->bitblt(from, xpos, ypos, xsize, ysize, 0xFF)
#else
#define drv_video_bitblt_ms(from, xpos, ypos, xsize, ysize) video_drv->bitblt(from, xpos, ypos, xsize, ysize )
#endif

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





#endif // DRV_VIDEO_SCREEN_H

