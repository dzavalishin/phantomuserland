/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - dump window list
 *
**/

#include <video/window.h>
#include <video/internal.h>
#include <kernel/debug.h>
#include <phantom_libc.h>


//#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

void phantom_dump_windows_buf(char *bp, int len)
{
    drv_video_window_t *w;

    {
        int rc;
        rc = snprintf(bp, len, "\x1b[35m Size     Position  Zpoz Name (flags)\x1b[37m\n");
        bp += rc;
        len -= rc;
    }

    w_lock();

    queue_iterate(&allwindows, w, drv_video_window_t *, chain)
    {
        int pn = snprintf( bp, len, " - %s%4dx%-4d @%4dx%-4d z %2d %-20.20s\n   fl 0x%b st 0x%b%s\x1b[37m\n",
                           (w->state & WSTATE_WIN_FOCUSED) ? "\x1b[32m" : "",
                           w->xsize, w->ysize, w->x, w->y, w->z,
                           (w->title ? w->title : ""),
                           w->flags, "\020\01Decor\02!InAll\03NoFocus\04!Free\05!Opaq\06OnTop\07DblBuf\010FullPnt",
                           w->state, "\020\01Focus\02Drag\03VIS\04Rolled\010Uncov\011InFB",
                           (w->stall ? " STALL" : "")
                           //w->events_count, w->owner,
                         );
        len -= pn;
        bp += pn;
    }

    w_unlock();
}

//#pragma GCC diagnostic pop




// -----------------------------------------------------------------------
//
// Dump window list to JSON
//
// -----------------------------------------------------------------------

#include <kernel/json.h>



static void json_encode_window( json_output *jo, drv_video_window_t *w )
{
    if( w == 0 )
    {
        json_out_string( jo, "exist", "no" );
        return;
    }



    json_out_string( jo, "title", (w->title ? w->title : "") );
    json_out_delimiter( jo );

    json_out_int( jo, "focused", !!(w->state & WSTATE_WIN_FOCUSED) );
    json_out_delimiter( jo );



    json_out_int( jo, "x", w->x );
    json_out_delimiter( jo );

    json_out_int( jo, "y", w->y );
    json_out_delimiter( jo );

    json_out_int( jo, "z", w->z );
    json_out_delimiter( jo );



    json_out_int( jo, "xsize", w->xsize );
    json_out_delimiter( jo );

    json_out_int( jo, "ysize", w->ysize );
    json_out_delimiter( jo );



    json_out_int( jo, "state", w->state );
    json_out_delimiter( jo );

    json_out_int( jo, "flags", w->flags );
    json_out_delimiter( jo );

    json_out_int( jo, "stall", w->stall );
    json_out_delimiter( jo );



    json_out_int( jo, "events_count", w->events_count );
    json_out_delimiter( jo );

    json_out_int( jo, "owner", w->owner );
    //json_out_delimiter( jo );

}



void json_dump_windows( json_output *jo )
{
    //json_foreach( jo, "windows", phantom_kernel_threads, sizeof(phantom_thread_t *), MAX_THREADS, json_encode_thread );

    drv_video_window_t *w;
    int i;

    json_out_open_array( jo, "windows" );

    w_lock();

    queue_iterate(&allwindows, w, drv_video_window_t *, chain)
    {
        if( i++ > 0 )
            json_out_delimiter( jo );

        json_out_open_anon_struct( jo );
        json_encode_window( jo, w );
        json_out_close_struct( jo );
    }

    w_unlock();

    json_out_close_array( jo );
}








