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

// Special value of z coord which means that we draw anyway - used for mouse
#define ZBUF_TOP 0xFF

typedef u_int8_t zbuf_t;
//typedef u_int32_t zbuf_t;

extern zbuf_t *zbuf;

//! hack - switches zbuf to be top->bottom or vice versa
void video_zbuf_turn_upside(int v);


void video_zbuf_init(void);
void video_zbuf_reset(void);
void video_zbuf_reset_square(int x, int y, int xsize, int ysize );
void video_zbuf_reset_square_z(int x, int y, int xsize, int ysize, u_int8_t zpos );
void video_zbuf_reset_win( window_handle_t w );

int video_zbuf_check( int linpos, u_int8_t zpos );

// Set z order for window according to its position in all win q
void drv_video_window_rezorder_all(void);

// Debug - paint zbuf onscreen
void video_zbuf_paint(void);

#endif // ZBUF_H
