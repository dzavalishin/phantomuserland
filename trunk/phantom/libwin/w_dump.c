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

    w_lock();
    queue_iterate(&allwindows, w, drv_video_window_t *, chain)
    {
        int pn = snprintf( bp, len, "%s%3dx%3d @%3dx%3d z %2d fl%b st%b %s%.10s\x1b[37m\n",
                           (w->state & WSTATE_WIN_FOCUSED) ? "\x1b[32m" : "",
                           w->xsize, w->ysize, w->x, w->y, w->z,
                           w->flags, "\020\01Decor\02!InAll\03NoFocus\04!Pixels\05!Opaq\06OnTop",
                           w->state, "\020\01Focused\02Dragged\03Visible\04Rolled\010Uncov\011InFB",
                           (w->stall ? "STALL " : ""),
                           //w->events_count, w->owner,
                           (w->title ? w->title : "??")
                         );
        len -= pn;
        bp += pn;
    }
    w_unlock();
}

//#pragma GCC diagnostic pop


