#ifndef _EV_PRIVATE_H
#define _EV_PRIVATE_H

#include <time.h>
#include <event.h>


#define EVENTS_ENABLED 1
#define KEY_EVENTS 1

// Debug only!
#define DIRECT_DRIVE 0

#define DELIVER2THREAD 1



#if 1
#define MIN_EVENT_POOL  128
#define MAX_EVENT_POOL  512

//#define MAX_EVENT_POOL  5120
#else

// FIXED: Dies instantly - Panic: recursive mutex lock in count_unused/hal_mutex_lock
#define MIN_EVENT_POOL  1
#define MAX_EVENT_POOL  3
#endif

extern int  ev_engine_active;

//! put event on main event q
void        ev_put_event(ui_event_t *e);
ui_event_t *ev_get_unused(void); // TODO ev_put_event must copy event struct itself
void        ev_return_unused(ui_event_t *e);


void        w_receive_event(ui_event_t *e);
void        w_explode_event(ui_event_t *e);

void        w_event_deliver_thread(void);
void        ev_keyboard_read_thread(void);

void        ev_log( int level, ui_event_t *e );


#endif // _EV_PRIVATE_H
