/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - move window.
 *
 **/

#define DEBUG_MSG_PREFIX "win"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/debug.h>

#include <phantom_libc.h>

#include <video/window.h>
#include <video/internal.h>
#include <video/zbuf.h>
#include <video/screen.h>

#include <sys/libkern.h>

#if 1 // last version


extern hal_mutex_t  rect_list_lock;

// Simple and uses hardware blitter, but result is not ideal - dirt on screen when move.
// Need some more work.

void w_move( drv_video_window_t *w, int x, int y )
{
    ui_event_t e1; //, e2, e3;

    //lprintf( "w move @ %d/%d, sz %d x %d, to %d/%d\n", w->x, w->y, w->xsize, w->ysize, x, y );

    w_lock();

    if( ((w->state & WSTATE_WIN_ROLLEDUP) && !(w->w_title)) || (!(w->state & WSTATE_WIN_VISIBLE)) ) // nothing to do
    {
        w->x = x;
        w->y = y;
        win_move_decorations(w);
        w_unlock();
        return;
    }

    // Do we have hardware accelerated screen to screen bitblt?
    int have_hw_copy = (video_drv->copy != 0) && (video_drv->copy != (void *)vid_null);

    //lprintf( "video accelerator use copy %d/%d -> %d/%d size %d/%d\n", w->x, w->y, x, y, w->xsize, w->ysize );
    rect_t src_r;
    rect_t tmp_r;

    if( w->state & WSTATE_WIN_ROLLEDUP )
    {
        assert(w->w_title);
        w_get_bounds( w->w_title, &src_r );
    }
    else
    {
        w_get_bounds( w, &src_r );

        if( w->w_decor )
        {
            w_get_bounds( w->w_decor, &tmp_r );
            rect_add( &src_r, &src_r, &tmp_r );
        }

        if( w->w_title )
        {
            w_get_bounds( w->w_title, &tmp_r );
            rect_add( &src_r, &src_r, &tmp_r );
        }
    }

    int dest_x = x;
    int dest_y = y;

    if( w->w_decor )
    {
        dest_x = x - ( w->x - w->w_decor->x );
        dest_y = y - ( w->y - w->w_decor->y );
    }

    rect_t big_r;
    scr_get_rect( &big_r );

    rect_t dst_r;
    dst_r = src_r;

    dst_r.x = dest_x;
    dst_r.y = dest_y;

    rect_mul( &src_r, &src_r, &big_r );
    rect_mul( &dst_r, &dst_r, &big_r );

    if( have_hw_copy )
    {
        hal_mutex_lock( &rect_list_lock );  // sync with main painter
        video_drv->copy( src_r.x, src_r.y, dst_r.x, dst_r.y, imin( src_r.xsize, dst_r.xsize ), imin(src_r.ysize, dst_r.ysize ) - 1 ); // why - 1?
        // TODO copy is async, wait for it to complete bef unlock?
        hal_mutex_unlock( &rect_list_lock );
    }

    w->x = x;
    w->y = y;
    win_move_decorations(w);

    scr_zbuf_reset_square( src_r.x, src_r.y, src_r.xsize, src_r.ysize );
    scr_zbuf_reset_square( dst_r.x, dst_r.y, dst_r.xsize, dst_r.ysize );

    // dest is already repaint, repaint (parts of) src

#if 0 // dirt on screen
    if( rect_intersects( &src_r, &dst_r ) )
    {
        rect_t diff1_r, diff2_r;
        rect_sub( &diff1_r, &diff2_r, &src_r, &dst_r );

        memset( &e1, 0, sizeof(e1) );
        e1.type = UI_EVENT_TYPE_GLOBAL;
        e1.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
        e1.w.rect = diff1_r;

        e2 = e1;
        e2.w.rect = diff2_r;

        ev_q_put_global( &e1 );
        ev_q_put_global( &e2 );
    }
    else
    {
        memset( &e1, 0, sizeof(e1) );
        e1.type = UI_EVENT_TYPE_GLOBAL;
        e1.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
        e1.w.rect = src_r;

        ev_q_put_global( &e1 );
    }
#else
    memset( &e1, 0, sizeof(e1) );
    e1.type = UI_EVENT_TYPE_GLOBAL;
    e1.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;

    e1.w.rect = src_r; // dest is already repaint
    ev_q_put_global( &e1 );

    if( !have_hw_copy )
    {
        e1.w.rect = dst_r; // np hw acc?
        ev_q_put_global( &e1 );
    }
#endif

    w_unlock();
}



// faster, but has artefacts at top when move down
#elif 0

void
drv_video_window_move( drv_video_window_t *w, int x, int y )
{
    ui_event_t e1, e2;
    rect_t oldw;
    rect_t neww;

    //w->state &= ~WSTATE_WIN_UNCOVERED; // mark as possibly covered

    e1.type = UI_EVENT_TYPE_GLOBAL;
    e2.type = UI_EVENT_TYPE_GLOBAL;
    e1.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
    e2.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;

    w_lock();

    oldw.x = w->x;
    oldw.y = w->y;
    oldw.xsize = w->xsize;
    oldw.ysize = w->ysize;

    // Suppose that decor overlaps us. If not - use add.
    if( w->w_decor )
    {
        oldw.x = w->w_decor->x;
        oldw.y = w->w_decor->y;
        oldw.xsize = w->w_decor->xsize;
        oldw.ysize = w->w_decor->ysize;
        scr_zbuf_reset_square( w->w_decor->x, w->w_decor->y, w->w_decor->xsize, w->w_decor->ysize );
    }
    else
        scr_zbuf_reset_square( w->x, w->y, w->xsize, w->ysize );

    w->x = x;
    w->y = y;
    win_move_decorations(w);

    neww.x = w->x;
    neww.y = w->y;
    neww.xsize = w->xsize;
    neww.ysize = w->ysize;

    // Suppose that decor overlaps us. If not - use add.
    if( w->w_decor )
    {
        neww.x = w->w_decor->x;
        neww.y = w->w_decor->y;
        neww.xsize = w->w_decor->xsize;
        neww.ysize = w->w_decor->ysize;
    }

    int o2 = rect_sub( &e1.w.rect, &e2.w.rect, &oldw, &neww );

    scr_zbuf_reset_square( e1.w.rect.x, e1.w.rect.y, e1.w.rect.xsize, e1.w.rect.ysize );
    event_q_put_global( &e1 );

    //if( o2 )
    {
        scr_zbuf_reset_square( e2.w.rect.x, e2.w.rect.y, e2.w.rect.xsize, e2.w.rect.ysize );
        event_q_put_global( &e2 );
    }

    event_q_put_win( 0, 0, UI_EVENT_WIN_REDECORATE, w );

    w_unlock();
}

#else

extern hal_mutex_t  rect_list_lock;


void
w_move( drv_video_window_t *w, int x, int y )
{
    ui_event_t e1, e2, e3;

    //lprintf( "w move @ %d/%d, sz %d x %d, to %d/%d\n", w->x, w->y, w->xsize, w->ysize, x, y );

    w_lock();

    if( ((w->state & WSTATE_WIN_ROLLEDUP) && !(w->w_title)) || (!(w->state & WSTATE_WIN_VISIBLE)) ) // nothing to do
    {
        w->x = x;
        w->y = y;
        win_move_decorations(w);
        w_unlock();
        return;
    }




#if 1
    if( (video_drv->copy != 0) && (video_drv->copy != (void *)vid_null) )
    {
        // have hardware accelerated screen to screen bitblt
        //lprintf( "video accelerator use copy %d/%d -> %d/%d size %d/%d\n", w->x, w->y, x, y, w->xsize, w->ysize );
#if 0
        if( w->w_decor )
        {
            int dest_x = x - ( w->x - w->w_decor->x );
            int dest_y = y - ( w->y - w->w_decor->y );

            if( w->w_title )
            {
                int y_size = w->w_title->y + w->w_title->ysize - w->w_decor->y - 1; // w->w_decor->ysize + w->w_title->ysize

                video_drv->copy( w->w_decor->x, w->w_decor->y, dest_x, dest_y, w->w_decor->xsize, y_size );
            }
            else
                video_drv->copy( w->w_decor->x, w->w_decor->y, dest_x, dest_y, w->w_decor->xsize, w->w_decor->ysize );
        }
        else
            video_drv->copy( w->x, w->y, x, y, w->xsize, w->ysize );
        //video_drv->clear( w->x, w->y, w->xsize, w->ysize ); // just to tests it - works on vmware

        //SHOW_FLOW( 0, "video accelerator use copy -> %d/%d", x, y ); // recursive mutex lock
        //lprintf( "done\n" );
#else
        rect_t src_r;
        rect_t tmp_r;

        if( w->state & WSTATE_WIN_ROLLEDUP )
        {
            assert(w->w_title);
            w_get_bounds( w->w_title, &src_r );
        }
        else
        {

            w_get_bounds( w, &src_r );

            if( w->w_decor )
            {
                w_get_bounds( w->w_decor, &tmp_r );
                rect_add( &src_r, &src_r, &tmp_r );
            }

            if( w->w_title )
            {
                w_get_bounds( w->w_title, &tmp_r );
                rect_add( &src_r, &src_r, &tmp_r );
            }
        }

        int dest_x = x;
        int dest_y = y;

        if( w->w_decor )
        {
            dest_x = x - ( w->x - w->w_decor->x );
            dest_y = y - ( w->y - w->w_decor->y );
        }

        rect_t big_r;
        scr_get_rect( &big_r );

        rect_t dst_r;
        dst_r = src_r;

        dst_r.x = dest_x;
        dst_r.y = dest_y;

        rect_mul( &src_r, &src_r, &big_r );
        rect_mul( &dst_r, &dst_r, &big_r );

        hal_mutex_lock( &rect_list_lock );  // sync with main painter
        video_drv->copy( src_r.x, src_r.y, dst_r.x, dst_r.y, imin( src_r.xsize, dst_r.xsize ), imin(src_r.ysize, dst_r.ysize ) - 1 ); // why - 1?
        // TODO copy is async, wait for it to complete bef unlock?
        hal_mutex_unlock( &rect_list_lock );


        // now do what code below did, but better

        w->x = x;
        w->y = y;
        win_move_decorations(w);

        scr_zbuf_reset_square( src_r.x, src_r.y, src_r.xsize, src_r.ysize );
        scr_zbuf_reset_square( dst_r.x, dst_r.y, dst_r.xsize, dst_r.ysize );

        // dest is already repaint, repaint (parts of) src

#if 0 // dirt on screen
        if( rect_intersects( &src_r, &dst_r ) )
        {
            rect_t diff1_r, diff2_r;
            rect_sub( &diff1_r, &diff2_r, &src_r, &dst_r );

            memset( &e1, 0, sizeof(e1) );
            e1.type = UI_EVENT_TYPE_GLOBAL;
            e1.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
            e1.w.rect = diff1_r;

            e2 = e1;
            e2.w.rect = diff2_r;

            ev_q_put_global( &e1 );
            ev_q_put_global( &e2 );
        }
        else
        {
            memset( &e1, 0, sizeof(e1) );
            e1.type = UI_EVENT_TYPE_GLOBAL;
            e1.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
            e1.w.rect = src_r;

            ev_q_put_global( &e1 );
        }
#else
        memset( &e1, 0, sizeof(e1) );
        e1.type = UI_EVENT_TYPE_GLOBAL;
        e1.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;

        e1.w.rect = src_r; // dest is already repaint
        ev_q_put_global( &e1 );

        //e1.w.rect = dst_r; // np hw acc?
        //ev_q_put_global( &e1 );

#endif
        w_unlock();
        return;

#endif

    }
#else
    /*{
     ui_event_t e_copy;
     memset( &e_copy, 0, sizeof(e_copy) );
     e_copy.type = UI_EVENT_TYPE_GLOBAL;
     e_copy.w.info = UI_EVENT_GLOBAL_COPY_RECT;

     e_copy.w.rect.x = w->x;
     e_copy.w.rect.y = w->y;
     e_copy.w.rect.xsize = w->xsize;
     e_copy.w.rect.ysize = w->ysize;

     e_copy.w.rect2.x = x;
     e_copy.w.rect2.y = y;

     ev_q_put_global( &e_copy );
     }*/
#endif

    memset( &e1, 0, sizeof(e1) );
    memset( &e2, 0, sizeof(e2) );
    memset( &e3, 0, sizeof(e3) );

    //w->state &= ~WSTATE_WIN_UNCOVERED; // mark as possibly covered
    e1.type = UI_EVENT_TYPE_GLOBAL;
    e2.type = UI_EVENT_TYPE_GLOBAL;
    e3.type = UI_EVENT_TYPE_GLOBAL;

    e1.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
    e2.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;
    e3.w.info = UI_EVENT_GLOBAL_REPAINT_RECT;

    e1.w.rect.x = w->x;
    e1.w.rect.y = w->y;
    e1.w.rect.xsize = w->xsize;
    e1.w.rect.ysize = w->ysize;

#if !VIDEO_T_IN_D
    if( w->w_title )
    {
        e2.w.rect.x = w->w_title->x;
        e2.w.rect.y = w->w_title->y;
        e2.w.rect.xsize = w->w_title->xsize;
        e2.w.rect.ysize = w->w_title->ysize;
    }
#endif

    if( w->w_decor )
    {
        e3.w.rect.x = w->w_decor->x;
        e3.w.rect.y = w->w_decor->y;
        e3.w.rect.xsize = w->w_decor->xsize;
        e3.w.rect.ysize = w->w_decor->ysize;
    }


    //int ox = w->x;
    //int oy = w->x;

#if !VIDEO_T_IN_D
    if( w->w_title )
        scr_zbuf_reset_square( w->w_title->x, w->w_title->y, w->w_title->xsize, w->w_title->ysize );
#endif
    if( !(w->state & WSTATE_WIN_ROLLEDUP) )
    {
        scr_zbuf_reset_square( w->x, w->y, w->xsize, w->ysize );

        if( w->w_decor )
            scr_zbuf_reset_square( w->w_decor->x, w->w_decor->y, w->w_decor->xsize, w->w_decor->ysize );
    }

    w->x = x;
    w->y = y;
    win_move_decorations(w);


#if 1



#if !VIDEO_T_IN_D
    if( w->w_title )
        ev_q_put_global( &e2 );
#endif

    if( !(w->state & WSTATE_WIN_ROLLEDUP) )
    {
        ev_q_put_global( &e1 );

        if( w->w_decor )
            ev_q_put_global( &e3 );
    }

#else
    scr_repaint_all();
#endif

    //event_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, w );
    ev_q_put_win( 0, 0, UI_EVENT_WIN_REDECORATE, w );

    w_unlock();
}
#endif
