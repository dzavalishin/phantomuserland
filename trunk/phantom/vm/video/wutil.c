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





#if !USE_ONLY_INDIRECT_PAINT

void _drv_video_winblt_locked( drv_video_window_t *from )
{
    if( (from->state & WSTATE_WIN_ROLLEDUP) || !(from->state & WSTATE_WIN_VISIBLE) )
        return;

    scr_mouse_disable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
    video_drv->winblt(from, from->x, from->y, from->z);
    scr_mouse_enable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
}

void _drv_video_winblt( drv_video_window_t *from )
{
    w_lock();
    _drv_video_winblt_locked(from);
    w_unlock();
}


void drv_video_winblt_locked( drv_video_window_t *from )
{
    _drv_video_winblt_locked( from );

    //mouse_disable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
    //video_drv->winblt(from, from->x, from->y, from->z);
    //mouse_enable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
}

void drv_video_winblt( drv_video_window_t *from )
{
    w_lock();
    _drv_video_winblt_locked(from);
    w_unlock();
}

#endif // USE_ONLY_INDIRECT_PAINT
