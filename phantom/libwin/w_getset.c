/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - window attributes get/set.
 *
**/

#include <video/window.h>
#include <video/internal.h>
//#include <video/vops.h>

#include <phantom_libc.h>



void
w_get_bounds( window_handle_t w, rect_t *out )
{
    assert(out);

    out->x = w->x;
    out->y = w->y;
    out->xsize = w->xsize;
    out->ysize = w->ysize;
}


void w_set_title( window_handle_t w, const char *title )
{
    // todo static buf or strdup/free
    w->title = strdup(title);
    
    // Can update just title area of title win
    ev_q_put_win( 0, 0, UI_EVENT_WIN_REDECORATE, w );
}

