#ifndef WINDOW_H
#define WINDOW_H

#include <phantom_types.h>
#include <queue.h>

#include <video/color.h>
#include <event.h>

#include <kernel/pool.h>


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
    int         	xsize; // physical
    int 		ysize;

    int                 x, y, z; // On screen
    int                 dx, dy;  // Drag base (see titleMouseEventProcessor)

    //int                 unused_generation; // used to redraw self and borders on global events
    int                 flags; // Not supposed to change during window's life
    int                 state; // Can change anytime

    //queue_chain_t       chain; // All windows are on this chain

    int                 li, ti, ri, bi; // insets

    rgba_t       	bg; // background color

    const char* 	title;

    //queue_head_t        events; // Incoming events
    //volatile int        events_count; // To prevent overfill of dead window q

    int                 stall; // True if event queue is overloaded and events are being lost

    /*!
     * Called from main event dispatch thread after event is placed to
     * window event queue. Supposed to process event or trigger waiting
     * thread to do that.
     */
    int                 (*inKernelEventProcess)( struct drv_video_window *w, struct ui_event *e );

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



// -----------------------------------------------------------------------
// Old windows
// -----------------------------------------------------------------------


// SNAPSHOT WARNING: this structure is used in snapshotted objects. If you change it, old snapshots are invalid. Be careful.
typedef struct drv_video_window
{
    int         	xsize; // physical
    int 		ysize;

    int                 x, y, z; // On screen
    int                 dx, dy;  // Drag base (see titleMouseEventProcessor)

    //int                 unused_generation; // used to redraw self and borders on global events
    int                 flags; // Not supposed to change during window's life
    int                 state; // Can change anytime

    queue_chain_t       chain; // All windows are on this chain

    int                 li, ti, ri, bi; // insets

    rgba_t       	bg; // background color

    const char* 	title;

    queue_head_t        events; // Incoming events
    volatile int	events_count; // To prevent overfill of dead window q

    int                 stall; // True if event queue is overloaded and events are being lost

    /*!
     * Called from main event dispatch thread after event is placed to
     * window event queue. Supposed to process event or trigger waiting
     * thread to do that.
     */
    int                 (*inKernelEventProcess)( struct drv_video_window *w, struct ui_event *e );

    tid_t               owner;

    struct drv_video_window     *w_title; // child window - title
    struct drv_video_window     *w_decor; // child window - decorations
    struct drv_video_window     *w_owner; // my parent window

    // bitmap itself
    rgba_t       	pixel[];
} drv_video_window_t;

// returns nonzero if rect is out of window
int rect_win_bounds( rect_t *r, drv_video_window_t *w );

// returns nonzero if point is in window
int point_in_win( int x, int y, drv_video_window_t *w );


#endif // WINDOW_H
