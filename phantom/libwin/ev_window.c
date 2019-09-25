/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - window event queue.
 *
**/



#define DEBUG_MSG_PREFIX "events"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10


#include "ev_private.h"

#include <video/window.h>
#include <video/internal.h>
#include <video/button.h>

#include <threads.h>

#include <kernel/snap_sync.h>



static void w_do_deliver_event(window_handle_t w)
{
    //if(w != 0 && w->eventDeliverSema)        hal_sem_release(w->eventDeliverSema);

    if(w != 0 && w->inKernelEventProcess)
    {
        struct ui_event e;
        int got = ev_w_get_event( w, &e, 0 );

        while(got)
        {
            struct ui_event e2;
            if( ev_w_get_event( w, &e2, 0 ) )
            {
                // 2 repaints follow
                if((e.type == e2.type) && (e.w.info == e2.w.info) && (e.focus == e2.focus))
                {
                    if((e.w.info == UI_EVENT_WIN_REPAINT) || (e.w.info == UI_EVENT_WIN_REDECORATE))
                    {
                        SHOW_FLOW0( 1, "combined repaint" );
                        // Choose more powerful spell
                        //e.w.info = UI_EVENT_WIN_REDECORATE;
                        // Eat one
                        //e = e2;
                        continue;
                    }
                }
            }
            else
                got = 0;

            SHOW_FLOW(8, "%p, w=%p, us=%p", &e, e.focus, w);

            w->inKernelEventProcess(w, &e);
            e = e2;
        }
    }
}



#if DELIVER2THREAD

static hal_sem_t        we_sem;
static int              we_inited = 0;

void w_event_deliver_thread(void)
{
    hal_sem_init( &we_sem, "wevent" );
    we_inited = 1;

    t_current_set_name("WEvent");
    t_current_set_priority(PHANTOM_SYS_THREAD_PRIO+1);

    //vm_lock_persistent_memory(); // We access persistent memory now and then

    while(1)
    {
        //if(phantom_virtual_machine_snap_request) // Check if we need release access to persistent memory
        //    phantom_thread_wait_4_snap();

        hal_sem_acquire( &we_sem ); // TODO need some 'acquire_all' method to eat all releases

        vm_lock_persistent_memory();

    restart:
        w_lock();

        window_handle_t w;

        queue_iterate_back(&allwindows, w, drv_video_window_t *, chain)
        {
            if( w->events_count )
            {
                if(w->eventDeliverSema)
                    hal_sem_release(w->eventDeliverSema);

                if(w->inKernelEventProcess)
                {
                    w_unlock();
                    w_do_deliver_event(w);
                    goto restart;
                }
            }
        }

        w_unlock();
        vm_unlock_persistent_memory();

    }
}

#endif





//! Select target window
static void select_event_target(struct ui_event *e)
{

    // Don't even try to select destination for this type of event
    if( e->type == UI_EVENT_TYPE_WIN )
        return;

    window_handle_t w;

    e->focus = focused_window;

    // If focused window is being dragged - don't try to change focus - TODO global push_focus_lock/push_focus_lock?
    if( (focused_window != 0) && (focused_window->state & WSTATE_WIN_DRAGGED) )
        return;

    // Keys are delivered to focused window only
    if( e->type == UI_EVENT_TYPE_KEY )
        return;

    // Mouse can be tied to focused window
    //if( focused_window != 0 && focused_window->mouse_tie )
    //    return;

    assert( e->type == UI_EVENT_TYPE_MOUSE );

    w_assert_lock();

    int wz = 0;
    queue_iterate(&allwindows, w, drv_video_window_t *, chain)
    {
        if( w->flags & WFLAG_WIN_NOFOCUS )
            continue;
        if( w->z < wz )
            continue;

        wz = w->z;

        if( point_in_win( e->abs_x, e->abs_y, w ) )
        {
            e->focus = w;
            //break; // need to check all to make sure we selected topmost
        }

    }

}





//! Select target and put event to window queue.
void w_receive_event(ui_event_t *e)
{
    assert(e);
    window_handle_t w = 0;

    w_lock();

    select_event_target(e);

    window_handle_t later_lost = 0;
    window_handle_t later_gain = 0;

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

    // Not a best place - it can produce events too
    if( e->type == UI_EVENT_TYPE_MOUSE )
    {
        ui_event_t ecopy = *e; // for any case
        w_check_button( w, &ecopy );
    }

    if( w->events_count < MAX_WINDOW_EVENTS )
    {
    	SHOW_FLOW(8, "e %p -> w %p", e, w);

#if DIRECT_DRIVE
        if(w != 0 && w->inKernelEventProcess)
            w->inKernelEventProcess(w, e);
#else
    	queue_enter(&(w->events), e, struct ui_event *, echain);
        w->events_count++;
        w->stall = 0;
#endif
    }
    else
        w->stall = 1;

ret:
    w_unlock();

    // It has mutex and can't be called in spinlock
    if(later_lost) ev_q_put_win( later_x, later_y, UI_EVENT_WIN_LOST_FOCUS, later_lost );
    if(later_gain) ev_q_put_win( later_x, later_y, UI_EVENT_WIN_GOT_FOCUS, later_gain );

#if (!DIRECT_DRIVE) && (!DELIVER2THREAD)
    // Has no own event process thread, serve from here
    w_do_deliver_event(w);
#endif

#if DELIVER2THREAD
    if(we_inited)
        hal_sem_release( &we_sem );
#endif

}







void w_explode_event(ui_event_t *e)
{
    window_handle_t w;

#if 1
    if( e->w.info == UI_EVENT_GLOBAL_REPAINT_RECT )
    {
        w_request_async_repaint( &(e->w.rect) );
        return;
    }

#endif

    w_lock();
    queue_iterate(&allwindows, w, drv_video_window_t *, chain)
    {
        rect_t  wr, isect;
        w_get_bounds( w, &wr );


        if( e->w.info == UI_EVENT_GLOBAL_REPAINT_RECT )
        {
            int intersects = rect_mul( &isect, &wr, &(e->w.rect) );
            if(intersects)
                ev_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, w );
        }

    }
    w_unlock();

}











//! Get next event for this window
int ev_w_get_event( window_handle_t w, struct ui_event *e, int wait )
{
    //int ie = 1;
    int ret;
    struct ui_event *tmp;

    // Possibly will have problem when killing window
    while(wait)
    {
        while( w->events_count <= 0 )
            hal_sleep_msec( 100 );

        w_lock();
        if( w->events_count > 0 )
            goto locked;

        w_unlock();
    }

    w_lock();
locked:
    if( w->events_count > 0 )
    {
    	assert(!queue_empty(&(w->events)));
        queue_remove_first(&(w->events), tmp, struct ui_event *, echain);
        w->events_count--;
        ret = 1;
    }
    else
        ret = 0;

    w_unlock();

    if( ret )
    {
    	SHOW_FLOW(8, "tmp %p, w=%p", tmp, tmp->focus);

        *e = *tmp;

        SHOW_FLOW(8, "e %p, w=%p", e, e->focus);

        // Bring it back to main Q engine
        ev_return_unused(tmp);
    }

    return ret;
}






