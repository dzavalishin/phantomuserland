#if 0
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * (new) Windowing system internals and housekeeping.
 *
 *
**/


#include <video/screen.h>
#include <video/internal.h>

#include <assert.h>
//#include <kernel/pool.h>
#include <kernel/libkern.h>
#include <kernel/sem.h>
#include <threads.h>
#include <phantom_libc.h>

static void paint_thread(void *arg);
static void start_paint_thread(void);

// max 0.2 sec without paint
//#define MAX_NO_PAINT 1900
#define MAX_NO_PAINT 200

static int paint_tid = -1;

static volatile int paint_request = 0;
static volatile int paint_all = 0;
static hal_sem_t paint_sem;

static volatile int prev_paint_request = 0;
static volatile int no_paint_msec = 0;
static rect_t total;

#define CHECK_START() ({ if(paint_tid < 0) start_paint_thread(); })

// TODO need mutex to access rect

void w_request_async_repaint( rect_t *r )
{
    CHECK_START();

    rect_t old_total = total;
    int old_pq = paint_request;

    rect_add( &total, &old_total, r );

    // Other thread changed something, we have collision.
    // For now just repaint all the screen.
    if(old_pq != paint_request)
        paint_all++;

    paint_request++; // TODO need atomic
    hal_sem_release( &paint_sem );
}

#if SCREEN_UPDATE_THREAD
void scr_repaint_all(void)
{
    CHECK_START();
    paint_all++;
    paint_request++; // TODO need atomic
    hal_sem_release( &paint_sem );
}
#endif

static void do_paint_area()
{
    rect_t todo = total;
    int old_pq = paint_request;

    total.xsize = total.ysize = 0;

    // Other thread changed something, we have collision.
    // For now just repaint all the screen.
    if(old_pq != paint_request)
        paint_all++;

    if( paint_all )
    {
        paint_all = 0;
        todo.x = todo.y = 0;
        todo.xsize = scr_get_xsize();
        todo.ysize = scr_get_ysize();
    }

    repaint_all_for_square(&todo);
}



static void paint_thread(void *arg)
{
    (void) arg;

    hal_set_current_thread_name("paint");
    // +1 so that it is a bit higher than regular sys threads
    hal_set_current_thread_priority(PHANTOM_SYS_THREAD_PRIO+1);

    while(1)
    {
        //hal_sleep_msec(10);
        hal_sem_acquire( &paint_sem );
        if( !paint_request )
            continue;

        if( prev_paint_request < paint_request )
        {
#if 0
            if( no_paint_msec < MAX_NO_PAINT )
            {
                prev_paint_request = paint_request;
                no_paint_msec += 10;
                continue;
            }
#endif
        }

        prev_paint_request = paint_request = 0;
        no_paint_msec = 0;

        do_paint_area();

    }

}

static void start_paint_thread(void)
{
    hal_sem_init( &paint_sem, "paint" );
    paint_tid = hal_start_thread( paint_thread, 0, 0 );
}




#endif
