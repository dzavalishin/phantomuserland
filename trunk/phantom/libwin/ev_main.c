/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Main win/ui event queue.
 *
 *
**/


#define DEBUG_MSG_PREFIX "events"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>
#include <threads.h>
#include <thread_private.h>
#include <wtty.h>

#include "ev_private.h"

int                            ev_engine_active = 0;

static hal_mutex_t             ev_main_q_mutex;
static hal_mutex_t             ev_unused_q_mutex;
static hal_cond_t              ev_have_event;


// Each main Q event is on one of these qs
static queue_head_t            ev_unused_events;  // list of unused event structs
static queue_head_t            ev_main_event_q;  	// list of generated events
static int                     ev_events_in_q = 0;



//! Allocate one more event
static void ev_allocate_event()
{
    struct ui_event *e = calloc( 1, sizeof(struct ui_event) );
    assert(e);
    queue_enter(&ev_unused_events, e, struct ui_event *, echain);
}


//! Get unused event from unused events queue, or allocate new one.
ui_event_t * ev_get_unused()
{
    struct ui_event *e;

    hal_mutex_lock( &ev_unused_q_mutex );
    if(queue_empty( &ev_unused_events ))
    {
        ev_allocate_event();
    }

    if(queue_empty(&ev_unused_events))
        panic("out of events");

    queue_remove_first(&ev_unused_events, e, struct ui_event *, echain);
    hal_mutex_unlock( &ev_unused_q_mutex );
    return e;
}

//! Return (and clear!) unused event to unused q
void ev_return_unused(ui_event_t *e)
{
    assert(ev_engine_active);
    memset( e, sizeof(struct ui_event), 0 );
    hal_mutex_lock( &ev_unused_q_mutex );
    queue_enter(&ev_unused_events, e, struct ui_event *, echain);
    hal_mutex_unlock( &ev_unused_q_mutex );
}


//! Count events on unused Q
static int ev_count_unused()
{
    int count = 0;

    hal_mutex_lock( &ev_unused_q_mutex );

    if(queue_empty(&ev_unused_events))
    {
        hal_mutex_unlock( &ev_unused_q_mutex );
        return 0;
    }

    struct ui_event *e;
    queue_iterate(&ev_unused_events, e, struct ui_event *, echain)
    {
        count++;
    }
    hal_mutex_unlock( &ev_unused_q_mutex );

    return count;
}

//! Remove unused events if there are too many
static void ev_remove_extra_unused()
{
    int c = ev_count_unused();
    if(c > MAX_EVENT_POOL)
    {
        struct ui_event *e = ev_get_unused();
        free(e);
    }
}





//! Put filled event onto the main event q
void ev_put_event(ui_event_t *e)
{
    if(!ev_engine_active) return; // Just ignore

    SHOW_FLOW(8, "%p", e);
    hal_mutex_lock( &ev_main_q_mutex );
    ev_events_in_q++;
    queue_enter(&ev_main_event_q, e, struct ui_event *, echain);
    hal_cond_broadcast( &ev_have_event );
    hal_mutex_unlock( &ev_main_q_mutex );
}





static void ev_push_event( struct ui_event *e )
{
    //SHOW_FLOW( 9, "type %d abs x %d t %d", e->type, e->abs_x, e->abs_y );
    //printf("%d,%d\n", e->abs_x, e->abs_y );
    ev_log( 9, e );

    if( e->type == UI_EVENT_TYPE_GLOBAL )
        w_explode_event(e);
    else
        w_receive_event(e);
}


//! This thread delivers events from main Q to windows
static void ev_push_thread()
{
    hal_set_thread_name("UIEventQ");
    // +1 so that it is a bit higher than regular sys threads
    hal_set_current_thread_priority(PHANTOM_SYS_THREAD_PRIO+1);

#if EVENTS_ENABLED && 1
    while(1)
    {
        ev_remove_extra_unused();

        struct ui_event *e;

        hal_mutex_lock( &ev_main_q_mutex );
        while(queue_empty(&ev_main_event_q))
            hal_cond_wait( &ev_have_event, &ev_main_q_mutex );

        if(queue_empty(&ev_main_event_q))
            panic("out of events");

        queue_remove_first(&ev_main_event_q, e, struct ui_event *, echain);
        ev_events_in_q--;
        hal_mutex_unlock( &ev_main_q_mutex );

        SHOW_FLOW(8, "%p", e);

        // Deliver to 'em
        ev_push_event(e);

        // window code will return when done
        //return_unused_event(e);
    }
#else
    while(1)
    {
    	hal_sleep_msec(20000);
    }
#endif


}





static int phantom_window_getc(void)
{
    //SHOW_FLOW0( 11, "window getc" );
    //wtty_t *tty = &(GET_CURRENT_THREAD()->ctty);
    wtty_t *tty = GET_CURRENT_THREAD()->ctty;

    if(tty == 0)
    {
        SHOW_ERROR0( 0, "No wtty, phantom_window_getc loops forever" );
        while(1)
            hal_sleep_msec(10000);
    }

    return wtty_getc( tty );
}


int ev_get_n_events_in_q() { return ev_events_in_q; }



/**
 *
 * Init UI event queue.
 *
**/

void init_main_event_q()
{
    queue_init(&ev_unused_events);
    queue_init(&ev_main_event_q);

    hal_mutex_init( &ev_main_q_mutex, "Main EvQ" );
    hal_mutex_init( &ev_unused_q_mutex, "Free EvQ" );

    hal_cond_init( &ev_have_event, "UIEvent" );

    hal_mutex_lock( &ev_unused_q_mutex );
    int i = MIN_EVENT_POOL;
    while(i--)
        ev_allocate_event();
    hal_mutex_unlock( &ev_unused_q_mutex );

#if EVENTS_ENABLED
    hal_start_kernel_thread( ev_push_thread );


    ev_engine_active = 1;

#if KEY_EVENTS
    phantom_set_console_getchar( phantom_window_getc );
    hal_start_kernel_thread( ev_keyboard_read_thread );
#endif
#endif

#if DELIVER2THREAD
    hal_start_kernel_thread( w_event_deliver_thread );
#endif

}
