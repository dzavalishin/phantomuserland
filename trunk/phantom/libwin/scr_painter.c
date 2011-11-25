/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system grand repaint - modofied painter alg.
 *
 * General idea:
 *
 * - buid list of screen relative rectangles to paint
 * - for each rectangle in list run from topmost window to bottom
 * - if window is opaque, just paint its part and subtract painted rectangle from paint list
 * - if window is transparent, add it to paintback window list, add currently painted rectangle to paintback rect list
 *
**/

#include <video/screen.h>
#include <video/internal.h>

#include <assert.h>
//#include <kernel/pool.h>
#include <kernel/libkern.h>
#include <kernel/sem.h>
#include <threads.h>
#include <queue.h>
#include <phantom_libc.h>


static tid_t            painter_tid;
static hal_sem_t        painter_sem;



typedef struct
{
    rect_t      	r;
    queue_chain_t	chain;
}
pqel_t;


// keep list of screen areas to be repainted
static hal_mutex_t  rect_list_lock;
static queue_head_t rect_list;

void paint_q_init(void)
{
    hal_mutex_init( &rect_list_lock, "rectList" );
    queue_init(&rect_list);
}

bool paint_q_empty(void)
{
    return queue_empty(&rect_list);
}


static pqel_t * mkel( rect_t *r )
{
    pqel_t *new_el = calloc(1,sizeof(pqel_t));
    if( 0 != new_el )
        new_el->r = *r;
    
    return new_el;
}




void paint_q_add( rect_t *r )
{
    hal_mutex_lock( &rect_list_lock );

    pqel_t *new_el = mkel( r );

    // Don't die, just let some garbage onscreen?
    if(0 == new_el)        return;

    hal_sem_release( &painter_sem );

    if(paint_q_empty())
    {
        queue_enter(&rect_list, new_el, pqel_t *, chain);
        goto finish;
    }

    pqel_t *pqel;

again:
    queue_iterate( &rect_list, pqel, pqel_t *, chain )
    {
        // r includes qe - delete qe
        if( rect_includes( r, &pqel->r ) )
        {
            queue_remove( &rect_list, pqel, pqel_t *, chain );
            goto again;
        }


        // qe includes r - skip addition
        if( rect_includes( &pqel->r, r ) )
        {
            free( new_el );
            goto finish;
        }
    }

    // add r itself
    queue_enter(&rect_list, new_el, pqel_t *, chain);

finish:
    hal_mutex_unlock( &rect_list_lock );
}







// Subtract rectangle from q
void paint_q_sub( rect_t *r )
{
    pqel_t *pqel;

    ASSERT_LOCKED_MUTEX( &rect_list_lock );

again:
    queue_iterate( &rect_list, pqel, pqel_t *, chain )
    {

        // r includes qe - delete qe
        if( rect_includes( r, &pqel->r ) )
        {
            queue_remove( &rect_list, pqel, pqel_t *, chain );
            goto again;
        }

        // r intersects w qe - divide in two
        if( rect_intersects( r, &pqel->r ) )
        {
            rect_t out1, out2;

            rect_sub( &out1, &out2, r, &pqel->r );

            if( !rect_empty( &out1 ) )
            {
                pqel_t *new1 = mkel( &out1 );  if( !new1 ) return;
                queue_enter(&rect_list, new1, pqel_t *, chain);
            }

            if( !rect_empty( &out2 ) )
            {
                pqel_t *new2 = mkel( &out2 );  if( !new2 ) return;
                queue_enter(&rect_list, new2, pqel_t *, chain);
            }

            queue_remove( &rect_list, pqel, pqel_t *, chain );
            goto again;
        }
    }

}




#if 1

// Repaint screen square from most bottom window up to <last> window.
// Painter's alorithm.
static void paint_square_downup( drv_video_window_t *last, rect_t *todo )
{

    drv_video_window_t *w;

    w_lock();
    scr_mouse_disable_p(video_drv, todo->x, todo->y, todo->xsize, todo->ysize );

    queue_iterate(&allwindows, w, drv_video_window_t *, chain)
    {
        w_repaint_screen_part( w, todo );

        if( w == last )
            break;
    }

}

// Repaint screen square from topmost window down.
// Minimizes painting effort for opaque windows.
static void paint_square_updown(rect_t *r)
{
    drv_video_window_t *w;

    queue_iterate_back(&allwindows, w, drv_video_window_t *, chain)
    {
        rect_t wr;
        w_get_bounds( w, &wr );

        if( !rect_intersects( r, &wr ) )
            continue;

        // if win is transparent
        if(WIN_HAS_FLAG(w,WFLAG_WIN_NOTOPAQUE))
        {
            paint_square_downup( w, r );
            break;
        }

        w_repaint_screen_part( w, r );
    }
}



static void repaint_q(void)
{
    pqel_t *pqel;

    ASSERT_LOCKED_MUTEX( &rect_list_lock );

    while(1)
    {
        hal_mutex_lock( &rect_list_lock );

        if(paint_q_empty())
            break;

        queue_remove_first( &rect_list, pqel, pqel_t *, chain );

        hal_mutex_unlock( &rect_list_lock );

        rect_t *r = &pqel->r;
        free(pqel);

        scr_zbuf_reset_square( r->x, r->y, r->xsize, r->ysize );
        paint_square_updown( r );
    }

    hal_mutex_unlock( &rect_list_lock );
}



static void painter_thread(void *arg)
{
    (void) arg;

    hal_set_current_thread_name("painter");
    // +1 so that it is a bit higher than regular sys threads
    hal_set_current_thread_priority(PHANTOM_SYS_THREAD_PRIO+1);

    while(1)
    {
        hal_sem_acquire( &painter_sem );
        hal_sleep_msec(2); // give 'em some chance to put more to queue
        repaint_q();
    }

}

void start_painter_thread(void)
{
    hal_sem_init( &painter_sem, "painter" );
    painter_tid = hal_start_thread( painter_thread, 0, 0 );
}







#endif


