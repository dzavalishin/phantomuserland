/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Window system event.
 *
 *
**/

#ifndef EVENT_H
#define EVENT_H

#ifndef VCONFIG_H
#include <video/vconfig.h>
#endif // VCONFIG_H

#include <queue.h>
#include <video/rect.h>


struct ui_event
{
    queue_chain_t               echain; // on q

    int				type; // UI_EVENT_TYPE_
    int				extra; // used for button event - button id

    time_t	 	       	time;

    int                         abs_x;
    int                         abs_y;
    int                         abs_z; // z of clicked win

    int                         rel_x;
    int                         rel_y;
    int                         rel_z;

    struct drv_video_window *   focus; // Target window

    // Shift, alt, etc - actual for key and mouse events
    int 			modifiers; 

    union {

        struct {
            int         	vk;     // key code
            int         	ch;     // unicode char
        } k;

        struct {
            int         	buttons;	// Bits correspond to mouse buttons
            int         	clicked;       	// Bits correspond to mouse buttons clicked this time
            int         	released;      	// Bits correspond to mouse buttons released this time
        } m;

        // WIN and GLOBAL events
        struct {
            int         	info;           // UI_EVENT_WIN_
            rect_t              rect;           // If relevant...
        } w;


        char                    fill[32];
    };

};

typedef struct ui_event ui_event_t;

#define UI_EVENT_TYPE_MOUSE     	(1<<0)
#define UI_EVENT_TYPE_KEY     		(1<<1)
#define UI_EVENT_TYPE_WIN     		(1<<2)
// Global events are usually converted to multiple WIN events
#define UI_EVENT_TYPE_GLOBAL   		(1<<3)


#define UI_MODIFIER_KEYUP               (1<<0) // Else down
#define UI_MODIFIER_SHIFT               (1<<1) 
#define UI_MODIFIER_LSHIFT              (1<<2)
#define UI_MODIFIER_RSHIFT              (1<<3)
#define UI_MODIFIER_CAPSLOCK            (1<<4)
#define UI_MODIFIER_CTRL                (1<<5)
#define UI_MODIFIER_LCTRL               (1<<6)
#define UI_MODIFIER_RCTRL               (1<<7)
#define UI_MODIFIER_ALT                 (1<<8)
#define UI_MODIFIER_LALT                (1<<9)
#define UI_MODIFIER_RALT                (1<<10)
#define UI_MODIFIER_WIN                 (1<<11)
#define UI_MODIFIER_LWIN                (1<<12)
#define UI_MODIFIER_RWIN                (1<<13)
#define UI_MODIFIER_SCRLOCK             (1<<14)
#define UI_MODIFIER_NUMLOCK             (1<<15)

#define UI_MOD_Y_N(__mod, __y, __n ) \
    ({ int m = (__mod); int yn; \
    yn = m & (__y);  \
    if( m & (__n) ) yn = 0; \
    yn; \
    })


// Key is down and just control is pressed
#define UI_MOD_CTRL_DOWN(__mod) \
    ({ int m = (__mod); int yn; \
    yn = m & UI_MODIFIER_CTRL;  \
    if( m & UI_MODIFIER_SHIFT ) yn = 0; \
    if( m & UI_MODIFIER_ALT ) yn = 0; \
    if( m & UI_MODIFIER_KEYUP ) yn = 0; \
    yn; \
    })

// Key is down and just control is pressed
#define UI_MOD_ALT_DOWN(__mod) \
    ({ int m = (__mod); int yn; \
    yn = m & UI_MODIFIER_ALT;  \
    if( m & UI_MODIFIER_SHIFT ) yn = 0; \
    if( m & UI_MODIFIER_CTRL ) yn = 0; \
    if( m & UI_MODIFIER_KEYUP ) yn = 0; \
    yn; \
    })


#define UI_EVENT_WIN_GOT_FOCUS          1
#define UI_EVENT_WIN_LOST_FOCUS         2
#define UI_EVENT_WIN_DESTROYED          3
#define UI_EVENT_WIN_REDECORATE         4 //! Repaint titlebar
#define UI_EVENT_WIN_REPAINT            5 //! Repaint all
#define UI_EVENT_WIN_BUTTON             6


#define UI_EVENT_GLOBAL_REPAINT_RECT    (0xFF00|1) //! Repaint all windows that intersect w. rect

/*/ w.info for UI_EVENT_TYPE_MOUSE
#define UI_EVENT_MOU_CLICK_1            1
#define UI_EVENT_MOU_PRESS_1            2
#define UI_EVENT_WIN_RELEASE_1          3
#define UI_EVENT_MOU_CLICK_2            11
#define UI_EVENT_MOU_PRESS_2            12
#define UI_EVENT_WIN_RELEASE_2          13
*/

#ifdef KERNEL

// TODO move to kern/event.h and rest to video/event.h

//! Construct mouse event 
void ev_make_mouse_event( struct ui_event *e, int x, int y, int buttons );

//! Put mouse event onto the main e q
void ev_q_put_mouse( int x, int y, int buttons );

//! Put key event onto the main e q
void ev_q_put_key( int vkey, int ch, int modifiers );

//! Put window event onto the main e q
void ev_q_put_win( int x, int y, int info, struct drv_video_window *focus );

//! Put any event onto the main e q
//void ev_q_put_e( ui_event_t *in );


//! Put global event onto the main e q, supposed that event system
//! will decide which windows have to receive this event.
//void event_q_put_global( int x, int y, int info );
void ev_q_put_global( ui_event_t *e );

//! Put any type of event onto the main e q - BE CAREFUL
//! NB - not forget to set focus!
void ev_q_put_any( ui_event_t *ie );



//! Get next event for this window
int ev_w_get_event( struct drv_video_window *w, struct ui_event *e, int wait );

//! Returns nonzero if processed event
int defaultWindowEventProcessor( struct drv_video_window *w, struct ui_event *e );

//! Just for debug
int ev_get_n_events_in_q(void); 

#endif // KERNEL


#endif // EVENT_H

