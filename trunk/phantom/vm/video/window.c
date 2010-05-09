/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: yes (needs cleanup and, possibly, data structures modifiation)
 *
 *
**/

#include "drv_video_screen.h"
#include <assert.h>
#include <phantom_libc.h>
#include <event.h>
#include <spinlock.h>


//static
queue_head_t     	allwindows = { &allwindows, &allwindows };

//static
drv_video_window_t *	focused_window = 0;

//static
hal_spinlock_t  		allw_lock;

// here we have a problem - some windows are persistent and
// keep their generation. So on start we need to advance
// generation twice to make sure all windows are catched up
static int win_generation = 0;



int rect_win_bounds( rect_t *r, drv_video_window_t *w )
{
    if( r->x < 0 )
    {
        r->xsize += r->x;
        r->x = 0;
    }

    if( r->y < 0 )
    {
        r->ysize += r->y;
        r->y = 0;
    }

    if( r->x+r->xsize >= w->xsize )
        r->xsize = w->xsize-r->x;

    if( r->y+r->ysize >= w->ysize )
        r->ysize = w->ysize-r->y;

    return (r->xsize <= 0) || (r->ysize <= 0);
}


int point_in_win( int x, int y, drv_video_window_t *w )
{
    if( x < w->x || y < w->y )
        return 0;

    if( x > w->x + w->xsize )
        return 0;

    if( y > w->y + w->ysize )
        return 0;

    return 1;
}




void drv_video_window_free(drv_video_window_t *w)
{
    drv_video_window_destroy(w);
    free(w);
}



static void
common_window_init( drv_video_window_t *w,
                        int xsize, int ysize )
{
    w->xsize = xsize;
    w->ysize = ysize;

    w->li = w->ti = w->ri = w->bi = 0;

    w->generation = 0;

    w->x = 0;
    w->y = 0;
    w->z = 0xFE; // quite atop

    w->bg = COLOR_BLACK;

    queue_init(&(w->events));
    w->events_count = 0;
    w->stall = 0;
}

static drv_video_window_t *do_drv_video_window_create(int xsize, int ysize)
{
    drv_video_window_t *w = calloc(1,drv_video_window_bytes(xsize,ysize));
    if(w == 0)
        return 0;

    //w->xsize = xsize;
    //w->ysize = ysize;
    //w->z = 0xFE; // quite atop

    //w->li = w->ti = w->ri = w->bi = 0;

    //queue_init(&(w->events));
    //w->events_count = 0;
    //w->stall = 0;

    common_window_init( w, xsize, ysize );

    return w;
}

static rgba_t brdr[] = {
    { 0x40, 0x40, 0x40, 0xFF },
    { 0x80, 0x80, 0x80, 0xFF },
    { 0x40, 0x40, 0x40, 0xFF },
};

rgba_t title_back_color = { 122, 230, 251, 0xFF };

static void win_make_decorations(drv_video_window_t *w)
{
    int bordr_size = sizeof(brdr)/sizeof(rgba_t);
#if 1
    int title_size = 18;


    drv_video_window_t *w3 = do_drv_video_window_create
        (
         w->xsize+bordr_size*2,
         title_size+bordr_size*2
        );

    w3->flags |= WFLAG_WIN_NOTINALL; // On destroy don't try to remove from allwindows
    //hal_spin_lock( &allw_lock );
    //queue_enter(&allwindows, w3, drv_video_window_t *, chain);
    //hal_spin_unlock( &allw_lock );

    w3->x = w->x-bordr_size; w3->y = w->y+w->ysize; //+bordr_size;
    w3->z = w->z;

    w3->bg = title_back_color;

    w3->flags = 0;

    drv_video_window_fill( w3, w3->bg );
    window_basic_border( w3, brdr, bordr_size );

    // BUG! It must be +3, not -1 on Y coord!
    drv_video_font_draw_string( w3, &drv_video_8x16cou_font,
                                w->title, COLOR_BLACK,
                                bordr_size+3, bordr_size-1 );

    drv_video_window_draw_bitmap( w3, w3->xsize - close_bmp.xsize - 5, 5, &close_bmp );
    drv_video_window_draw_bitmap( w3, w3->xsize - pin_bmp.xsize - 2 - close_bmp.xsize - 5, 5, &pin_bmp );


    drv_video_winblt(w3);
    drv_video_window_free(w3);
#endif

    //int bordr_size = sizeof(brdr)/sizeof(rgba_t);
    drv_video_window_t *w2 = do_drv_video_window_create(w->xsize+bordr_size*2, w->ysize+bordr_size*2);

    w2->flags |= WFLAG_WIN_NOTINALL; // On destroy don't try to remove from allwindows
    //hal_spin_lock( &allw_lock );
    //queue_enter(&allwindows, w2, drv_video_window_t *, chain);
    //hal_spin_unlock( &allw_lock );

    w2->x = w->x-bordr_size; w2->y = w->y-bordr_size;
    w2->z = w->z;

    w2->bg = w->bg;

    w2->flags = 0;

    drv_video_window_fill( w2, w2->bg );
    window_basic_border( w2, brdr, bordr_size );

    drv_video_winblt(w2);
    drv_video_window_free(w2);
}


drv_video_window_t *
drv_video_window_create(
                        int xsize, int ysize,
                        int x, int y,
                        rgba_t bg, const char *title )
{
    drv_video_window_t *w = do_drv_video_window_create(xsize, ysize);
    drv_video_window_init( w, xsize, ysize, x, y, bg );
    w->title = title;
    return w;
}



// for statically allocated ones
void
drv_video_window_init( drv_video_window_t *w,
                        int xsize, int ysize,
                        int x, int y,
                        rgba_t bg )
{
    common_window_init( w, xsize, ysize );

    //w->xsize = xsize;
    //w->ysize = ysize;

    //w->li = w->ti = w->ri = w->bi = 0;

    w->flags = WFLAG_WIN_DECORATED;
    //w->generation = 0;

    w->x = x;
    w->y = y;
    //w->z = 0xFE; // quite atop
    w->bg = bg;

    w->title = "?";

    //queue_init(&(w->events));
    //w->events_count = 0;
    //w->stall = 0;

    win_make_decorations(w);

    hal_spin_lock( &allw_lock );
    queue_enter(&allwindows, w, drv_video_window_t *, chain);
    hal_spin_unlock( &allw_lock );
}


void drv_video_window_destroy(drv_video_window_t *w)
{
    if( focused_window == w )
    {
        event_q_put_win( w->x, w->y, UI_EVENT_WIN_LOST_FOCUS, focused_window );
        focused_window = 0;
    }

    if(!w->flags & WFLAG_WIN_NOTINALL )
    {
        hal_spin_lock( &allw_lock );
        queue_remove(&allwindows, w, drv_video_window_t *, chain);
        hal_spin_unlock( &allw_lock );
    }
    // This will inform win on its death and unlock event read loop to make
    // sure it doesnt loop in dead window
    event_q_put_win( w->x, w->y, UI_EVENT_WIN_DESTROYED, focused_window );



    // drain event queue
    if( w->events_count > 0 )
    {
        struct ui_event e;

        // Wait to make sure regular event pump drv_video_window_get_event is done
        hal_sleep_msec( 200 ); 

        while( w->events_count > 0 )
            drv_video_window_get_event( w, &e, 0 );
    }

}


void drv_video_window_update_generation(void)
{
    win_generation++;
    // redraw all here, or ask some thread to do that
    drv_video_window_t *w;
    hal_spin_lock( &allw_lock );
    queue_iterate(&allwindows, w, drv_video_window_t *, chain)
    {
        drv_video_winblt( w );
    }
    hal_spin_unlock( &allw_lock );
}



void
drv_video_window_fill( drv_video_window_t *win, rgba_t color )
{
    int i = (win->xsize * win->ysize) - 1;
    for( ; i >= 0; i-- )
        win->pixel[i] = color;
}


void
drv_video_window_clear( drv_video_window_t *win )
{
    drv_video_window_fill( win, COLOR_BLACK );
}


void
drv_video_window_fill_rect( drv_video_window_t *w, rgba_t color, rect_t r )
{
    if( rect_win_bounds( &r, w ) )
        return;

    int yp = r.y + r.ysize - 1;
    for( ; yp >= r.y; yp-- )
    {
        rgba_t *dst = w->pixel + yp*w->xsize + r.x;
        rgba2rgba_replicate( dst, &color, r.xsize );
    }
}


void
drv_video_window_draw_bitmap( drv_video_window_t *w, int x, int y, drv_video_bitmap_t *bmp )
{
    bitmap2bitmap(
                  w->pixel, w->xsize, w->ysize, x, y,
                  bmp->pixel, bmp->xsize, bmp->ysize, 0, 0,
                  bmp->xsize, bmp->ysize
                 );

}








void drv_video_window_preblit( drv_video_window_t *w )
{
    //window_basic_border( w, brdr, sizeof(brdr)/sizeof(rgba_t) );
    if( (w->generation != win_generation) )
    {
        w->generation = win_generation;
        if(w->flags & WFLAG_WIN_DECORATED)
            win_make_decorations(w);
    }
}



