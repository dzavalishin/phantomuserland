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

// here we have a problem - some windows are persistent and
// keep their generation. So on start we need to advance
// generation twice to make sure all windows are cathed up
static int win_generation = 0;

void drv_video_window_update_generation(void)
{
    win_generation++;
    // TODO ERR redraw all here, or ask some thread to do that
}


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


void drv_video_window_free(drv_video_window_t *w)
{
    free(w);
}



static drv_video_window_t *do_drv_video_window_create(int xsize, int ysize)
{
    drv_video_window_t *w = malloc(drv_video_window_bytes(xsize,ysize));
    if(w == 0)
        return 0;

    w->xsize = xsize;
    w->ysize = ysize;

    w->li = w->ti = w->ri = w->bi = 0;

    return w;
}

static rgba_t brdr[] = {
    { 0x40, 0x40, 0x40, 0xFF },
    { 0x80, 0x80, 0x80, 0xFF },
    { 0x40, 0x40, 0x40, 0xFF },
};

static void win_make_decorations(drv_video_window_t *w)
{
    int sz = sizeof(brdr)/sizeof(rgba_t);
    drv_video_window_t *w2 = do_drv_video_window_create(w->xsize+sz*2, w->ysize+sz*2);

    w2->x = w->x-sz; w2->y = w->y-sz;
    w2->bg = w->bg;

    w2->flags = 0;

    drv_video_window_fill( w2, w2->bg );
    window_basic_border( w2, brdr, sz );
    drv_video_winblt(w2);
    drv_video_window_free(w2);

}


drv_video_window_t *
drv_video_window_create(
                        int xsize, int ysize,
                        int x, int y,
                        rgba_t bg )
{
    drv_video_window_t *w = do_drv_video_window_create(xsize, ysize);

    drv_video_window_init( w, xsize, ysize, x, y, bg );

    //drv_video_window_t *w2 = do_drv_video_window_create(xsize+sz*2, ysize+sz*2);

    /*
    w->x = x; w->y = y;
    w->bg = bg;
    w->flags = WIN_DECORATED;

    win_make_decorations(w);
    */

    return w;
}


// for statically allocated ones
void
drv_video_window_init( drv_video_window_t *w,
                        int xsize, int ysize,
                        int x, int y,
                        rgba_t bg )
{
    w->xsize = xsize;
    w->ysize = ysize;

    w->li = w->ti = w->ri = w->bi = 0;

    w->flags = WIN_DECORATED;
    w->generation = 0;

    //int sz = sizeof(brdr)/sizeof(rgba_t);
    //drv_video_window_t *w2 = do_drv_video_window_create(xsize+sz*2, ysize+sz*2);

    w->x = x; w->y = y;
    //w2->x = x-sz; w2->y = y-sz;
    w->bg = bg;

    win_make_decorations(w);
    //drv_video_window_fill( w2, bg );
    //window_basic_border( w2, brdr, sz );
    //drv_video_winblt(w2);
    //drv_video_window_free(w2);

    //return w;
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






void drv_video_window_preblit( drv_video_window_t *w )
{
    //window_basic_border( w, brdr, sizeof(brdr)/sizeof(rgba_t) );
    if( (w->generation != win_generation) )
    {
        w->generation = win_generation;
        if(w->flags & WIN_DECORATED)
            win_make_decorations(w);
    }
}


