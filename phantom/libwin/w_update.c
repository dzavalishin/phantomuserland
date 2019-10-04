/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - update window (paint to screen).
 *
 *
**/

#define DEBUG_MSG_PREFIX "win"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <assert.h>
#include <phantom_libc.h>

#include <video/window.h>
//#include <video/screen.h>
#include <video/internal.h>


#if !USE_ONLY_INDIRECT_PAINT

	
//void drv_video_winblt( drv_video_window_t *from );

void w_update( drv_video_window_t *w ) 
{ 
#if 0
    w_switch_buffers(w); 
#else
    rect_t r;
    w_get_bounds( w, &r );
    w_request_async_repaint( &r );
#endif
    drv_video_winblt( w ); 
}

#else
void w_update( drv_video_window_t *w ) 
{
    w_switch_buffers(w); 
    ev_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, w );
}

#endif // USE_ONLY_INDIRECT_PAINT

