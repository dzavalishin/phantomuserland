#if 0

/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 *
 *
**/

#include <video/vops.h>
#include <assert.h>
#include <sys/types.h>




void bitmap2bitmap(
                   struct rgba_t *dest, int destWidth, int destHeight, int destX, int destY,
                   const struct rgba_t *src, int srcWidth, int srcHeight, int srcX, int srcY,
                   int moveWidth, int moveHeight
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
        rgba2rgba_move( dstPtr, srcPtr, moveWidth );
        dstPtr += destWidth;
        srcPtr += srcWidth;
    }
}

// The same, but flips picture vertically
void bitmap2bitmap_yflip(
                   struct rgba_t *dest, int destWidth, int destHeight, int destX, int destY,
                   const struct rgba_t *src, int srcWidth, int srcHeight, int srcX, int srcY,
                   int moveWidth, int moveHeight
                  )
{
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

    const struct rgba_t *srcPtr = src + (srcY+moveHeight)*srcWidth + srcX;
    struct rgba_t *dstPtr      = dest + destY*destWidth + destX;

    int hcnt;
    for(hcnt = moveHeight; hcnt; hcnt--)
    {
        rgba2rgba_move( dstPtr, srcPtr, moveWidth );
        dstPtr += destWidth;
        srcPtr -= srcWidth;
    }
}



//#if VIDEO_ZBUF
#if 1 // old code was in vm/video/bitblt.c
void rgba2rgb_zbmove( struct rgb_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos  )
{
    int *isrc = (int *)src;
    while(nelem-- > 0)
    {
#if 0
        // BUG don't update zbuf if alpha is zero?
        if( *zb > zpos ) { zb++; dest++; src++; continue; }
        *zb++ = zpos;

        if(src->a)
        {
            dest->r = src->r;
            dest->g = src->g;
            dest->b = src->b;
        }
        dest++;
        src++;
#else
        // BUG don't update zbuf if alpha is zero?

        if( !(*isrc>>24) || *zb > zpos ) { zb++; dest++; isrc++; continue; }
        *zb++ = zpos;
        //if( !(src->a) ) { dest++; src++; continue; }

        int w = *isrc++;

        dest->r = w >> 16;
        dest->g = w >> 8;
        dest->b = w >> 0;

        dest++;
#endif
    }
}

void rgb2rgba_zbmove( struct rgba_t *dest, const struct rgb_t *src, zbuf_t *zb, int nelem, zbuf_t zpos )
{
    while(nelem-- > 0)
    {
        if( *zb > zpos ) { zb++; dest++; src++; continue; }
        *zb++ = zpos;

        dest->a = 0xFF;

        dest->r = src->r;
        dest->g = src->g;
        dest->b = src->b;

        dest++;
        src++;
    }
}



#if 0 // use asm version

#if 0
void rgba2rgba_zbmove( struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos )
{
    while(nelem-- > 0)
        if(src->a)
        {
            if( *zb > zpos ) { zb++; dest++; src++; continue; }
            *zb++ = zpos;

            *dest++ = *src++;
        }
        else
        {
            dest++;
            src++;
            zb++;
        }
}
#else

#define ZBM_STEP \
    {                                 \
        if( src->a && (*zb <= zpos) ) \
        {                             \
            *zb = zpos;               \
            *dest = *src;             \
        }                             \
                                      \
        dest++;                       \
        src++;                        \
        zb++;                         \
    }

void rgba2rgba_zbmove( struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos )
{
    while(nelem > 8)
    {
        nelem -= 8;
        ZBM_STEP
        ZBM_STEP
        ZBM_STEP
        ZBM_STEP
        ZBM_STEP
        ZBM_STEP
        ZBM_STEP
        ZBM_STEP
    }

    while(nelem-- > 0)
    {
        ZBM_STEP
    }
}

#endif

#endif


void rgba2rgba_zbreplicate( struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos )
{
    if(!src->a) return;
    while(nelem-- > 0)
    {
        if( *zb > zpos ) { zb++; dest++; continue; }
        *zb++ = zpos;
        *dest++ = *src;
    }
}



void int565_to_rgba_zbmove( struct rgba_t *dest, const short int *src, zbuf_t *zb, int nelem, zbuf_t zpos )
{
    while(nelem-- > 0)
    {
        if( *zb > zpos ) { zb++; dest++; src++; continue; }
        *zb++ = zpos;

        dest->a = 0xFF;

        dest->r = ((*src >>11) & 0x1F) << 3;
        dest->g = ((*src >>5)  & 0x3F) << 2;
        dest->b = (*src & 0x1F) << 3;

        dest++;
        src++;
    }
}


//#else

void rgba2rgb_move( struct rgb_t *dest, const struct rgba_t *src, int nelem )
{
    while(nelem-- > 0)
    {
        if(src->a)
        {
            dest->r = src->r;
            dest->g = src->g;
            dest->b = src->b;
        }
        dest++;
        src++;
    }
}

void rgba2rgb_move_noalpha( struct rgb_t *dest, const struct rgba_t *src, int nelem )
{
    while(nelem-- > 0)
    {
        dest->r = src->r;
        dest->g = src->g;
        dest->b = src->b;
        
        dest++;
        src++;
    }
}



void rgb2rgba_move( struct rgba_t *dest, const struct rgb_t *src, int nelem )
{
    while(nelem-- > 0)
    {
        dest->a = 0xFF;

        dest->r = src->r;
        dest->g = src->g;
        dest->b = src->b;

        dest++;
        src++;
    }
}


void rgba2rgba_move( struct rgba_t *dest, const struct rgba_t *src, int nelem )
{
    while(nelem-- > 0)
        if(src->a)
        {
            *dest++ = *src++;
        }
        else
        {
            dest++;
            src++;
        }
}

void rgba2rgba_move_noalpha( struct rgba_t *dest, const struct rgba_t *src, int nelem )
{
    while(nelem-- > 0)
        *dest++ = *src++;
}



void rgba2rgba_24_move( struct rgba_t *dest, const struct rgba_t *src, int nelem )
{
    while(nelem-- > 0)
    {
        if(src->a)
        {
            u_int32_t *d = (u_int32_t *)dest;
            u_int32_t *s= (u_int32_t *)src;
            *d = ((*s) & 0xFFFFFFu) | 0xFF000000u;
        }
        dest++;
        src++;
    }
}



void rgba2rgba_replicate( struct rgba_t *dest, const struct rgba_t *src, int nelem )
{
    if(!src->a) return;
    while(nelem-- > 0)
    {
        *dest++ = *src;
    }
}



void int565_to_rgba_move( struct rgba_t *dest, const short int *src, int nelem )
{
    while(nelem-- > 0)
    {
        dest->a = 0xFF;

        dest->r = ((*src >>11) & 0x1F) << 3;
        dest->g = ((*src >>5)  & 0x3F) << 2;
        dest->b = (*src & 0x1F) << 3;

        dest++;
        src++;
    }

}

//#endif


#endif






void
w_draw_bitmap( drv_video_window_t *w, int x, int y, drv_video_bitmap_t *bmp )
{
    bitmap2bitmap(
                  w->pixel, w->xsize, w->ysize, x, y,
                  bmp->pixel, bmp->xsize, bmp->ysize, 0, 0,
                  bmp->xsize, bmp->ysize
                 );

}









#endif
