#ifndef WINDOW_H
#define WINDOW_H

#include <phantom_types.h>
#include <queue.h>

#include <video/color.h>
#include <video/bitmap.h>
#include <event.h>

#include <kernel/pool.h>

#define VIDEO_T_IN_D 0
#define T_IN_D VIDEO_T_IN_D

#if NEW_WINDOWS
typedef pool_handle_t window_handle_t;
#else
typedef struct drv_video_window * window_handle_t;
#endif


#define WFLAG_WIN_DECORATED             (1<<0)
// This is temp win, not included in allwindows list
#define WFLAG_WIN_NOTINALL            	(1<<1)
// Don't bring focus to this window
#define WFLAG_WIN_NOFOCUS               (1<<2)
// Don't free window pixels
#define WFLAG_WIN_NOPIXELS              (1<<3)


#define WSTATE_WIN_FOCUSED              (1<<0)
#define WSTATE_WIN_DRAGGED              (1<<1)
#define WSTATE_WIN_VISIBLE              (1<<2)

// -----------------------------------------------------------------------
// New windows (in work)
// -----------------------------------------------------------------------

typedef struct _phantom_window
{
    int                 xsize; // physical
    int                 ysize;

    int                 x, y, z; // On screen
    int                 dx, dy;  // Drag base (see titleMouseEventProcessor)

    //int                 unused_generation; // used to redraw self and borders on global events
    int                 flags; // Not supposed to change during window's life
    int                 state; // Can change anytime

    //queue_chain_t       chain; // All windows are on this chain

    int                 li, ti, ri, bi; // insets

    rgba_t              bg; // background color

    const char*         title;

    //queue_head_t        events; // Incoming events
    //volatile int        events_count; // To prevent overfill of dead window q

    int                 stall; // True if event queue is overloaded and events are being lost

    /*!
     * Called from main event dispatch thread after event is placed to
     * window event queue. Supposed to process event or trigger waiting
     * thread to do that.
     */
    int                 (*inKernelEventProcess)( window_handle_t, struct ui_event *e );

    tid_t               owner;

    //pool_handle_t       w_title; // child window - title
    //pool_handle_t       w_decor; // child window - decorations
    //pool_handle_t       w_owner; // my parent window

    // bitmap itself
    rgba_t       	*pixel;
} window_t;

pool_handle_t 	w_create( int xsize, int ysize );
void 		w_resize( pool_handle_t h, int xsize, int ysize );
void 		w_moveto( pool_handle_t h, int x, int y );
void 		w_blt( pool_handle_t h );
void		w_set_visible( pool_handle_t h, int v );

//void 		w_to_top(pool_handle_t h);
//void 		w_to_bottom(pool_handle_t h);

void    	w_clear( pool_handle_t h );
void    	w_fill( pool_handle_t h, rgba_t color );
void 		w_fill_rect( pool_handle_t h, rgba_t color, rect_t r );
//void		w_pixel( pool_handle_t h, int x, int y, rgba_t color );
//void    	w_draw_bitmap( pool_handle_t h, int x, int y, drv_video_bitmap_t *bmp );

void 		w_draw_h_line( pool_handle_t h, rgba_t color, int x, int y, int len );
void 		w_draw_v_line( pool_handle_t h, rgba_t color, int x, int y, int len );
void 		w_draw_line( pool_handle_t h, rgba_t color, int x, int y, int x2, int y2 );



#if !NEW_WINDOWS
// -----------------------------------------------------------------------
// Old windows
// -----------------------------------------------------------------------


// SNAPSHOT WARNING: this structure is used in snapshotted objects. If you change it, old snapshots are invalid. Be careful.
typedef struct drv_video_window
{
    int                 xsize; // physical
    int                 ysize;

    int                 x, y, z; // On screen
    int                 dx, dy;  // Drag base (see titleMouseEventProcessor)

    //int                 unused_generation; // used to redraw self and borders on global events
    int                 flags; // Not supposed to change during window's life
    int                 state; // Can change anytime

    queue_chain_t       chain; // All windows are on this chain

    int                 li, ti, ri, bi; // insets

    rgba_t              bg; // background color

    const char*         title;

    queue_head_t        events; // Incoming events
    volatile int        events_count; // To prevent overfill of dead window q

    int                 stall; // True if event queue is overloaded and events are being lost

    /*!
     * Called from main event dispatch thread after event is placed to
     * window event queue. Supposed to process event or trigger waiting
     * thread to do that.
     */
    int                 (*inKernelEventProcess)( struct drv_video_window *w, struct ui_event *e );

    tid_t               owner;

#if !VIDEO_T_IN_D
    struct drv_video_window     *w_title; // child window - title
#endif
    struct drv_video_window     *w_decor; // child window - decorations
    struct drv_video_window     *w_owner; // my parent window

    // bitmap itself
    rgba_t       	pixel[];
} drv_video_window_t;

// returns nonzero if rect is out of window
int    rect_win_bounds( rect_t *r, drv_video_window_t *w );

// returns nonzero if point is in window
int    point_in_win( int x, int y, drv_video_window_t *w );


// ------------------------------------------------------------------------
// Window interface - old, to be killed
// ------------------------------------------------------------------------

// malloc value to create drv_video_window_t
static __inline__ int drv_video_window_bytes( int xsize, int ysize ) { return (sizeof(rgba_t) * xsize * ysize) + sizeof(drv_video_window_t); }
static __inline__ int drv_video_bitmap_bytes( int xsize, int ysize ) { return (sizeof(rgba_t) * xsize * ysize) + sizeof(drv_video_bitmap_t); }


// dynamic allocation
drv_video_window_t *drv_video_window_create(int xsize, int ysize, int x, int y, rgba_t bg, const char* title );
// free dynamically allocated window
void 	drv_video_window_free(drv_video_window_t *w);

// init for statically allocated ones
void 	drv_video_window_init( drv_video_window_t *w, int xsize, int ysize, int x, int y, rgba_t bg );
// destroy for statically allocated ones
void 	drv_video_window_destroy(drv_video_window_t *w);



void 	drv_video_window_to_top(drv_video_window_t *w);
void 	drv_video_window_to_bottom(drv_video_window_t *w);


void    drv_video_window_clear( drv_video_window_t *win );
void    drv_video_window_fill( drv_video_window_t *win, rgba_t color );

void 	drv_video_window_fill_rect( drv_video_window_t *win, rgba_t color, rect_t r );

void	drv_video_window_pixel( drv_video_window_t *w, int x, int y, rgba_t color );


void    drv_video_window_draw_bitmap( drv_video_window_t *w, int x, int y, drv_video_bitmap_t *bmp );




void    drv_video_window_draw_line( drv_video_window_t *w,
                                 int x1, int y1, int x2, int y2, rgba_t c);

void    drv_video_window_fill_ellipse( drv_video_window_t *w,
                                 int x,int y,int lx,int ly,
                                 rgba_t c);

void    drv_video_window_fill_box( drv_video_window_t *w,
                                 int x,int y,int lx,int ly,
                                 rgba_t c);

void    drv_video_window_draw_box( drv_video_window_t *w,
                                 int x,int y,int lx,int ly,
                                 rgba_t c);


void    drv_video_window_move( drv_video_window_t *w, int x, int y );

//! Warning - caller must check buffer size!
void    drv_video_window_resize( drv_video_window_t *w, int xsize, int ysize );

void    drv_video_window_set_title( drv_video_window_t *w,  const char *title );

void    drv_video_window_get_bounds( drv_video_window_t *w, rect_t *out );






#endif // !new win

extern queue_head_t         allwindows;
extern window_handle_t      focused_window;



window_handle_t drv_video_next_window(window_handle_t curr);



// -----------------------------------------------------------------------
// Legacy?
// -----------------------------------------------------------------------


#if !NEW_WINDOWS

#if !USE_ONLY_INDIRECT_PAINT

	
void drv_video_winblt( drv_video_window_t *from );
static __inline__ void drv_video_window_update( drv_video_window_t *w ) { drv_video_winblt( w ); }

#else

static __inline__ void drv_video_window_update( drv_video_window_t *w ) 
{
    event_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, w );
}

#endif // USE_ONLY_INDIRECT_PAINT


/**
 *
 * Replicates src to dest. src has one horiz. line of srcSize pixels.
 * nSteps is number of replication steps vertically.
 *
**/

void replicate2window_ver( drv_video_window_t *dest, int destX, int destY,
                       int nSteps, const rgba_t *src, int srcSize );


/**
 *
 * Replicates src to dest. src has one vert. line of srcSize pixels.
 * nSteps is number of times to draw src horizontally.
 *
**/

void replicate2window_hor( drv_video_window_t *dest, int destX, int destY,
                       int nSteps, const rgba_t *src, int srcSize );


void window_basic_border( drv_video_window_t *dest, const rgba_t *src, int srcSize );

#endif


#endif // WINDOW_H
