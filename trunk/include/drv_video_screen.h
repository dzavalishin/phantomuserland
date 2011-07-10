/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Video subsystem.
 *
**/

#ifndef DRV_VIDEO_SCREEN_H
#define DRV_VIDEO_SCREEN_H

#include <phantom_types.h>
#include <queue.h>

#include <video/rect.h>
#include <video/font.h>
#include <video/color.h>
#include <video/bitmap.h>
#include <video/window.h>
#include <video/vops.h>
#include <video/zbuf.h>
//#include <video/screen.h>

#include <event.h>


#define VIDEO_ZBUF 1


// windows lock - must be taken when manipulate all win q or on update
void w_lock(void);
void w_unlock(void);
void w_assert_lock(void);



void 	drv_video_window_repaint_all(void);
void    repaint_all_for_square( rect_t *todo );







struct data_area_4_bitmap;

int drv_video_ppm_load( drv_video_bitmap_t **to, void *from );
int drv_video_string2bmp( struct data_area_4_bitmap *bmp, void *_from );



// Default mouse implementation to be used by video drivers if no
// hardware impl exists
void drv_video_set_mouse_cursor_deflt(drv_video_bitmap_t *nc);
void drv_video_draw_mouse_deflt(void);
void drv_video_mouse_off_deflt(void);
void drv_video_mouse_on_deflt(void);




#if !USE_ONLY_INDIRECT_PAINT

/*
static __inline__ void drv_video_winblt_locked( drv_video_window_t *from )
{
    mouse_disable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
    video_drv->winblt(from, from->x, from->y, from->z);
    mouse_enable_p(video_drv, from->x, from->y, from->xsize, from->ysize );
}

static __inline__ void drv_video_winblt( drv_video_window_t *from )
{
    w_lock();
    drv_video_winblt_locked(from);
    w_unlock();
}
*/

static __inline__ void drv_video_window_update( drv_video_window_t *w ) { drv_video_winblt( w ); }

#else

static __inline__ void drv_video_window_update( drv_video_window_t *w ) 
{
    event_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, w );
}

#endif // USE_ONLY_INDIRECT_PAINT








/**
 *
 * Replicates src to dest. src has one horiz. line of srcSize pixels.
 * nSteps is number of replication steps vertically.
 *
**/

void replicate2window_ver( drv_video_window_t *dest, int destX, int destY,
                       int nSteps, const rgba_t *src, int srcSize );


/**
 *
 * Replicates src to dest. src has one vert. line of srcSize pixels.
 * nSteps is number of times to draw src horizontally.
 *
**/

void replicate2window_hor( drv_video_window_t *dest, int destX, int destY,
                       int nSteps, const rgba_t *src, int srcSize );


void window_basic_border( drv_video_window_t *dest, const rgba_t *src, int srcSize );







#ifndef VIDEO_H
#include <video.h>
#endif // VIDEO_H


#endif // DRV_VIDEO_SCREEN_H

