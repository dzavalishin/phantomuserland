/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - create/destroy.
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
#include <event.h>

#include <video/window.h>
#include <video/control.h>
#include <video/internal.h>
#include <video/screen.h>



queue_head_t     	allwindows = { &allwindows, &allwindows };
drv_video_window_t *	focused_window = 0;




void drv_video_window_free(drv_video_window_t *w)
{
    drv_video_window_destroy(w);
    free(w);
}

/**
 * 
 * NB! xsize/ysize must be set up before call to us!
 * 
**/
void iw_setup_buffers(drv_video_window_t *w)
{
    w->buf[0] = w->bitmap;
    w->buf[1] = w->bitmap + (w->xsize * w->ysize);
    iw_switch_buffers(w); // Will assign r_ and w_
}

void iw_switch_buffers(drv_video_window_t *w)
{
    assert(w);
#if VIDEO_DOUBLE_BUF
    // Not doublebuffer mode? Just set both ptrs to buf 0
    if(!WIN_HAS_FLAG(w,WFLAG_WIN_DOUBLEBUF))
    {
        w->r_pixel = w->bitmap;
        w->w_pixel = w->bitmap;
        return;
    }

    if( w->r_pixel == w->buf[0] )
    {
        w->r_pixel = w->buf[1];
        w->w_pixel = w->buf[0];
    }
    else
    {
        w->r_pixel = w->buf[0];
        w->w_pixel = w->buf[1];
    }

    // If not full paint, copy current displayed buffer to paint buffer
    if(!WIN_HAS_FLAG(w,WFLAG_WIN_FULLPAINT))
        memmove( w->w_pixel, w->r_pixel, sizeof(rgba_t) * w->xsize * w->ysize );
#else

#ifndef r_pixel
    w->r_pixel = w->pixels;
#endif

#ifndef w_pixel
    w->w_pixel = w->pixels;
#endif

#endif
}

static void
common_window_init( drv_video_window_t *w,  void *pixels,
                        int xsize, int ysize )
{
    LOG_FLOW( 1, "w %p, pix %p size %dx%d", w, pixels, xsize, ysize );
    if(pixels == 0)
    {
        // TODO drv_video_window_bytes gives too much memory, need special func for pixels only
        pixels = calloc( 1, drv_video_window_bytes(xsize, ysize) );
        assert(pixels != 0); // userland gives us buffer, so we alloc just for kernel
    }

    w->bitmap = pixels;

    w->state |= WSTATE_WIN_VISIBLE; // default state is visible

    w->xsize = xsize;
    w->ysize = ysize;

    w->li = w->ti = w->ri = w->bi = 0;

    //w->generation = 0;

    w->x = 0;
    w->y = 0;
    w->z = 0xFE; // quite atop

    w->bg = COLOR_BLACK;

    queue_init(&(w->events));
    w->events_count = 0;
    w->stall = 0;

    w->inKernelEventProcess = defaultWindowEventProcessor;
    w->owner = 0;

#if !VIDEO_T_IN_D
    w->w_title = 0;
#endif
    w->w_decor = 0;
    w->w_owner = 0;

    // After setting most of the structure, namely x/y sizes
    iw_setup_buffers(w);
}

//! Called from object restart code to reinit window struct
//! contained in VM object. This func just clears pointers.
//! After calling this func you must reset all the required
//! fields and call w_restart_attach( w )
//! to add window to in-kernel lists and repaint it.
void w_restart_init(drv_video_window_t *w, void *pixels)
{
    w->bitmap = pixels; 
    // TODO actually iw_setup_buffers will break current painting by
    // switching buffers in the middle of user program's paint process
    // we must either do not call it for pointers in snapped w are 
    // correct (check?), or recreate in a smart way.
    iw_setup_buffers(w); 

    w->title = 0; 

    w->inKernelEventProcess = 0;
    // Can't without owner!
    w->inKernelEventProcess = defaultWindowEventProcessor;

    // BUG! How do we fill owner? We must have object ref here
    w->owner = -1;

    w->eventDeliverSema = 0;
    w->w_title = 0;
    w->w_decor = 0;
    w->w_owner = 0;

    w->controls = 0; // ? TODO how do we resetup 'em?

    queue_init(&(w->events));
    w->events_count = 0;

    // Can't be after reboot
    w->state &= ~(WSTATE_WIN_FOCUSED|WSTATE_WIN_DRAGGED|WSTATE_WIN_INFB);

    // Unsure, better be off
    w->state &= ~WSTATE_WIN_UNCOVERED;

}

// Called from vm restart code with no w_lock taken
void w_restart_attach( drv_video_window_t *w )
{
    w_lock();
    iw_enter_allwq(w);
    win_make_decorations(w);
    w_unlock();
}


drv_video_window_t *private_drv_video_window_create(int xsize, int ysize)
{
    drv_video_window_t *w = calloc(1,sizeof(drv_video_window_t));
    if(w == 0)
        return 0;

        // TODO drv_video_window_bytes gives too much memory, need special func for pixels only
    void *pixels = calloc( 1, drv_video_window_bytes(xsize, ysize) );

    if(pixels == 0)
    {
        free(w);
        return 0;
    }

    common_window_init( w, pixels, xsize, ysize );
    w->flags |= WFLAG_WIN_DECORATED;
   
    return w;
}



drv_video_window_t *
drv_video_window_create(
                        int xsize, int ysize,
                        int x, int y,
                        rgba_t bg, const char *title, int flags )
{
    //drv_video_window_t *w = private_drv_video_window_create(xsize, ysize);

    //LOG_FLOW()
    lprintf( "drv_video_window_create %dx%d\n", xsize, ysize ); 

    drv_video_window_t *w = calloc(1,sizeof(drv_video_window_t));
    if(w == 0)
        return 0;


    drv_video_window_init( w, 0, xsize, ysize, x, y, bg, flags, title );
    //w->flags = flags;

    //w->title = title; // TODO pass title to _init
    // Repaint title
    //ev_q_put_win( 0, 0, UI_EVENT_WIN_REDECORATE, w );

    return w;
}



// for statically allocated ones
void
drv_video_window_init( drv_video_window_t *w,
                        void *pixels,
                       int xsize, int ysize,
                       int x, int y,
                       rgba_t bg, int flags,
                       const char *title
                     )
{
    bzero(w, sizeof(*w));
    common_window_init( w, pixels, xsize, ysize );

    //w->flags |= WFLAG_WIN_DECORATED;
    w->flags = flags;

    w->x = x;
    w->y = y;
    //w->z = 0xFE; // quite atop
    w->bg = bg;

    w->title = title ? title : "?";

    w_lock();
    //drv_video_winblt_locked( w->w_owner ); // need?
    //win_make_decorations(w);
    iw_enter_allwq(w);
    win_make_decorations(w);
    w_unlock();

    // Actually reorders window in all w list - finally it is possible that win
    // will NOT be on top - some windows with special abilities can still be above
    // More critical is that w_to_top reorders child (decor and title) windows
    // in a correct way
    w_to_top(w);
}


void iw_enter_allwq( drv_video_window_t *w)
{
    w_assert_lock();
    queue_enter(&allwindows, w, drv_video_window_t *, chain);
    drv_video_window_rezorder_all();
}


void drv_video_window_destroy(drv_video_window_t *w)
{
    if( focused_window == w )
    {
        ev_q_put_win( w->x, w->y, UI_EVENT_WIN_LOST_FOCUS, focused_window );
        focused_window = 0;
    }

    if(!(w->flags & WFLAG_WIN_NOTINALL))
    {
        w_lock();
        queue_remove(&allwindows, w, drv_video_window_t *, chain);
        w_unlock();
    }
    // This will inform win on its death and unlock event read loop to make
    // sure it doesnt loop in dead window
    ev_q_put_win( w->x, w->y, UI_EVENT_WIN_DESTROYED, focused_window );


    if(w->controls)
        destroy_controls_pool(w->controls);

    request_repaint_all_for_win( w );

    // drain event queue
    if( w->events_count > 0 )
    {
        struct ui_event e;

        // Wait to make sure regular event pump drv_video_window_get_event is done
        hal_sleep_msec( 200 ); 

        while( w->events_count > 0 )
            ev_w_get_event( w, &e, 0 );
    }

#if !VIDEO_T_IN_D
    if(w->w_title)
        drv_video_window_free(w->w_title);
    w->w_title = 0;
#endif

    if(w->w_decor)
        drv_video_window_free(w->w_decor);
    w->w_decor = 0;
}







