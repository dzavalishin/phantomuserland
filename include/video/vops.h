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

#ifndef VCONFIG_H
#include <video/vconfig.h>
#endif // VCONFIG_H

#include <video/color.h>
#include <video/zbuf.h>

#define W_BLEND_PIXEL( old, new, newalpha ) \
    ((unsigned char) \
      ( \
        ( ((unsigned char)(new)) * (newalpha) ) \
        + \
        ( ((unsigned char)(old)) * (1.0f - (newalpha)) ) \
      )\
    )


void video_scroll_hor( rgba_t *pixels, int xs, int ys, int lstep, int s, rgba_t bg );

void rgba2rgba_move_noalpha( struct rgba_t *dest, const struct rgba_t *src, int nelem );
void rgba2rgb_move_noalpha( struct rgb_t *dest, const struct rgba_t *src, int nelem );


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

#ifdef ARCH_ia32
//void sse_rgba2rgba_zbmove(struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos);
void auto_rgba2rgba_zbmove(struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos);
#endif // ARCH_ia32

// -----------------------------------------------------------------------
//
// Alpha blending 
//
// -----------------------------------------------------------------------

typedef void (*rgba_move_func_t)( struct rgba_t *dest, const struct rgba_t *src, int nelem );

// Using mover
void bitmap2bitmap_generic(
                   struct rgba_t *dest, int destWidth, int destHeight, int destX, int destY,
                   const struct rgba_t *src, int srcWidth, int srcHeight, int srcX, int srcY,
                   int moveWidth, int moveHeight, 
                   rgba_move_func_t mover
                  );

// Alpha blend src into dst - line
void rgba2rgba_blend( struct rgba_t *dest, const struct rgba_t *src, int nelem );

// Alpha blend src into dst - rectangle
void bitmap2bitmap_blend(
                   struct rgba_t *dest, int destWidth, int destHeight, int destX, int destY,
                   const struct rgba_t *src, int srcWidth, int srcHeight, int srcX, int srcY,
                   int moveWidth, int moveHeight );


// -----------------------------------------------------------------------
// Using destination alpha 
// -----------------------------------------------------------------------


// Alpha blend src into dst USING DESTINATION ALPHA
void rgba2rgba_blend_destalpha( struct rgba_t *dest, const struct rgba_t *src, int nelem );

void bitmap2bitmap_blend_destalpha(
                   struct rgba_t *dest, int destWidth, int destHeight, int destX, int destY,
                   const struct rgba_t *src, int srcWidth, int srcHeight, int srcX, int srcY,
                   int moveWidth, int moveHeight
                  );

/// blend src to dst - bitmap
/// bitmaps sizes supposed to be the same
void w_blend_bitmap_destalpha( drv_video_bitmap_t *dst, drv_video_bitmap_t *src );

/**
 * 
 * \brief Blend src to dst - bitmap
 * 
 * Shifts src by x/y
 * 
 * Src supposed to be bigger
 * 
 * Used to get control's part of window background into the mix.
 * 
 * \param[inout] dst Image of control (button, text field, etc) background with aplha channel
 * \param[in] src Image of window background
 * \param[in] x Shift of dst in src
 * \param[in] y Shift of dst in src
 * 
**/
void w_blend_bitmap_destalpha_shift( drv_video_bitmap_t *dst, drv_video_bitmap_t *src, int x, int y );

void w_blend_bg_to_bitmap( drv_video_bitmap_t *dst, window_handle_t src, int x, int y );


#endif // VOPS_H
