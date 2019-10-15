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
#include <event.h>

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
    // TODO static buf or strdup/free
    w->title = strdup(title);
    
    // Can update just title area of title win
    ev_q_put_win( 0, 0, UI_EVENT_WIN_REDECORATE, w );
}



void w_set_visible( window_handle_t w, int v )
{
    //if(v) w->state |= WSTATE_WIN_VISIBLE;
    //else  w->state &= ~WSTATE_WIN_VISIBLE;
    u_int32_t to1 = 0;
    u_int32_t to0 = ~0;

    if(v) to1 |= WSTATE_WIN_VISIBLE;
    else  to0 &= ~WSTATE_WIN_VISIBLE;

    w->state |= to1;
    w->state &= to1;
    ev_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, w );

    if(w->w_decor)
    {
        w->w_decor->state |= to1;
        w->w_decor->state &= to1;
        ev_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, w->w_decor );
    }

    if(w->w_title)
    {
        w->w_title->state |= to1;
        w->w_title->state &= to1;
        ev_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, w->w_title );
    }

    
}

void w_set_bg_color( window_handle_t w, rgba_t color ) 
{ 
    w->bg = color; 
}