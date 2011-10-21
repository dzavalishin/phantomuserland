/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal video ops/definitions - needed mostly by windowing code.
 *
**/

#include <video/bitmap.h>

void init_new_windows(void);


// -----------------------------------------------------------------------
// windows lock - must be taken when manipulate all win q or on update
// -----------------------------------------------------------------------


void w_lock(void);
void w_unlock(void);
void w_assert_lock(void);


// -----------------------------------------------------------------------
// Screen repaint
// -----------------------------------------------------------------------

void 	drv_video_window_repaint_all(void);
void    repaint_all_for_square( rect_t *todo );
void    w_request_async_repaint( rect_t *r );

void    request_repaint_all_for_square( rect_t *todo );
void    request_repaint_all_for_win( struct drv_video_window *w );


// -----------------------------------------------------------------------
// Default mouse implementation to be used by video drivers if no
// hardware impl exists
// -----------------------------------------------------------------------

void drv_video_set_mouse_cursor_deflt(drv_video_bitmap_t *nc);
void drv_video_draw_mouse_deflt(void);
void drv_video_mouse_off_deflt(void);
void drv_video_mouse_on_deflt(void);



// -----------------------------------------------------------------------
// vm stuff?
// -----------------------------------------------------------------------


struct data_area_4_bitmap;

int drv_video_ppm_load( drv_video_bitmap_t **to, void *from );
int drv_video_string2bmp( struct data_area_4_bitmap *bmp, void *_from );





