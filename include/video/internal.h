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
#include <video/window.h>

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

void 	scr_repaint_all(void) __attribute__((deprecated));
void    repaint_all_for_square( rect_t *todo );
void    w_request_async_repaint( rect_t *r );

void    request_repaint_all_for_square( rect_t *todo );
void    request_repaint_all_for_win( struct drv_video_window *w );

void    win_make_decorations(drv_video_window_t *w);


// -----------------------------------------------------------------------
// Default mouse implementation to be used by video drivers if no
// hardware impl exists
// -----------------------------------------------------------------------

void vid_mouse_set_cursor_deflt(drv_video_bitmap_t *nc);
void vid_mouse_draw_deflt(void);
void vid_mouse_off_deflt(void);
void vid_mouse_on_deflt(void);



// -----------------------------------------------------------------------
// vm stuff?
// -----------------------------------------------------------------------


struct data_area_4_bitmap;

int drv_video_string2bmp( struct data_area_4_bitmap *bmp, void *_from );





