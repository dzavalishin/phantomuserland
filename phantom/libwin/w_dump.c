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


