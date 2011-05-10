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


//! Convert HSI color to RGB one
rgba_t Hsi2Rgb(double H, double S, double I );


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
// ------------------------------------------------------------------------
// Kernel video stuff
// ------------------------------------------------------------------------

// Used in window object restart func to re-enter window to OS all win q
void drv_video_window_enter_allwq( drv_video_window_t *w);




extern struct drv_video_screen_t        video_driver_bochs_vesa_emulator;
extern struct drv_video_screen_t        video_driver_basic_vga;
extern struct drv_video_screen_t        video_driver_direct_vesa;
extern struct drv_video_screen_t        video_driver_bios_vesa;
extern struct drv_video_screen_t        video_driver_cirrus;
extern struct drv_video_screen_t        video_driver_gen_clone;


// TODO call from text vga console drvr?
// Can be called from any driver (or anywhere) to reset VGA to text mode
extern void video_drv_basic_vga_set_text_mode(void);

// Redirect console output to this window. TODO: input?
extern void phantom_init_console_window(void);
void phantom_stop_console_window(void);


void phantom_enforce_video_driver(struct drv_video_screen_t *vd);
void set_video_driver_bios_vesa_pa( physaddr_t pa, size_t size );
void set_video_driver_bios_vesa_mode( u_int16_t mode );

// If 1, VESA will be used if found, even if other driver is found
// If 0, VESA will fight for itself as usual driver
// Now using 1, kernel traps if trying to do VM86 too late in boot process
#define VESA_ENFORCE 1

void setTextVideoMode(void); // Using int10
int setVesaMode( u_int16_t mode ); // Using int10


// ------------------------------------------------------------------------
// pixel processing
// ------------------------------------------------------------------------


#endif // VIDEO_H

