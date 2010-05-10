
#include <queue.h>

//! Max number of unprocessed events in window queue. Extra events will be thrown away.
#define MAX_WINDOW_EVENTS 512

struct ui_event
{
    queue_chain_t               chain; // on q

    int				type; // UI_EVENT_TYPE_

    time_t	 	       	time;

    int                         abs_x;
    int                         abs_y;
    int                         abs_z; // z of clicked win

    int                         rel_x;
    int                         rel_y;
    int                         rel_z;

    struct drv_video_window *   focus; // Target window

    union {

        struct {
            int         	vk;     // key code
            int         	ch;     // unicode char
        } k;

        struct {
            int         	buttons;	// Bits correspond to buttons

        } m;

        struct {
            int         info;           // UI_EVENT_WIN_
        } w;

    };

};


#define UI_EVENT_TYPE_MOUSE     	(1<<0)
#define UI_EVENT_TYPE_KEY     		(1<<1)
#define UI_EVENT_TYPE_WIN     		(1<<2)



#define UI_EVENT_WIN_GOT_FOCUS          1
#define UI_EVENT_WIN_LOST_FOCUS         2
#define UI_EVENT_WIN_DESTROYED          3
#define UI_EVENT_WIN_REDECORATE         4 //! Repaint titlebar
#define UI_EVENT_WIN_REPAINT            5 //! Repaint all





//! Put mouse event onto the main e q
void event_q_put_mouse( int x, int y, int buttons );

//! Put key event onto the main e q
void event_q_put_key( int vkey, int ch );

//! Put window event onto the main e q
void event_q_put_win( int x, int y, int info, struct drv_video_window *focus );


