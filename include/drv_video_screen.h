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

// Windows 'screen' driver works in BGR format :(

#define BGR 1

struct rgb_t
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
};


struct rgba_t
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
};

extern struct rgba_t COLOR_BLACK;
extern struct rgba_t COLOR_WHITE;

extern struct rgba_t COLOR_LIGHTGRAY;
extern struct rgba_t COLOR_DARKGRAY;
extern struct rgba_t COLOR_YELLOW;
extern struct rgba_t COLOR_LIGHTRED;
extern struct rgba_t COLOR_RED;
extern struct rgba_t COLOR_BROWN;
extern struct rgba_t COLOR_BLUE;
extern struct rgba_t COLOR_CYAN;
extern struct rgba_t COLOR_LIGHTBLUE;
extern struct rgba_t COLOR_GREEN;
extern struct rgba_t COLOR_LIGHTGREEN;
extern struct rgba_t COLOR_MAGENTA;
extern struct rgba_t COLOR_LIGHTMAGENTA;

struct drv_video_bitmap_t
{
    int         	xsize;
    int 		ysize;
    struct rgba_t       pixel[];
};


struct drv_video_window_t
{
    int         	xsize;
    int 		ysize;
    int                 x, y; // On screen
    struct rgba_t       pixel[];
};

struct drv_video_font_t
{
    int         	xsize;
    int 		ysize;
    char *              font;
};

extern struct drv_video_font_t         drv_video_16x16_font;
extern struct drv_video_font_t         drv_video_8x16ant_font;
extern struct drv_video_font_t         drv_video_8x16bro_font;
extern struct drv_video_font_t         drv_video_8x16cou_font;
extern struct drv_video_font_t         drv_video_8x16med_font;
extern struct drv_video_font_t         drv_video_8x16rom_font;
extern struct drv_video_font_t         drv_video_8x16san_font;
extern struct drv_video_font_t         drv_video_8x16scr_font;



// malloc value to create drv_video_window_t
static __inline__ int drv_video_window_bytes( int xsize, int ysize ) { return (sizeof(struct rgba_t) * xsize * ysize) + sizeof(struct drv_video_window_t); }
static __inline__ int drv_video_bitmap_bytes( int xsize, int ysize ) { return (sizeof(struct rgba_t) * xsize * ysize) + sizeof(struct drv_video_bitmap_t); }

void    drv_video_window_clear( struct drv_video_window_t *win );
void    drv_video_window_fill( struct drv_video_window_t *win, struct rgba_t color );

void 	drv_video_font_draw_string(
                                           struct drv_video_window_t *win,
                                           const struct drv_video_font_t *font,
                                           char *s, const struct rgba_t color,
                                           int x, int y );
void 	drv_video_font_scroll_line(
                                           struct drv_video_window_t *win,
                                           const struct drv_video_font_t *font, struct rgba_t color );

void 	drv_video_font_scroll_pixels( struct drv_video_window_t *win, int npix, struct rgba_t color);

// returns new x position
void 	drv_video_font_tty_string(
                                          struct drv_video_window_t *win,
                                          const struct drv_video_font_t *font,
                                          const char *s,
                                          const struct rgba_t color,
                                          const struct rgba_t back,
                                          int *x, int *y );

struct data_area_4_bitmap;

int drv_video_ppm_load( struct drv_video_bitmap_t **to, void *from );
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
    void 	(*bitblt) (const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize);
    void 	(*winblt) (const struct drv_video_window_t *from, int xpos, int ypos);
    void 	(*readblt) (const struct rgba_t *to, int xpos, int ypos, int xsize, int ysize);

    // Callbacks - to be filled by OS before driver init - BUG - kill!

    void        (*mouse)  (void); // mouse activity detected - callback from driver

    // Mouse cursor

    void        (*redraw_mouse_cursor)();
    void        (*set_mouse_cursor)(struct drv_video_bitmap_t *cursor);
    void        (*mouse_disable)();
    void        (*mouse_enable)();

};

extern struct drv_video_screen_t        drv_video_win32;

extern struct drv_video_screen_t        *video_drv;

extern void drv_video_null();


struct drv_video_bitmap_t *      drv_video_get_default_mouse_bmp();


extern void drv_video_bitblt_forw(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize);
extern void drv_video_bitblt_rev(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize);
extern void drv_video_win_winblt(const struct drv_video_window_t *from, int xpos, int ypos);
extern void drv_video_win_winblt_rev(const struct drv_video_window_t *from, int xpos, int ypos);
void drv_video_readblt_forw( struct rgba_t *to, int xpos, int ypos, int xsize, int ysize);
void drv_video_readblt_rev( struct rgba_t *to, int xpos, int ypos, int xsize, int ysize);


// RGB videospace access workers
void drv_video_bitblt_worker(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize, int reverse);
void drv_video_bitblt_reader(struct rgba_t *to, int xpos, int ypos, int xsize, int ysize, int reverse);





void rgba2rgb_move( struct rgb_t *dest, const struct rgba_t *src, int nelem );
void rgba2rgba_move( struct rgba_t *dest, const struct rgba_t *src, int nelem );
void rgb2rgba_move( struct rgba_t *dest, const struct rgb_t *src, int nelem );
void int565_to_rgba_move( struct rgba_t *dest, const short int *src, int nelem );


void bitmap2bitmap(
		struct rgba_t *dest, int destWidth, int destHeight, int destX, int destY,
		const struct rgba_t *src, int srcWidth, int srcHeight, int srcX, int srcY,
		int moveWidth, int moveHeight
);

// Default mouse implementation to be used by video drivers if no
// hardware impl exists
void drv_video_set_mouse_cursor_deflt(struct drv_video_bitmap_t *nc);
void drv_video_draw_mouse_deflt(void);
void drv_video_mouse_off_deflt(void);
void drv_video_mouse_on_deflt(void);







//void drv_video_bitblt(const char *from, int xpos, int ypos, int xsize, int ysize);
#define drv_video_update() video_drv->update()
#define drv_video_readblt(from, xpos, ypos, xsize,ysize) ( video_drv->mouse_disable(), video_drv->readblt(from, xpos, ypos, xsize,ysize), video_drv->mouse_enable() )
#define drv_video_bitblt(from, xpos, ypos, xsize,ysize)  ( video_drv->mouse_disable(),video_drv->bitblt(from, xpos, ypos, xsize,ysize), video_drv->mouse_enable() )
#define drv_video_winblt(from, xpos, ypos)               ( video_drv->mouse_disable(),video_drv->winblt(from, xpos, ypos), video_drv->mouse_enable() )

// These are special for mouse pointer code - they're not try to disable mouse
#define drv_video_readblt_ms(from, xpos, ypos, xsize,ysize) video_drv->readblt(from, xpos, ypos, xsize,ysize)
#define drv_video_bitblt_ms(from, xpos, ypos, xsize,ysize) video_drv->bitblt(from, xpos, ypos, xsize,ysize)


#define drv_video_set_mouse_cursor(nc) 		video_drv->set_mouse_cursor(nc)
#define drv_video_draw_mouse()            	video_drv->redraw_mouse_cursor()
#define drv_video_mouse_off()             	video_drv->mouse_disable()
#define drv_video_mouse_on()              	video_drv->mouse_enable()





#endif // DRV_VIDEO_SCREEN_H

