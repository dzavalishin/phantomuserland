/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Z buffer operations.
 *
**/

#ifndef ZBUF_H
#define ZBUF_H

#ifndef VCONFIG_H
#include <video/vconfig.h>
#endif // VCONFIG_H

#include <video/window.h>

#define USE_ZBUF_SHADOW 0



// Special value of z coord which means that we draw anyway - used for mouse
#define ZBUF_TOP 0xFFFFFFFF

//typedef u_int8_t zbuf_t;
typedef u_int32_t zbuf_t;

extern zbuf_t *zbuf;

//! hack - switches zbuf to be top->bottom or vice versa
void scr_zbuf_turn_upside(int v);


void scr_zbuf_init(void);
void scr_zbuf_reset(void);
void scr_zbuf_reset_square(int x, int y, int xsize, int ysize );
void scr_zbuf_reset_square_z(int x, int y, int xsize, int ysize, zbuf_t zpos );

void scr_zbuf_reset_win( window_handle_t w );
void scr_zbuf_set_win_z( window_handle_t w );

int scr_zbuf_check( int linpos, zbuf_t zpos );

// Set z order for window according to its position in all win q
void drv_video_window_rezorder_all(void);

// Debug - paint zbuf onscreen
void scr_zbuf_paint(void);


#if USE_ZBUF_SHADOW
void scr_zbuf_request_reset_square(int x, int y, int xsize, int ysize );
void scr_zbuf_request_reset_square_z(int x, int y, int xsize, int ysize, zbuf_t zpos );
void scr_zbuf_apply_shadow(void);

#endif


#endif // ZBUF_H
