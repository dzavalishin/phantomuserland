/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Main win/ui event management code.
 *
 *
**/

#define EVENTS_ENABLED 1

#include <threads.h>
#include <event.h>
#include <queue.h>
#include <malloc.h>
#include <hal.h>
#include <string.h>
#include <video.h>

void drv_video_window_explode_event(struct ui_event *e);


static void event_push_thread();

static void allocate_event();
static void push_event( struct ui_event *e );

//static int phantom_window_getc(void);


static int                      event_engine_active = 0;

static hal_mutex_t             main_q_mutex;
static hal_mutex_t             unused_q_mutex;
static hal_cond_t              have_event;


// Each main Q event is on one of these qs
static queue_head_t     unused_events;  // list of unused event structs
static queue_head_t     main_event_q;  	// list of generated events

#define MIN_EVENT_POOL  128
#define MAX_EVENT_POOL  512

/**
 *
 * Init UI event queue.
 *
**/

void init_main_event_q()
{
    queue_init(&unused_events);
    queue_init(&main_event_q);

    hal_mutex_init( &main_q_mutex, "MainEvQ" );
    hal_mutex_init( &unused_q_mutex, "FreeEvQ" );

    hal_cond_init( &have_event, "UIEvent" );

    hal_mutex_lock( &unused_q_mutex );
    int i = MIN_EVENT_POOL;
    while(i--)
        allocate_event();
    hal_mutex_unlock( &unused_q_mutex );

#if EVENTS_ENABLED
    hal_start_kernel_thread( event_push_thread );

    event_engine_active = 1;

#if 0
    phantom_set_console_getchar( phantom_window_getc );
    phantom_dev_keyboard_start_events();
#endif
#endif
}


#if 0
static int phantom_window_getc(void)
{
    wtty_t *tty = &(GET_CURRENT_THREAD()->ctty);
    return 0;
}
#endif




//! Allocate one more event
static void allocate_event()
{
    struct ui_event *e = calloc( 1, sizeof(struct ui_event) );
    queue_enter(&unused_events, e, struct ui_event *, echain);
}


//! Get unused event from unused events queue, or allocate new one.
static struct ui_event * get_unused()
{
    struct ui_event *e;

    hal_mutex_lock( &unused_q_mutex );
    if(queue_empty(&unused_events))
    {
        allocate_event();
    }

    if(queue_empty(&unused_events))
        panic("out of events");

    queue_remove_first(&unused_events, e, struct ui_event *, echain);
    hal_mutex_unlock( &unused_q_mutex );
    return e;
}

//! Return (and clear!) unused event to unused q
void return_unused_event(struct ui_event *e)
{
    assert(event_engine_active);
    memset( e, sizeof(struct ui_event), 0 );
    hal_mutex_lock( &unused_q_mutex );
    queue_enter(&unused_events, e, struct ui_event *, echain);
    hal_mutex_unlock( &unused_q_mutex );
}


//! Put filled event onto the main event q
static void put_event(struct ui_event *e)
{
    if(!event_engine_active) return; // Just ignore

    hal_mutex_lock( &main_q_mutex );
    queue_enter(&main_event_q, e, struct ui_event *, echain);
    hal_cond_broadcast( &have_event );
    hal_mutex_unlock( &main_q_mutex );
}


//! Count events on unused Q
static int count_unused()
{
    int count = 0;

    hal_mutex_lock( &unused_q_mutex );
    struct ui_event *e;
    queue_iterate(&unused_events, e, struct ui_event *, echain)
    {
        count++;
    }
    hal_mutex_unlock( &unused_q_mutex );

    return count;
}

//! Remove unused events if there are too many
static void remove_extra_unused()
{
    int c = count_unused();
    if(c > MAX_EVENT_POOL)
    {
        struct ui_event *e = get_unused();
        free(e);
    }
}


//! Put (copy of) any event onto the main e q
void event_q_put_e( struct ui_event *in )
{
    struct ui_event *e = get_unused();
    *e = *in;
    put_event(e);
}




//! This thread delivers events from main Q to windows
static void event_push_thread()
{
    hal_set_thread_name("UIEventQ");

    while(1)
    {
#if EVENTS_ENABLED
        remove_extra_unused();

        struct ui_event *e;

        hal_mutex_lock( &main_q_mutex );
        while(queue_empty(&main_event_q))
            hal_cond_wait( &have_event, &main_q_mutex );

        if(queue_empty(&main_event_q))
            panic("out of events");

        queue_remove_first(&main_event_q, e, struct ui_event *, echain);
        hal_mutex_unlock( &main_q_mutex );

        // Deliver to 'em
        push_event(e);

        // window code will return when done
        //return_unused_event(e);
#endif
    }


}

void drv_video_window_receive_event(struct ui_event *e);


static void push_event( struct ui_event *e )
{
    if( e->type == UI_EVENT_TYPE_GLOBAL )
        drv_video_window_explode_event(e);
    else
        drv_video_window_receive_event(e);
}




//! Put key event onto the main e q
void event_q_put_key( int vkey, int ch, int modifiers )
{
    if(!event_engine_active) return; // Just ignore

    struct ui_event *e = get_unused();
    e->type = UI_EVENT_TYPE_KEY;
    e->time = 0; // TODO put time
    e->focus= 0;

    e->k.vk = vkey;
    e->k.ch = ch;

    e->modifiers = modifiers;

    put_event(e);
}


//! Put mouse event onto the main e q
void event_q_put_mouse( int x, int y, int buttons )
{
    if(!event_engine_active) return; // Just ignore

    struct ui_event *e = get_unused();
    e->type = UI_EVENT_TYPE_MOUSE;
    e->time = 0; // TODO put time
    e->focus= 0;

    e->m.buttons = buttons;
    e->abs_x = x;
    e->abs_y = y;

    put_event(e);
}


//! Put window event onto the main e q
void event_q_put_win( int x, int y, int info, struct drv_video_window *   focus )
{
    if(!event_engine_active) return; // Just ignore

    struct ui_event *e = get_unused();
    e->type = UI_EVENT_TYPE_WIN;
    e->time = 0; // TODO put time
    e->focus= focus;

    e->w.info = info;
    e->abs_x = x;
    e->abs_y = y;

    put_event(e);
}



//! Put global event onto the main e q
void event_q_put_global( ui_event_t *ie )
{
    if(!event_engine_active) return; // Just ignore

    struct ui_event *e = get_unused();

    *e = *ie;

    e->type = UI_EVENT_TYPE_GLOBAL;
    e->time = 0; // TODO put time
    e->focus= 0;

    put_event(e);
}




// -----------------------------------------------------------------
// Temporarily moved from vm/video/ here
// -----------------------------------------------------------------

#include <drv_video_screen.h>

extern queue_head_t     	allwindows;
extern drv_video_window_t *	focused_window;
extern hal_spinlock_t  		allw_lock;


//! Select target window 
static void select_event_target(struct ui_event *e)
{
    // Don't even try to select destination for this type of event
    if( e->type == UI_EVENT_TYPE_WIN )
        return;

    drv_video_window_t *w;

    e->focus = focused_window;

    // Keys are delivered to focused window only
    if( e->type == UI_EVENT_TYPE_KEY )
        return;

    // Mouse can be tied to focused window
    //if( focused_window != 0 && focused_window->mouse_tie )
    //    return;

    assert( e->type == UI_EVENT_TYPE_MOUSE );

    int wz = 0;
    //hal_spin_lock( &allw_lock );
    queue_iterate(&allwindows, w, drv_video_window_t *, chain)
    {
        if( w->z < wz )
            continue;

        wz = w->z;

        if( point_in_win( e->abs_x, e->abs_y, w ) )
        {
            e->focus = w;
            //break; // need to check all to make sure we selected topmost
        }
        
    }
    //hal_spin_unlock( &allw_lock );

}




//! Select target and put event to window queue.
void drv_video_window_receive_event(struct ui_event *e)
{
    assert(e);
    drv_video_window_t *w = 0;

    int ie = hal_save_cli();
    hal_spin_lock( &allw_lock );

    select_event_target(e);

    drv_video_window_t *later_lost = 0;
    drv_video_window_t *later_gain = 0;

    if( e->focus == 0 )
    {
        //printf("unfocused event");
        goto ret;
    }

	
    int later_x, later_y;

    // For now use any mouse event to change focus
    if(e->type == UI_EVENT_TYPE_MOUSE && focused_window != e->focus)
    {
        later_x = e->abs_x;
        later_y = e->abs_y;
        if(focused_window != 0)
            later_lost = focused_window;
        later_gain = e->focus;
        focused_window = e->focus;
    }

    // Target
    w = e->focus;

    // Calc relative x, y
    e->rel_x = e->abs_x - w->x;
    e->rel_y = e->abs_y - w->y;

    e->abs_z = w->z;
    e->rel_z = 0;

    if( w->events_count < MAX_WINDOW_EVENTS )
    {
        queue_enter(&(w->events), e, struct ui_event *, echain);
        w->events_count++;
        w->stall = 0;
    }
    else
        w->stall = 1;

ret:
    hal_spin_unlock( &allw_lock );
    if(ie) hal_sti();

    // It has mutex and can't be called in spinlock
    if(later_lost) event_q_put_win( later_x, later_y, UI_EVENT_WIN_LOST_FOCUS, later_lost );
    if(later_gain) event_q_put_win( later_x, later_y, UI_EVENT_WIN_GOT_FOCUS, later_gain );


    if(w != 0 && w->inKernelEventProcess)
        w->inKernelEventProcess(w);

}


void drv_video_window_explode_event(struct ui_event *e)
{
    drv_video_window_t *w;

    int ie = hal_save_cli();
    hal_spin_lock( &allw_lock );
    queue_iterate(&allwindows, w, drv_video_window_t *, chain)
    {
        rect_t  wr, isect;
        drv_video_window_get_bounds( w, &wr );


        if( e->w.info == UI_EVENT_GLOBAL_REPAINT_RECT )
        {
            int intersects = rect_mul( &isect, &wr, &(e->w.rect) );
            if(intersects)
                event_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, w );
        }

    }
    hal_spin_unlock( &allw_lock );
    if(ie) hal_sti();


}



//! Get next event for this window
int drv_video_window_get_event( drv_video_window_t *w, struct ui_event *e, int wait )
{
    int ie = 1;
    int ret;
    struct ui_event *tmp;

    // Possibly will have problem when killing window
    while(wait)
    {
        while( w->events_count <= 0 )
            hal_sleep_msec( 100 );

        ie = hal_save_cli();
        hal_spin_lock( &allw_lock );
        if( w->events_count > 0 )
            goto locked;

        hal_spin_unlock( &allw_lock );
        if(ie) hal_sti();
    }

    ie = hal_save_cli();
    hal_spin_lock( &allw_lock );
locked:
    if( w->events_count > 0 )
    {
        queue_remove_first(&(w->events), tmp, struct ui_event *, echain);
        w->events_count--;
        ret = 1;
    }
    else
        ret = 0;

    hal_spin_unlock( &allw_lock );
    if(ie) hal_sti();

    if( ret )
    {
        *e = *tmp;
        // Bring it back to main Q engine
        return_unused_event(tmp);
    }

    return ret;
}



