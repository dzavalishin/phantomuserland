/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system helpers.
 *
 *
**/


#include <drv_video_screen.h>
//#include <video.h>
#include <assert.h>
#include <phantom_libc.h>
#include <event.h>
#include <spinlock.h>

#include "win_local.h"

// min/max
#include <sys/libkern.h>

int rect_win_bounds( rect_t *r, drv_video_window_t *w )
{
    if( r->x < 0 )
    {
        r->xsize += r->x;
        r->x = 0;
    }

    if( r->y < 0 )
    {
        r->ysize += r->y;
        r->y = 0;
    }

    if( r->x+r->xsize >= w->xsize )
        r->xsize = w->xsize-r->x;

    if( r->y+r->ysize >= w->ysize )
        r->ysize = w->ysize-r->y;

    return (r->xsize <= 0) || (r->ysize <= 0);
}


int point_in_win( int x, int y, drv_video_window_t *w )
{
    if( x < w->x || y < w->y )
        return 0;

    if( x > w->x + w->xsize )
        return 0;

    if( y > w->y + w->ysize )
        return 0;

    return 1;
}


void rect_add( rect_t *out, rect_t *a, rect_t *b )
{
    assert(out);

    out->x = min( a->x, b->x );
    out->y = min( a->y, b->y );

    unsigned xr = max( a->x + a->xsize, b->x + b->xsize );
    unsigned yr = max( a->y + a->ysize, b->y + b->ysize );

    out->xsize = xr - out->x;
    out->ysize = yr - out->y;
}

//! Returns true if intersection exists
int rect_mul( rect_t *out, rect_t *a, rect_t *b )
{
    assert(out);

    out->x = max( a->x, b->x );
    out->y = max( a->y, b->y );

    unsigned xr = min( a->x + a->xsize, b->x + b->xsize );
    unsigned yr = min( a->y + a->ysize, b->y + b->ysize );

    out->xsize = xr - out->x;
    out->ysize = yr - out->y;

    return (out->xsize > 0) && (out->ysize > 0);
}


