/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * (new) Windowing system internals and housekeeping.
 *
 *
**/

#include <drv_video_screen.h>
#include <assert.h>
#include <kernel/pool.h>
#include <kernel/libkern.h>
#include <phantom_libc.h>

#define DOW(__h,code) ({ window_t *w = pool_get_el(wp,__h); {code} pool_release_el( wp, __h ); })


static pool_t *wp;

static void * 	do_w_create(void *arg);
static void  	do_w_destroy(void *arg);

static void  	do_w_resize( window_t *w, int xsize, int ysize );

static void 	do_w_blt( window_t *w );

static void 	do_w_repaint_all();


void init_new_windows()
{
    wp = create_pool();
    wp->init = do_w_create;
    wp->destroy = do_w_destroy;
}


pool_handle_t w_create(int xsize, int ysize)
{
    pool_handle_t ret = pool_create_el( wp, 0 );

    window_t *w = pool_get_el(wp,ret);

    do_w_resize( w, xsize, ysize );


    pool_release_el( wp, ret );
    return ret;
}

static void * do_w_create(void *arg)
{
    window_t *w = calloc( sizeof(window_t), 1 );
    assert(w);
    // if(!w) return 0;

    //w->z = 0xFE; // quite atop
    w->z = 0xE0; // quite atop
    w->bg = COLOR_BLACK;

    //queue_init(&(w->events));
    //w->events_count = 0;

    w->inKernelEventProcess = defaultWindowEventProcessor;

    //w->w_title = -1;
    //w->w_decor = -1;
    //w->w_owner = -1;

    w->state |= WSTATE_WIN_VISIBLE;

    return w;
}

static void do_w_destroy(void *arg)
{
    window_t *w = arg;
    assert(w);

    if( (w->pixel != 0) && !(w->flags & WFLAG_WIN_NOPIXELS) )
        free(w->pixel);

    //if( w->w_title >= 0 )	pool_release_el( wp, w->w_title );
    //if( w->w_decor >= 0 )	pool_release_el( wp, w->w_decor );
    //if( w->w_owner >= 0 )	pool_release_el( wp, w->w_owner );

    free(w);
}

void w_destroy(pool_handle_t h)
{
    w_set_visible(h, 0);
    pool_release_el( wp, h ); // initial pool_create_el() sets refcount to 1, decrement it here
}


static void do_w_resize( window_t *w, int xsize, int ysize )
{
    assert(!(w->flags & WFLAG_WIN_NOPIXELS));

    rgba_t *npixel = calloc( xsize*ysize, sizeof(rgba_t) );


    if( w->pixel != 0 )
    {
        int mw = imin( xsize, w->xsize );
        int mh = imin( ysize, w->ysize );
        bitmap2bitmap(
		npixel, xsize, ysize, 0, 0,
		w->pixel, w->xsize, w->ysize, 0, 0,
		mw, mh );
        free(w->pixel);
    }


    w->pixel = npixel;

    w->xsize = xsize;
    w->ysize = ysize;
}


void w_resize(pool_handle_t h, int xsize, int ysize)
{
    DOW(h, do_w_resize( w, xsize, ysize ); );
}


static void do_w_blt( window_t *w )
{
    drv_video_bitblt( w->pixel, w->x, w->y, w->xsize, w->ysize, w->z);
}


void w_blt(pool_handle_t h)
{
    //window_t *w = pool_get_el(wp,h);
    //do_w_blt( w );
    //pool_release_el( wp, h );

    DOW(h, do_w_blt( w ); );
}

static errno_t wpaint(pool_t *pool, void *el, pool_handle_t handle)
{
    (void) pool;
    (void) handle;
    do_w_blt( el );
    return 0;
}

static void 	do_w_repaint_all()
{
    //errno_t e =
    pool_foreach( wp, wpaint );
    //rgba2rgba_zbreplicate( struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos )
}


void w_set_visible( pool_handle_t h, int v )
{
    window_t *w = pool_get_el(wp,h);

    if( v )     w->state |= WSTATE_WIN_VISIBLE;
    else     	w->state &= ~WSTATE_WIN_VISIBLE;

    // TODO event
    if( v ) do_w_blt( w );
    else
    {
        video_zbuf_reset_square( w->x, w->y, w->xsize, w->ysize );
        do_w_repaint_all();
    }

    pool_release_el( wp, h );
}


void do_w_fill( window_t *w, rgba_t color )
{
    int i = (w->xsize * w->ysize) - 1;
    for( ; i >= 0; i-- )
        w->pixel[i] = color;
}


void w_fill( pool_handle_t h, rgba_t color )
{
    DOW(h, do_w_fill( w, color ); do_w_blt( w ); );
}

void w_clear( pool_handle_t h )
{
    DOW(h, do_w_fill( w, w->bg ); do_w_blt( w ); );
}



void w_moveto( pool_handle_t h, int x, int y )
{
    window_t *w = pool_get_el(wp,h);

    int v = w->state & WSTATE_WIN_VISIBLE;

    if( v )
    {
        video_zbuf_reset_square( w->x, w->y, w->xsize, w->ysize );
        video_zbuf_reset_square( x, y, w->xsize, w->ysize );
    }

    w->x = x;
    w->y = y;

    if( v )
        do_w_repaint_all();

    pool_release_el( wp, h );
}

// just for test
void w_set_z_order( pool_handle_t h, int zorder )
{
    window_t *w = pool_get_el(wp,h);

    int down = w->z > zorder;
    if( down )
        video_zbuf_reset_square( w->x, w->y, w->xsize, w->ysize );

    w->z = zorder;
    if( down )
        do_w_repaint_all();
    else
        do_w_blt( w );

    pool_release_el( wp, h );
}



// -----------------------------------------------------------------------
// test
// -----------------------------------------------------------------------

void new_videotest()
{
    pool_handle_t wb = w_create( video_drv->xsize, video_drv->ysize );
    w_clear( wb );
    w_set_z_order( wb, 0 );

    printf("create win\n");
    pool_handle_t w1 = w_create( 200, 300 );
    printf("fill win\n");
    w_fill( w1, COLOR_LIGHTRED );

    pool_handle_t w2 = w_create( 200, 300 );
    w_moveto( w2, 100, 100 );
    w_fill( w2, COLOR_LIGHTGREEN );
    w_set_z_order( w2, 30 );

    //video_zbuf_reset_z(50);

    w_moveto( w2, 50, 50 );

    //video_zbuf_paint();
}



