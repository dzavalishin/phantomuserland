#if 1
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Window - direct blit to screen.
 *
 *
**/


#include <assert.h>
#include <phantom_libc.h>
#include <event.h>

#include <video/internal.h>
#include <video/screen.h>

// min/max
#include <sys/libkern.h>





#if !USE_ONLY_INDIRECT_PAINT

void iw_winblt_locked( drv_video_window_t *from )
{
    w_assert_lock();

    if( (from->state & WSTATE_WIN_ROLLEDUP) || !(from->state & WSTATE_WIN_VISIBLE) )
        return;

    scr_mouse_disable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
    video_drv->winblt(from, from->x, from->y, from->z);
    scr_mouse_enable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
}

void iw_winblt( drv_video_window_t *from )
{
    w_lock();
    iw_winblt_locked(from);
    w_unlock();
}


#if 1
void drv_video_winblt_locked( drv_video_window_t *from )
{
    iw_winblt_locked( from );
}

void drv_video_winblt( drv_video_window_t *from )
{
    w_lock();
    iw_winblt_locked(from);
    w_unlock();
}
#endif

#endif // USE_ONLY_INDIRECT_PAINT

#endif
