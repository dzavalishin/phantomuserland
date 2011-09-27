/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Generic rgba_t operations.
 *
**/

#ifndef VOPS_H
#define VOPS_H

#include <video/color.h>
#include <video/zbuf.h>

void video_scroll_hor( rgba_t *pixels, int xs, int ys, int lstep, int s, rgba_t bg );


void rgba2rgb_move( struct rgb_t *dest, const rgba_t *src, int nelem );
void rgba2rgba_move( rgba_t *dest, const rgba_t *src, int nelem );
void rgba2rgba_24_move( struct rgba_t *dest, const struct rgba_t *src, int nelem ); //! copies 24 bits, sets alpha byte to FF

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

// + flip vert
void bitmap2bitmap_yflip(
                   struct rgba_t *dest, int destWidth, int destHeight, int destX, int destY,
                   const struct rgba_t *src, int srcWidth, int srcHeight, int srcX, int srcY,
                   int moveWidth, int moveHeight
                  );


#endif // VOPS_H
