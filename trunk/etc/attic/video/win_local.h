#ifndef WIN_LOCAL
#define WIN_LOCAL

#error unused

#include <video/screen.h>


drv_video_window_t *private_drv_video_window_create(int xsize, int ysize);
//void defaultWindowEventProcessor( drv_video_window_t *w, struct ui_event *e );
//int drv_video_window_get_event( drv_video_window_t *w, struct ui_event *e, int wait );
void win_make_decorations(drv_video_window_t *w);
void win_move_decorations(drv_video_window_t *w);
void win_draw_decorations(drv_video_window_t *w);

#if USE_ONLY_INDIRECT_PAINT

static __inline__ void _drv_video_winblt_locked( drv_video_window_t *from )
{
    mouse_disable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
    video_drv->winblt(from, from->x, from->y, from->z);
    mouse_enable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
}

static __inline__ void _drv_video_winblt( drv_video_window_t *from )
{
    w_lock();
    _drv_video_winblt_locked(from);
    w_unlock();
}

#else


void _drv_video_winblt_locked( drv_video_window_t *from );                                                                           	
void _drv_video_winblt( drv_video_window_t *from );
void drv_video_winblt_locked( drv_video_window_t *from );
void drv_video_winblt( drv_video_window_t *from );

#endif // USE_ONLY_INDIRECT_PAINT

#endif // WIN_LOCAL
