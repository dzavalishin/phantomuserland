/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - pattern replication - for border drawing.
 *
**/



#include <video/window.h>
#include <video/internal.h>
#include <video/vops.h>


void w_replicate_ver( window_handle_t dest, int destX, int destY,
                           int nSteps, const struct rgba_t *src, int srcSize
                         )
{
    int srcShift = 0;
    if( destX < 0 )
    {
        srcShift = -destX;
        destX = 0;
    }

    if( destX+srcSize >= dest->xsize )
        srcSize = dest->xsize-destX;

    if( srcSize <= 0 ) return;

    if( destY < 0 )
    {
        nSteps += destY;
        destY = 0;
    }

    if( destY+nSteps >= dest->ysize )
        nSteps = dest->ysize - destY;

    src += srcShift;

    int vcnt;
    for(vcnt = destY; vcnt < destY+nSteps; vcnt++ )
    {
        struct rgba_t *dstPtr = dest->w_pixel + vcnt*dest->xsize + destX;
        rgba2rgba_move( dstPtr, src, srcSize );
    }
}


// nSteps is x size, srcSize is y size
void w_replicate_hor( window_handle_t dest, int destX, int destY,
                           int nSteps, const struct rgba_t *src, int srcSize
                         )
{
    int srcShift = 0;
    if( destY < 0 )
    {
        srcShift = -destY;
        destY = 0;
    }

    if( destY+srcSize >= dest->ysize )
        srcSize = dest->ysize - destY;

    if( srcSize <= 0 ) return;

    if( destX < 0 )
    {
        nSteps += destX;
        destX = 0;
    }

    if( destX+nSteps >= dest->xsize )
        nSteps = dest->xsize-destX;

    src += srcShift;

    int vcnt;
    for(vcnt = destY; vcnt < destY+srcSize; vcnt++ )
    {
        struct rgba_t *dstPtr = dest->w_pixel + vcnt*dest->xsize + destX;
        rgba2rgba_replicate( dstPtr, src, nSteps );
        src++;
    }
}

