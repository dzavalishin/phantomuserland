/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Pixel copy/conversion code with alpha blending
 *
 *
**/

#include <video/vops.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/libkern.h>





void bitmap2bitmap_generic(
                   struct rgba_t *dest, int destWidth, int destHeight, int destX, int destY,
                   const struct rgba_t *src, int srcWidth, int srcHeight, int srcX, int srcY,
                   int moveWidth, int moveHeight, 
                   rgba_move_func_t mover
                  )
{
    /*
     if( destX >= destWidth || destX+moveWidth <= 0)
     return;

     if( srcX >= srcWidth || srcX+moveWidth <= 0)
     return;
     */

    // now clip

    int leftExcessX = 0;

    if( destX+moveWidth >= destWidth )
        moveWidth = destWidth-destX;

    if( destX <= 0)
        leftExcessX = -destX;

    if( srcX+moveWidth >= srcWidth )
        moveWidth = srcWidth-srcX;

    if( srcX <= 0 && ((-srcX) < leftExcessX) )
        leftExcessX = -srcX;

    if(leftExcessX)
    {
        moveWidth -= leftExcessX;
        destX += leftExcessX;
        srcX += leftExcessX;
    }


    int leftExcessY = 0;

    if( destY+moveHeight >= destHeight )
        moveHeight = destHeight-destY;

    if( destY <= 0)
        leftExcessY = -destY;

    if( srcY+moveHeight >= srcHeight )
        moveHeight = srcHeight-srcY;

    if( srcY <= 0 && ((-srcY) < leftExcessY) )
        leftExcessY = -srcY;

    if(leftExcessY)
    {
        moveHeight -= leftExcessY;
        destY += leftExcessY;
        srcY += leftExcessY;
    }

    if( moveWidth <= 0 || moveHeight <= 0 )
        return;

    //printf("blit %d x %d", moveWidth, moveHeight );
    //int srcShift = srcX * sizeof(struct rgba_t);
    //int dstShift = destX* sizeof(struct rgba_t);
    //int srcLineStep = srcWidth * sizeof(struct rgba_t);
    //int dstLineStep = destWidth * sizeof(struct rgba_t);

    const struct rgba_t *srcPtr = src + srcY*srcWidth + srcX;
    struct rgba_t *dstPtr      = dest + destY*destWidth + destX;

    int hcnt;
    for(hcnt = moveHeight; hcnt; hcnt--)
    {
        mover( dstPtr, srcPtr, moveWidth );
        dstPtr += destWidth;
        srcPtr += srcWidth;
    }
}









// Alpha blend src into dst
void rgba2rgba_blend( struct rgba_t *dest, const struct rgba_t *src, int nelem )
{
    while(nelem-- > 0)
    {
        float a = src->a / 255.0;
        dest->r = W_BLEND_PIXEL( dest->r, src->r, a );
        dest->g = W_BLEND_PIXEL( dest->g, src->g, a );
        dest->b = W_BLEND_PIXEL( dest->b, src->b, a );

        dest++;
        src++;
    }
}



void bitmap2bitmap_blend(
                   struct rgba_t *dest, int destWidth, int destHeight, int destX, int destY,
                   const struct rgba_t *src, int srcWidth, int srcHeight, int srcX, int srcY,
                   int moveWidth, int moveHeight
                  )
{
    bitmap2bitmap_generic(
                   dest, destWidth, destHeight, destX, destY,
                   src, srcWidth, srcHeight, srcX, srcY,
                   moveWidth, moveHeight, rgba2rgba_blend );

}


/// blend src to dst - bitmap
/// bitmaps sizes supposed to be the same
void w_blend_bitmap( drv_video_bitmap_t *dst, drv_video_bitmap_t *src )
{
    int xsize = imin( src->xsize, dst->xsize );
    int ysize = imin( src->ysize, dst->ysize );

    bitmap2bitmap_blend(
                  dst->pixel, dst->xsize, dst->ysize, 0, 0,
                  src->pixel, src->xsize, src->ysize, 0, 0,
                  xsize, ysize
                 );

}




// -----------------------------------------------------------------------
//
// Alpha blending using dest alpha.
//
// Usage: 
//
//   dst - UI control background pic with alpha
//   src - regular window BG fill
//
//   dst mixed = UI control background painted onto window bg, 
//   stored back to UI control background bitmap
//
// -----------------------------------------------------------------------




// Alpha blend src into dst USING DESTINATION ALPHA
void rgba2rgba_blend_destalpha( struct rgba_t *dest, const struct rgba_t *src, int nelem )
{
    while(nelem-- > 0)
    {
        float a = dest->a / 255.0;
        dest->r = W_BLEND_PIXEL( src->r, dest->r, a );
        dest->g = W_BLEND_PIXEL( src->g, dest->g, a );
        dest->b = W_BLEND_PIXEL( src->b, dest->b, a );
        dest->a = 0xFF; // TODO make me to be correct

        dest++;
        src++;
    }
}

void bitmap2bitmap_blend_destalpha(
                   struct rgba_t *dest, int destWidth, int destHeight, int destX, int destY,
                   const struct rgba_t *src, int srcWidth, int srcHeight, int srcX, int srcY,
                   int moveWidth, int moveHeight
                  )
{
    bitmap2bitmap_generic(
                   dest, destWidth, destHeight, destX, destY,
                   src, srcWidth, srcHeight, srcX, srcY,
                   moveWidth, moveHeight, rgba2rgba_blend_destalpha );

}


/// blend src to dst - bitmap
/// bitmaps sizes supposed to be the same
void w_blend_bitmap_destalpha( drv_video_bitmap_t *dst, drv_video_bitmap_t *src )
{
    int xsize = imin( src->xsize, dst->xsize );
    int ysize = imin( src->ysize, dst->ysize );

    bitmap2bitmap_blend_destalpha(
                  dst->pixel, dst->xsize, dst->ysize, 0, 0,
                  src->pixel, src->xsize, src->ysize, 0, 0,
                  xsize, ysize
                 );
}

/// blend src to dst - bitmap
/// Shifts src by x/y - to get control's part of window background into the mix
void w_blend_bitmap_destalpha_shift( drv_video_bitmap_t *dst, drv_video_bitmap_t *src, int x, int y )
{
    bitmap2bitmap_blend_destalpha(
                  dst->pixel, dst->xsize, dst->ysize, 0, 0,
                  src->pixel, src->xsize, src->ysize, x, y,
                  dst->xsize, dst->ysize
                 );
}

/// blend src to dst - bitmap
/// Shifts src by x/y - to get control's part of window background into the mix
void w_blend_bg_to_bitmap( drv_video_bitmap_t *dst, window_handle_t src, int x, int y )
{
    bitmap2bitmap_blend_destalpha(
                  dst->pixel, dst->xsize, dst->ysize, 0, 0,
                  src->w_pixel, src->xsize, src->ysize, x, y,
                  dst->xsize, dst->ysize
                 );
}
