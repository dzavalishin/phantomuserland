/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Video subsystem - screen structure and ops.
 *
**/

#ifndef SCREEN_H
#define SCREEN_H

#ifndef VCONFIG_H
#include <video/vconfig.h>
#endif // VCONFIG_H

#include <errno.h>

#include <video/color.h>
#include <video/zbuf.h>
#include <video/bitmap.h>
#include <video/window.h>

/**
 *
 * Represents a videodriver.
 *
 */

struct drv_video_screen_t
{
    const char  *name; // Driver name

    int         xsize;
    int         ysize;
    //! Bits per pixel
    int         bpp;

    int         mouse_x;
    int         mouse_y;
    int         mouse_flags;

    char        *screen;

    // Probe/init/finish

    int         (*probe) (void); // Returns true if hardware present, sets xsize/ysize.
    int         (*start) (void); // Start driver, switch to graphics mode.
    errno_t     (*accel) (void); // Start driver in accelerating mode - video mode is already set by VESA, just add some acceleration to existing drv
    int         (*stop)  (void); // Stop driver, switch to text mode. Can be called in unstable kernel state, keep it simple.

    // Main interface

    void 	(*update) (void);

    void 	(*bitblt) (const rgba_t *from, int xpos, int ypos, int xsize, int ysize, zbuf_t zpos, u_int32_t flags);

#if VIDEO_DRV_WINBLT
#if NEW_WINDOWS
    void 	(*winblt) ( window_handle_t from, rect_t src, int src_stride, int dest_xpos, int dest_ypos, zbuf_t zpos);
#else
    void 	(*winblt) ( const window_handle_t from, int xpos, int ypos, zbuf_t zpos);
#endif
#endif

    void 	(*readblt) ( rgba_t *to, int xpos, int ypos, int xsize, int ysize);

    // Callbacks - to be filled by OS before driver init - BUG - kill!

    void        (*mouse)  (void); // mouse activity detected - callback from driver

    // Mouse cursor

    void        (*mouse_redraw_cursor)(void);
    void        (*mouse_set_cursor)(drv_video_bitmap_t *cursor);
    void        (*mouse_disable)(void);
    void        (*mouse_enable)(void);

    // Acceleration

    // Screen to screen copy
    void 	(*copy) (int src_xpos, int src_ypos, int dst_xpos, int dst_ypos, int xsize, int ysize );

    // Screen rect clear
    void 	(*clear) (int xpos, int ypos, int xsize, int ysize );

    void 	(*bitblt_part) (const rgba_t *from, int src_xsize, int src_ysize, int src_xpos, int src_ypos, int dst_xpos, int dst_ypos, int xsize, int ysize, zbuf_t zpos, u_int32_t flags );

};

#define VIDEO_PROBE_FAIL 0
// Complete driver
#define VIDEO_PROBE_SUCCESS 1
// Not complete driver, just accelerator (can't set mode)
#define VIDEO_PROBE_ACCEL 2 


// emulator video drivers
extern struct drv_video_screen_t        drv_video_win32;
extern struct drv_video_screen_t        drv_video_x11;

extern struct drv_video_screen_t        *video_drv;

extern void vid_null(void);



// Blitter can ignore z buffer
#define BLT_FLAG_NOZBUF         (1<<0)
// Blitter can ignore alpha byte
#define BLT_FLAG_NOALPHA        (1<<1)




int scr_get_xsize(void);
int scr_get_ysize(void);
int scr_get_bpp(void);


static inline void scr_get_rect( rect_t *r )
{
    assert(r);
    r->x = 0;
    r->y = 0;
    r->xsize = scr_get_xsize();
    r->ysize = scr_get_ysize();
}




extern void vid_bitblt_forw(const rgba_t *from, int xpos, int ypos, int xsize, int ysize, zbuf_t zpos, u_int32_t flags );
extern void vid_bitblt_rev(const rgba_t *from, int xpos, int ypos, int xsize, int ysize, zbuf_t zpos, u_int32_t flags );

void vid_bitblt_part_forw(const rgba_t *from, int src_xsize, int src_ysize, int src_xpos, int src_ypos, int dst_xpos, int dst_ypos, int xsize, int ysize, zbuf_t zpos, u_int32_t flags );
void vid_bitblt_part_rev(const rgba_t *from, int src_xsize, int src_ysize, int src_xpos, int src_ypos, int dst_xpos, int dst_ypos, int xsize, int ysize, zbuf_t zpos, u_int32_t flags );


#if !NEW_WINDOWS
extern void vid_win_winblt(const window_handle_t from, int xpos, int ypos, zbuf_t zpos);
extern void vid_win_winblt_rev(const window_handle_t from, int xpos, int ypos, zbuf_t zpos);
#endif

void vid_readblt_forw( rgba_t *to, int xpos, int ypos, int xsize, int ysize);
void vid_readblt_rev( rgba_t *to, int xpos, int ypos, int xsize, int ysize);


// RGB videospace access workers
void vid_bitblt_reader(rgba_t *to, int xpos, int ypos, int xsize, int ysize, int reverse);
void vid_bitblt_worker(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize, int reverse, zbuf_t zpos, u_int32_t flags);

//void vid_bitblt_part(const rgba_t *from, int src_xsize, int src_ysize, int src_xpos, int src_ypos, int dst_xpos, int dst_ypos, int xsize, int ysize, zbuf_t zpos);
void vid_bitblt_part(const rgba_t *from, int src_xsize, int src_ysize, int src_xpos, int src_ypos, int dst_xpos, int dst_ypos, int xsize, int ysize, int reverse, zbuf_t zpos, u_int32_t flags);



void scr_mouse_disable_p(struct drv_video_screen_t *video_drv, int xpos, int ypos, int xsize, int ysize );
void scr_mouse_enable_p(struct drv_video_screen_t *video_drv, int xpos, int ypos, int xsize, int ysize );


// -----------------------------------------------------------------------
// Mode switch
// -----------------------------------------------------------------------

//! Switch video bitblt functions, used to read/write videomem, to 32 bit mode. Default is 24 bit mode.
void switch_screen_bitblt_to_32bpp(int use32bpp );

// TODO call from text vga console drvr?
// Can be called from any driver (or anywhere) to reset VGA to text mode
extern void video_drv_basic_vga_set_text_mode(void);

void phantom_enforce_video_driver(struct drv_video_screen_t *vd);
void set_video_driver_bios_vesa_pa( physaddr_t pa, size_t size );
void set_video_driver_bios_vesa_mode( u_int16_t mode );


void setTextVideoMode(void); // Using int10
int setVesaMode( u_int16_t mode ); // Using int10


// -----------------------------------------------------------------------
// Drivers
// -----------------------------------------------------------------------

extern struct drv_video_screen_t        video_driver_bochs_vesa_emulator;
extern struct drv_video_screen_t        video_driver_basic_vga;
extern struct drv_video_screen_t        video_driver_direct_vesa;
extern struct drv_video_screen_t        video_driver_bios_vesa;
extern struct drv_video_screen_t        video_driver_cirrus;
extern struct drv_video_screen_t        video_driver_gen_clone;


// -----------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------

#define scr_update() video_drv->update()
#define scr_readblt(from, xpos, ypos, xsize,ysize) ( video_drv->mouse_disable(), video_drv->readblt(from, xpos, ypos, xsize,ysize), video_drv->mouse_enable() )


#define scr_bitblt(___from, xpos, ypos, xsize, ysize, zpos, flg)  ( \
    scr_mouse_disable_p(video_drv, xpos, ypos, xsize, ysize), \
    video_drv->bitblt((___from), xpos, ypos, xsize, ysize, zpos, flg), \
    scr_mouse_enable_p(video_drv, xpos, ypos, xsize, ysize ) )



// These are special for mouse pointer code - they're not trying to disable mouse
#define scr_readblt_ms(from, xpos, ypos, xsize,ysize) video_drv->readblt(from, xpos, ypos, xsize,ysize )
#define scr_bitblt_ms(from, xpos, ypos, xsize, ysize) video_drv->bitblt(from, xpos, ypos, xsize, ysize, ZBUF_TOP, 0)



#define scr_mouse_set_cursor(nc)        video_drv->mouse_set_cursor(nc)
#define scr_mouse_redraw()              video_drv->mouse_redraw_cursor()
#define scr_mouse_off()                 video_drv->mouse_disable()
#define scr_mouse_on()                  video_drv->mouse_enable()


// Convert y coordinate from Phantom style (grow up) to usual style (grow down)
//#define Y_PHANTOM_TO_HOST(y) (scr_get_ysize() - (y))
//#define Y_HOST_TO_PHANTOM(y) (scr_get_ysize() - (y))
#define Y_PHANTOM_TO_HOST(y) ((video_drv->ysize) - (y) - 1)
#define Y_HOST_TO_PHANTOM(y) ((video_drv->ysize) - (y) - 1)




// ------------------------------------------------------------------------
// Kernel video stuff
// ------------------------------------------------------------------------

// Used in window object restart func to re-enter window to OS all win q
//void drv_video_window_enter_allwq( drv_video_window_t *w );


// Redirect console output to this window. TODO: input?
extern void phantom_init_console_window(void);
void phantom_stop_console_window(void);





#endif // SCREEN_H

