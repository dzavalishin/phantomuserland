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


//#include <drv_video_screen.h>
//#include <video.h>
#include <assert.h>
#include <phantom_libc.h>
#include <event.h>
//#include <spinlock.h>

#include <video/internal.h>

#include "win_local.h"

// min/max
#include <sys/libkern.h>



int rect_eq( rect_t *a, rect_t *b )
{
    return
        (a->x     == b->x     ) &&
        (a->y     == b->y     ) &&
        (a->xsize == b->xsize ) &&
        (a->ysize == b->ysize );
}

int rect_empty( rect_t *a )
{
    return (a->x > 0) && (a->y > 0);
}




// rect coords must be win relative
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

    if( x >= w->x + w->xsize )
        return 0;

    if( y >= w->y + w->ysize )
        return 0;

    return 1;
}

int point_in_rect( int x, int y, rect_t *r )
{
    if( x < r->x || y < r->y )
        return 0;

    if( x >= r->x + r->xsize )
        return 0;

    if( y >= r->y + r->ysize )
        return 0;

    return 1;
}



void rect_add( rect_t *out, rect_t *a, rect_t *b )
{
    assert(out);

    if( (a == 0) || (a->xsize == 0) || (a->ysize == 0) )
    {
        assert(b);
        *out = *b;
    }

    if( (b == 0) || (b->xsize == 0) || (b->ysize == 0) )
    {
        assert(a);
        *out = *a;
    }

    out->x = imin( a->x, b->x );
    out->y = imin( a->y, b->y );

    unsigned xr = imax( a->x + a->xsize, b->x + b->xsize );
    unsigned yr = imax( a->y + a->ysize, b->y + b->ysize );

    out->xsize = xr - out->x;
    out->ysize = yr - out->y;
}

//! Returns true if intersection exists
int rect_mul( rect_t *out, rect_t *a, rect_t *b )
{
    assert(out);

    out->x = imax( a->x, b->x );
    out->y = imax( a->y, b->y );

    unsigned xr = imin( a->x + a->xsize, b->x + b->xsize );
    unsigned yr = imin( a->y + a->ysize, b->y + b->ysize );

    out->xsize = xr - out->x;
    out->ysize = yr - out->y;

    return (out->xsize > 0) && (out->ysize > 0);
}



//! Returns two rectangles which together cover space which is
//! covered by old, but not covered by new.
int rect_sub( rect_t *out1, rect_t *out2, rect_t *old, rect_t *new )
{
    // out1 is big vertical rectangle, out2 - small horizontal
    /*
           +---------+
           |         |   <-- new
    +------+------+  |
    | out1 |      |  |
    |      +------+--+
    |      | out2 |      <-- old
    +------+------+
    */

    // decide it we moved to the right

    if( new->x > old->x )
    {
        // right, out1 is leftmost part of old
        *out1 = *old;
        out1->xsize = new->x - old->x;
    }
    else
    {
        // left, out1 is rightmost part of old
        *out1 = *old;
        out1->xsize = old->x - new->x;
        out1->x = new->x + new->xsize;
    }

    // decide it we moved up

    // SIMPLIFIED - out1 and out2 overlap

    if( new->y > old->y )
    {
        // up
        *out2 = *old;
        out2->ysize = new->y - old->y;
    }
    else
    {
        // down
        *out2 = *old;
        out2->ysize = old->y - new->y;
        out2->y = new->y + new->ysize;
    }

    return (!rect_eq( out1, out2 )) && (!rect_empty(out2));

}



int rect_dump( rect_t *a )
{
    printf("rect @ %d/%d, sz %d x %d\n", a->x, a->y, a->xsize, a->ysize );
}




#if !USE_ONLY_INDIRECT_PAINT

void _drv_video_winblt_locked( drv_video_window_t *from )
{
    mouse_disable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
    video_drv->winblt(from, from->x, from->y, from->z);
    mouse_enable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
}

void _drv_video_winblt( drv_video_window_t *from )
{
    w_lock();
    _drv_video_winblt_locked(from);
    w_unlock();
}


void drv_video_winblt_locked( drv_video_window_t *from )
{
    mouse_disable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
    video_drv->winblt(from, from->x, from->y, from->z);
    mouse_enable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
}

void drv_video_winblt( drv_video_window_t *from )
{
    w_lock();
    _drv_video_winblt_locked(from);
    w_unlock();
}

#endif // USE_ONLY_INDIRECT_PAINT
