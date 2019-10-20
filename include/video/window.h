/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Phantom OS team
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Window.
 *
**/

#ifndef WINDOW_H
#define WINDOW_H

#ifndef VCONFIG_H
#include <video/vconfig.h>
#endif // VCONFIG_H

#include <phantom_types.h>
#include <queue.h>

#include <video/color.h>
#include <video/bitmap.h>
#include <event.h>

#include <kernel/pool.h>

#define VIDEO_T_IN_D 0


typedef struct drv_video_window * window_handle_t;
typedef pool_handle_t taskbar_handle_t;


// Win flags supposed to stay the same

#define WFLAG_WIN_DECORATED             (1<<0)
// This is temp win, not included in allwindows list
#define WFLAG_WIN_NOTINALL            	(1<<1)
// Don't bring focus to this window
#define WFLAG_WIN_NOFOCUS               (1<<2)
// Don't free window pixels
#define WFLAG_WIN_NOPIXELS              (1<<3)
// Can not treat all pixels alpha as 0xFF
#define WFLAG_WIN_NOTOPAQUE             (1<<4)
// Must be on top of all others
#define WFLAG_WIN_ONTOP                 (1<<5)
// Double buffered window
#define WFLAG_WIN_DOUBLEBUF             (1<<6)
// In double buffered mode window is supposed to completely
// repaint itself each buffer swap. No buffer copy occures.
#define WFLAG_WIN_FULLPAINT             (1<<7)
// Don't bring focus to this window by key event (ctrl-tab)
#define WFLAG_WIN_NOKEYFOCUS            (1<<8)
// Put this window to task bar - it is a main application window - UNIMPL yet
#define WFLAG_WIN_TASK_BAR              (1<<9)
// For menu - hide if focus is gone
#define WFLAG_WIN_HIDE_ON_FOCUS_LOSS    (1<<10)


#define WIN_HAS_FLAG(__w,__f) ((__w)->flags & (__f))

// Win state can change frequently

#define WSTATE_WIN_FOCUSED              (1<<0)
#define WSTATE_WIN_DRAGGED              (1<<1)
#define WSTATE_WIN_VISIBLE              (1<<2)
#define WSTATE_WIN_ROLLEDUP             (1<<3)

// No one is above
#define WSTATE_WIN_UNCOVERED            (1<<8)
// Pixels live in graphics device's framebuf - can use hw blitter
#define WSTATE_WIN_INFB                 (1<<9)


// Default button ids
#define WBUTTON_SYS(id) (0xFFFF|((id)<<16))

#define WBUTTON_SYS_ROLLUP 				WBUTTON_SYS(0)
#define WBUTTON_SYS_CLOSE 				WBUTTON_SYS(1)



// -----------------------------------------------------------------------
// Window struct
// -----------------------------------------------------------------------

/**
 * 
 * Main structure representing window.
 * 
 * SNAPSHOT WARNING: this structure is used in snapshotted objects. 
 * If you change it, old snapshots are invalid. Be careful.
 * 
 * 
 * 
**/
typedef struct drv_video_window
{
    int                         xsize;          //< Pixels
    int                         ysize;

    int                         x, y, z;         //< On screen
    int                         dx, dy;          //< Drag base (see titleMouseEventProcessor)

    int                         flags;           //< Not supposed to change during window's life
    int                         state;           //< Can change anytime

    queue_chain_t               chain;           //< All windows are on this chain

    int                         li, ti, ri, bi;  //< insets - unused?

    rgba_t                      bg;              //< background color

    const char*                 title;

    queue_head_t                events;          //< Incoming events
    volatile int                events_count;    //< To prevent overfill of dead window q

    int                         stall;           //< True if event queue is overloaded and events are being lost

    /*!
     * Called from main event dispatch thread after event is placed to
     * window event queue. Supposed to process event or trigger waiting
     * thread to do that.
     */
    int                         (*inKernelEventProcess)( struct drv_video_window *w, struct ui_event *e );  
    hal_sem_t                   *eventDeliverSema; //< If not null - signalled on new event in win q

    tid_t                       owner;

#if !VIDEO_T_IN_D
    struct drv_video_window     *w_title;        //< Child window - title
#endif
    struct drv_video_window     *w_decor;        //< Child window - decorations
    struct drv_video_window     *w_owner;        //< My parent window - i am decor/title

    pool_t                      *controls;       //< Attached UI controls

    taskbar_handle_t            task_bar_h;      //< Me it task bar
    window_handle_t             context_menu;    //< Default window context menu

    rgba_t       	            *r_pixel;        //< read ptr - for blit to screen
    rgba_t       	            *w_pixel;        //< write ptr - for painting
    rgba_t       	            *buf[2];         //< 1st/2nd halves ptrs for dbl buf switch
#if VIDEO_DOUBLE_BUF
#else
//#define r_pixel pixels
//#define w_pixel pixels
#endif

    // bitmap itself
    rgba_t       	            *bitmap;
} drv_video_window_t;



// returns nonzero if rect is out of window
int    rect_win_bounds( rect_t *r, drv_video_window_t *w );

// returns nonzero if point is in window
int    point_in_win( int x, int y, drv_video_window_t *w );


// ------------------------------------------------------------------------
// Window interface
// ------------------------------------------------------------------------

//! malloc value to create drv_video_window_t
// * 2 is for double buffered mode
static __inline__ int drv_video_window_bytes( int xsize, int ysize ) { return (sizeof(rgba_t) * xsize * ysize * 2) + sizeof(drv_video_window_t); }


// dynamic allocation
drv_video_window_t *drv_video_window_create(int xsize, int ysize, int x, int y, rgba_t bg, const char* title, int flags );
// free dynamically allocated window
void 	drv_video_window_free(drv_video_window_t *w);

// init for statically allocated ones
void 	drv_video_window_init( drv_video_window_t *w, void *pixels, int xsize, int ysize, int x, int y, rgba_t bg, int flags, const char *title );
// destroy for statically allocated ones
void 	drv_video_window_destroy(drv_video_window_t *w);



void 	w_to_top(window_handle_t w);
void 	w_to_bottom(window_handle_t w);


void    w_clear( window_handle_t win );
void    w_fill( window_handle_t win, rgba_t color );

void 	w_draw_rect( window_handle_t win, rgba_t color, rect_t r );
void 	w_fill_rect( window_handle_t win, rgba_t color, rect_t r );

//#define w_draw_pixel w_pixel

void	w_draw_pixel( window_handle_t w, int x, int y, rgba_t color );
void    w_draw_line( window_handle_t w, int x1, int y1, int x2, int y2, rgba_t c);
void    w_fill_ellipse( window_handle_t w, int x,int y,int lx,int ly, rgba_t c);
void    w_fill_box( window_handle_t w, int x,int y,int lx,int ly, rgba_t c);
void    w_draw_box( window_handle_t w, int x,int y,int lx,int ly, rgba_t c);
void    w_draw_bitmap( window_handle_t w, int x, int y, drv_video_bitmap_t *bmp );
void    w_draw_blend_bitmap( drv_video_window_t *w, int x, int y, drv_video_bitmap_t *bmp ); //< Draw with alpha blending


void    w_move( window_handle_t w, int x, int y );

errno_t w_scroll_hor( window_handle_t w, int x, int y, int xs, int ys, int s );
void    w_scroll_up( window_handle_t win, int npix, rgba_t color);

void    w_set_title( window_handle_t w,  const char *title );
void    w_get_bounds( window_handle_t w, rect_t *out );
void    w_set_visible( window_handle_t h, int v );
void	w_set_bg_color( window_handle_t w, rgba_t color );

/**
 * 
 * Resize window - tricky!
 * 
 * Warning - caller must check buffer size!
 * 
 * It is up to caller to be sure that window has enough 
 * place in its allocated memory for new size. If it is
 * not so, caller must allocate and pass new buffer.
 * 
 * Use drv_video_window_bytes() to find correct size of
 * video memory for window, add sizeof(window_handle_t)
 * 
 * \param[in] w           Window to resize
 * \param[in] xsize       New width
 * \param[in] ysize       New height
 * \param[in] new_buffer  Zero if old memory is big enough for a new size, or new buffer.
 * \param[in] clear       Do not attempt to save contents, clear with bg color.
 * 
**/
void    w_resize_ext( window_handle_t w, int xsize, int ysize, void *new_buffer, int clear );

// -----------------------------------------------------------------------
// Getters for state
// -----------------------------------------------------------------------


#define w_is_in_focus(w) ({ ((w)->state) & WSTATE_WIN_FOCUSED; })
#define w_is_visible(w) ({ ((w)->state) & WSTATE_WIN_VISIBLE; })

/// Is one of topmost - i.e. covered only by WFLAG_WIN_ONTOP ones
int w_is_top(drv_video_window_t *w);


// -----------------------------------------------------------------------
// Globals - TODO move away
// -----------------------------------------------------------------------

extern queue_head_t         allwindows;
extern window_handle_t      focused_window;

// -----------------------------------------------------------------------
// Task bar
// -----------------------------------------------------------------------

/**
 * 
 * Add window to task bar. Task bar will control window visibility.
 * Also, application can control image in task bar by changing
 * task bar icon with w_set_task_bar_icon.
 * 
**/
taskbar_handle_t w_add_to_task_bar( window_handle_t w );

taskbar_handle_t w_add_to_task_bar_icon( window_handle_t w, drv_video_bitmap_t *icon );

taskbar_handle_t w_add_to_task_bar_ext( window_handle_t w, drv_video_bitmap_t *icon,
        drv_video_bitmap_t *n_bmp,
        drv_video_bitmap_t *p_bmp,
        drv_video_bitmap_t *h_bmp );

void w_set_task_bar_icon( window_handle_t w, drv_video_bitmap_t *bmp );
//! Not impl yet - displays a marker with a number aside to icon top
void w_set_task_bar_note( window_handle_t w, int n_of_outstanding_events );
//void w_set_task_bar_menu( window_handle_t w, window_handle_t m );
errno_t w_remove_from_task_bar( window_handle_t w );


/// Add marker in taskbar telling that window needs attention
void w_add_notification( window_handle_t w, int count_to_add );
/// Remove marker. Also automativally called on bringing window to top
void w_reset_notification( window_handle_t w );

void w_set_task_bar_menu( window_handle_t w, window_handle_t m );

// -----------------------------------------------------------------------
// Internals
// -----------------------------------------------------------------------


window_handle_t drv_video_next_window(window_handle_t curr);

void win2blt_flags( u_int32_t *flags, const window_handle_t w );

// -----------------------------------------------------------------------
// Persistent mem support
// -----------------------------------------------------------------------

//! Called from object restart code to reinit window struct
//! contained in VM object. This func just clears pointers.
//! After calling this func you must reset all the required
//! fields and call w_restart_attach( w )
//! to add window to in-kernel lists and repaint it.
void            w_restart_init(window_handle_t w, void *pixels);

// Called from vm restart code to reattach window to win system
void            w_restart_attach( window_handle_t w );

// Get most bottom (background) window. Used to set scrren background pic.
window_handle_t w_get_bg_window(void);


// -----------------------------------------------------------------------
// Legacy?
// -----------------------------------------------------------------------


#if !NEW_WINDOWS

void w_update( drv_video_window_t *w );


#if !USE_ONLY_INDIRECT_PAINT
void drv_video_winblt( drv_video_window_t *from );
#endif // USE_ONLY_INDIRECT_PAINT

int w_titleWindowEventProcessor( window_handle_t w, struct ui_event *e );


/**
 *
 * Replicates src to dest. src has one horiz. line of srcSize pixels.
 * nSteps is number of replication steps vertically.
 *
**/

void w_replicate_ver( window_handle_t dest, int destX, int destY,
                       int nSteps, const rgba_t *src, int srcSize );


/**
 *
 * Replicates src to dest. src has one vert. line of srcSize pixels.
 * nSteps is number of times to draw src horizontally.
 *
**/

void w_replicate_hor( window_handle_t dest, int destX, int destY,
                       int nSteps, const rgba_t *src, int srcSize );


void window_basic_border( window_handle_t dest, const rgba_t *src, int srcSize, int isTitle );

#endif

void w_repaint_screen_part( drv_video_window_t *w, rect_t *todo );



#if 0
#if NEW_WINDOWS

typedef pool_handle_t window_handle_t;


// -----------------------------------------------------------------------
// New windows attempt - outdated
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
void            w_blt( pool_handle_t h );

void            w_draw_h_line( window_handle_t h, rgba_t color, int x, int y, int len );
void            w_draw_v_line( window_handle_t h, rgba_t color, int x, int y, int len );

//void 		w_moveto( pool_handle_t h, int x, int y );
//void		w_pixel( pool_handle_t h, int x, int y, rgba_t color );


#endif
#endif



#endif // WINDOW_H
