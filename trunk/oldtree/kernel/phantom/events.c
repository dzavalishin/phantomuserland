#if 0
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Main win/ui event management code.
 *
 *
**/



#define DEBUG_MSG_PREFIX "events"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>
#include <kernel/config.h>
#include <kernel/mutex.h>

#include <threads.h>
#include <event.h>
#include <dev/key_event.h>
#include <queue.h>
#include <malloc.h>
#include <hal.h>
#include <string.h>
#include <console.h>
#include <wtty.h>
#include <time.h>

#include <video/internal.h>
#include <video/window.h>
#include <video/button.h>


void drv_video_window_explode_event(struct ui_event *e);
int drv_video_window_get_event( drv_video_window_t *w, struct ui_event *e, int wait );


static void event_push_thread(void);
static void keyboard_read_thread(void);

static int phantom_window_getc(void);

static void allocate_event();
static void push_event( struct ui_event *e );




int get_n_events_in_q() { return events_in_q; }

#if DELIVER2THREAD
static void w_event_deliver_thread(void);
#endif





#if KEY_EVENTS

#include <thread_private.h>

#endif






//! Put (copy of) any event onto the main e q
void event_q_put_e( struct ui_event *in )
{
    struct ui_event *e = get_unused();
    *e = *in;
    put_event(e);
}





//void drv_video_window_receive_event(struct ui_event *e);





// -----------------------------------------------------------------
// Temporarily moved from vm/video/ here
// -----------------------------------------------------------------

//#include <drv_video_screen.h>





//#define DIRECT_DRIVE 1








#endif
