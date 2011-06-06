#ifndef WINDOW_H
#define WINDOW_H

#include <phantom_types.h>
#include <queue.h>

#include <video/color.h>
#include <event.h>


#define WFLAG_WIN_DECORATED             (1<<0)
// This is temp win, not included in allwindows list
#define WFLAG_WIN_NOTINALL            	(1<<1)
// Don't bring focus to this window
#define WFLAG_WIN_NOFOCUS               (1<<2)
										
#define WSTATE_WIN_FOCUSED              (1<<0)
#define WSTATE_WIN_DRAGGED              (1<<1)

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
