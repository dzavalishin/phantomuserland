/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - resize. Unimpl.
 *
**/

#define DEBUG_MSG_PREFIX "win"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>

#include <video/window.h>
#include <video/internal.h>
#include <video/zbuf.h>
#include <video/vops.h>


static void
do_window_resize( drv_video_window_t *w, int xsize, int ysize, void *new_buffer, int clear )
{

    if(clear)
    {
        if( 0 == new_buffer )
        {
            if( (xsize * ysize) < (w->xsize * w->ysize))
                new_buffer = w->bitmap;
            else
            {
                free(w->bitmap); // TODO assert not persistent ptr
                new_buffer = calloc( 1, drv_video_window_bytes(xsize,ysize) );
            }            
        }

        w->bitmap = new_buffer;
        w->xsize = xsize;
        w->ysize = ysize;
        iw_setup_buffers(w); // Needs correct win x/y size
        w_fill( w, w->bg );
    }
    else
    {
        int free_old = 0;
        if( 0 == new_buffer )
        {
            new_buffer = calloc( 1, drv_video_window_bytes(xsize,ysize) );
            free_old = 1;
        }

        w_fill_bitmap( new_buffer, w->bg, xsize*ysize );

        // read from w_pixel?
        bitmap2bitmap(
            new_buffer, xsize, ysize, 0, 0,
            w->r_pixel, w->xsize, w->ysize, 0, 0,
            xsize, ysize
        );

        if(free_old) free(w->bitmap); // TODO assert not persistent ptr
        w->bitmap = new_buffer;
        w->xsize = xsize;
        w->ysize = ysize;
        iw_setup_buffers(w); // Needs correct win x/y size
    }
    
}




void    
w_resize_ext( window_handle_t w, int xsize, int ysize, void *new_buffer, int clear )
{
    rect_t oldsize;
    rect_t newsize;
    rect_t maxsize;

    //w->state &= ~WSTATE_WIN_UNCOVERED; // mark as possibly covered

    w_get_bounds( w, &oldsize );
    do_window_resize(w, xsize, ysize, new_buffer, clear);
    w_get_bounds( w, &newsize );

    win_make_decorations( w ); // TODO clear old decorations?!

    rect_add( &maxsize, &oldsize, &newsize );

    scr_zbuf_reset_square( maxsize.x, maxsize.y, maxsize.xsize, maxsize.ysize );

    {
    ui_event_t e;
    //struct ui_event e;
    e.type = UI_EVENT_TYPE_GLOBAL;
    e.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
    e.w.rect = maxsize;

    ev_q_put_global( &e );
    }
}
