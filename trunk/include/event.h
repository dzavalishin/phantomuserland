
#include <queue.h>

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

    struct drv_video_window *   focus; // Who is in focus now

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




