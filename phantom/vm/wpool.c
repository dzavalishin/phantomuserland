#if 1
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

//#include <drv_video_screen.h>
#include <video/screen.h>

#include <video/window.h>
#include <video/internal.h>
#include <video/vops.h>


#include <assert.h>
#include <kernel/pool.h>
#include <kernel/libkern.h>
#include <phantom_libc.h>

#if !NEW_WINDOWS
void init_new_windows(void)
{
}
#endif

#if NEW_WINDOWS

#define DOW(__h,code) ({ window_t *w = pool_get_el(wp,__h); {code} pool_release_el( wp, __h ); })


static pool_t *wp;

static void * 	do_w_create(void *arg);
static void  	do_w_destroy(void *arg);

static void  	do_w_resize( window_t *w, int xsize, int ysize );

static void 	do_w_blt( window_t *w );

static void 	do_w_repaint_all();


void init_new_windows(void)
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




static void do_w_blt( window_t *w )
{
    scr_bitblt( w->pixel, w->x, w->y, w->xsize, w->ysize, w->z, 0 );
}


void w_blt(pool_handle_t h)
{
    //window_t *w = pool_get_el(wp,h);
    //do_w_blt( w );
    //pool_release_el( wp, h );

    DOW(h, do_w_blt( w ); );
}

static errno_t wpaint(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;
    do_w_blt( el );
    return 0;
}

static void 	do_w_repaint_all()
{
    //errno_t e =
    pool_foreach( wp, wpaint, 0 );
    //rgba2rgba_zbreplicate( struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos )
}
// -----------------------------------------------------------------------
// State/pos
// -----------------------------------------------------------------------
/*
void w_resize(pool_handle_t h, int xsize, int ysize)
{
    DOW(h, do_w_resize( w, xsize, ysize ); );
}

*/
void w_set_visible( pool_handle_t h, int v )
{
    window_t *w = pool_get_el(wp,h);

    if( v )     w->state |= WSTATE_WIN_VISIBLE;
    else     	w->state &= ~WSTATE_WIN_VISIBLE;

    // TODO event
    if( v ) do_w_blt( w );
    else
    {
        scr_zbuf_reset_square( w->x, w->y, w->xsize, w->ysize );
        do_w_repaint_all();
    }

    pool_release_el( wp, h );
}
#endif




#if NEW_WINDOWS
void w_moveto( pool_handle_t h, int x, int y )
{
    window_t *w = pool_get_el(wp,h);

    int v = w->state & WSTATE_WIN_VISIBLE;

    if( v )
    {
        scr_zbuf_reset_square( w->x, w->y, w->xsize, w->ysize );
        scr_zbuf_reset_square( x, y, w->xsize, w->ysize );
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
        scr_zbuf_reset_square( w->x, w->y, w->xsize, w->ysize );

    w->z = zorder;
    if( down )
        do_w_repaint_all();
    else
        do_w_blt( w );

    pool_release_el( wp, h );
}
#endif
// -----------------------------------------------------------------------
// Rect/point in win
// -----------------------------------------------------------------------


int rect_w_bounds( rect_t *r, window_handle_t w )
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


int point_in_w( int x, int y, window_t *w )
{
    if( x < w->x || y < w->y )
        return 0;

    if( x > w->x + w->xsize )
        return 0;

    if( y > w->y + w->ysize )
        return 0;

    return 1;
}

// -----------------------------------------------------------------------
// Output
// -----------------------------------------------------------------------

#if NEW_WINDOWS
static void do_w_fill( window_t *w, rgba_t color )
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


static void do_w_fill_rect( window_t *w, rgba_t color, rect_t r )
{
    if( rect_w_bounds( &r, w ) )
        return;

    int yp = r.y + r.ysize - 1;
    for( ; yp >= r.y; yp-- )
    {
        rgba_t *dst = w->pixel + yp*w->xsize + r.x;
        rgba2rgba_replicate( dst, &color, r.xsize );
    }
}

void w_fill_rect( pool_handle_t h, rgba_t color, rect_t r )
{
    DOW(h, do_w_fill_rect( w, color, r ); do_w_blt( w ); );
}

static void
do_w_pixel( window_t *w, int x, int y, rgba_t color )
{
    rect_t r;
    r.x = x;
    r.y = y;
    r.xsize = r.ysize = 1;

    if( rect_w_bounds( &r, w ) )
        return;

    rgba_t *dst = w->pixel + y*w->xsize + x;
    *dst = color;
}

void w_pixel( pool_handle_t h, int x, int y, rgba_t color )
{
    DOW(h, do_w_pixel( w, x, y, color ); );
}
#endif



static void do_w_draw_h_line( window_handle_t w, rgba_t color, int x, int y, int len )
{
    rect_t r;

    r.x = x;
    r.y = y;
    r.xsize = len;
    r.ysize = 1;

    if( rect_w_bounds( &r, w ) )
        return;

    rgba_t *dst = w->w_pixel + r.y*w->xsize + r.x;
    rgba2rgba_replicate( dst, &color, r.xsize );
}

#if NEW_WINDOWS
void w_draw_h_line( window_handle_t h, rgba_t color, int x, int y, int len )
{
    DOW(h, do_w_draw_h_line( w, color, x, y, len ); do_w_blt( w ); );
}
#else
void w_draw_h_line( window_handle_t h, rgba_t color, int x, int y, int len )
{
    do_w_draw_h_line( h, color, x, y, len ); 
}
#endif

#if NEW_WINDOWS
static void do_w_draw_v_line( window_t *w, rgba_t color, int x, int y, int len )
{
    rect_t r;

    r.x = x;
    r.y = y;
    r.xsize = 1;
    r.ysize = len;

    if( rect_w_bounds( &r, w ) )
        return;

    int yp = r.y + r.ysize - 1;
    for( ; yp >= r.y; yp-- )
    {
        rgba_t *dst = w->pixel + yp*w->xsize + r.x;
        *dst = color;
    }
}

void w_draw_v_line( pool_handle_t h, rgba_t color, int x, int y, int len )
{
    DOW(h, do_w_draw_v_line( w, color, x, y, len ); do_w_blt( w ); );
}
#endif

#if NEW_WINDOWS
static void do_w_draw_rect( window_t *w, rgba_t c,
                                 int x,int y,int lx, int ly
                                 )
{
    //do_w_draw_line( w, x,      y,      x+lx-1, y,      c);
    //do_w_draw_line( w, x,      y+ly-1, x+lx-1, y+ly-1, c);
    //do_w_draw_line( w, x,      y,      x,      y+ly-1, c);
    //do_w_draw_line( w, x+lx-1, y,      x+lx-1, y+ly-1, c);

    do_w_draw_h_line( w, c, x,      y,      lx );
    do_w_draw_h_line( w, c, x,      y+ly,   lx );
    do_w_draw_v_line( w, c, x,      y,      ly );
    do_w_draw_v_line( w, c, x+lx-1, y,      ly );
}

void w_draw_rect( pool_handle_t h, rgba_t c,
                                 int x, int y, int lx, int ly
                                 )
{
    DOW(h, do_w_draw_rect( w, c, x, y, lx, ly );  );
}
#endif

// -----------------------------------------------------------------------
// Line
// -----------------------------------------------------------------------

#if NEW_WINDOWS

// SLOOOW! Checks bounds on each pixel

#define _PLOT(w,x,y,c) do {\
    if((x) > 0 && (y) > 0 && (x) < (w)->xsize && (y) <= (w)->ysize)\
    (w)->pixel[(x)+(y)*(w)->xsize] = c;\
    } while(0)\

// fast, but can DAMAGE MEMORY - check bounds before calling

#define _UNCH_PLOT(w,x,y,c) do {\
    (w)->pixel[(x)+(y)*(w)->xsize] = c;\
    } while(0)\


static inline int SGN(int v) { return v == 0 ? 0 : ( (v > 0) ? 1 : -1); }



static void do_w_draw_line( window_t *w, rgba_t c, int x1, int y1, int x2, int y2)
{
    int a,x,y;
    int i;
    int d;

    int dx = x2-x1;
    int dy = y2-y1;

    //if (!dx) { vline(w,x1,y1,y2,c); return }
    //if (!dy) { hline(w,x1,y1,x2,c); return }

    if (abs(dx) > abs(dy))
    {
        d=SGN(dx);

        a = (dx == 0) ? 0 : (dy<<16) / abs(dx);

        for( i=x1,y=32768+(y1<<16); i != x2; i += d,y += a )
            _PLOT(w,i,(int)(y>>16),c);
    }
    else
    {
        d=SGN(dy);

        a = (dy == 0) ? 0 : ((dx<<16) / abs(dy));

        for( i=y1,x=32768+(x1<<16); i != y2; i += d,x += a )
            _PLOT(w,(int)(x>>16),i,c);
    }

    _PLOT(w,x2,y2,c);

}


//#if NEW_WINDOWS
void w_draw_line( pool_handle_t h, rgba_t color, int x, int y, int x2, int y2 )
{
    DOW(h, do_w_draw_line( w, color, x, y, x2, y2 ); do_w_blt( w ); );
}
#endif


// -----------------------------------------------------------------------
// test
// -----------------------------------------------------------------------

#if NEW_WINDOWS
void new_videotest()
{
    pool_handle_t wb = w_create( scr_get_xsize(), scr_get_ysize() );
    w_clear( wb );
    w_set_z_order( wb, 0 );

    w_blt( wb );

    printf("create win\n");
    pool_handle_t w1 = w_create( 200, 300 );
    printf("fill win\n");
    w_fill( w1, COLOR_LIGHTRED );

    pool_handle_t w2 = w_create( 200, 300 );
    w_moveto( w2, 100, 100 );
    w_fill( w2, COLOR_LIGHTGREEN );
    w_set_z_order( w2, 30 );

    w_blt( w1 );
    w_blt( w2 );

    //scr_zbuf_reset_z(50);

    w_moveto( w2, 50, 50 );

    rect_t r;

    r.x = r.y = 50;
    r.xsize = r.ysize = 20;

    w_fill_rect( w1, COLOR_BLACK, r );

    w_draw_h_line( w1, COLOR_YELLOW, 50, 50, 20 );
    w_draw_v_line( w1, COLOR_YELLOW, 50, 50, 20 );
    w_draw_line( w1, COLOR_RED, 50, 50, 70-1, 70-1 );

    w_draw_rect( w1, COLOR_BLACK,
                 100, 50, 30, 40 );

    w_scroll_hor( w1, 60, 60, 80, 80, 10 );
    w_draw_rect( w1, COLOR_BLACK, 60, 60, 20, 20 );

    w_blt( w1 );
    w_blt( w2 );

    //scr_zbuf_paint();
}
#endif

#endif
