/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Console window - mostly debug.
 *
**/

#define DEBUG_MSG_PREFIX "console"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


//#include <i386/pio.h>
#include <phantom_libc.h>
#include <hal.h>
#include <time.h>


#include "video.h"
#include "console.h"
#include "misc.h"

#include <threads.h>
#include <kernel/timedcall.h>



//#define CON_FONT drv_video_8x16san_font
#define CON_FONT drv_video_8x16cou_font

#define DEB_FONT drv_video_8x16san_font

#define BUFS 128



drv_video_window_t *phantom_console_window = 0;
drv_video_window_t *phantom_debug_window = 0;


static rgba_t console_fg;
static rgba_t console_bg;


int 	set_fg(struct rgba_t c) { console_fg = c; return 0; }
int 	set_bg(struct rgba_t c) { console_bg = c; return 0; }


static int ttx = 0, tty = 0;
static int phantom_console_window_puts(const char *s)
{
    if(phantom_console_window == 0)
        return 0;

    drv_video_font_tty_string( phantom_console_window, &CON_FONT,
    	s, console_fg, console_bg, &ttx, &tty );

    drv_video_winblt( phantom_console_window );
    return 0;
}



static char cbuf[BUFS+1];
static int cbufpos = 0;


static void flush_stdout(void)
{
    if( cbufpos >= BUFS)
        cbufpos = BUFS;
    cbuf[cbufpos] = '\0';
    phantom_console_window_puts(cbuf);
    cbufpos = 0;
}


static timedcall_t cons_timer =
{
    (void *)flush_stdout,
    0, 100,
    0, 0, { 0, 0 }, 0
};





int phantom_console_window_putc(int c)
{
    phantom_undo_timed_call( &cons_timer );

    switch(c)
    {
    case '\b':
        if(cbufpos > 0) cbufpos--;
        return c;

    case '\t':
        while(cbufpos % 8)
        {
            if(cbufpos >= BUFS)
                break;
            cbuf[cbufpos++] = ' ';
        }
        return c;

    case '\n':
        cbuf[cbufpos++] = c;
        goto flush;

    case '\r':
        cbuf[cbufpos++] = c;
        goto flush;

    default:
        cbuf[cbufpos++] = c;
        if( cbufpos >= BUFS )
            goto flush;

        phantom_request_timed_call( &cons_timer, 0 );
        return c;
    }

flush:
    flush_stdout();
    return c;
}



static int ttxd = 0, ttyd = 0;
int phantom_debug_window_puts(const char *s)
{
    if(phantom_debug_window == 0)        return 0;

    drv_video_font_tty_string( phantom_debug_window, &DEB_FONT,
    	s, console_fg, console_bg, &ttxd, &ttyd );

    //drv_video_winblt( phantom_debug_window );
    return 0;
}


static char dbuf[BUFS+1];
static int dbufpos = 0;

int phantom_debug_window_putc(int c)
{
    dbuf[dbufpos++] = c;

    if( dbufpos >= BUFS || c == '\n' )
    {
        dbuf[dbufpos] = '\0';
        phantom_console_window_puts(dbuf);
        dbufpos = 0;
    }
    return c;
}




static int cw_puts(const char *s)
{
    while(*s)
        phantom_console_window_putc(*s++);
    return 0;
}

static struct console_ops win_ops =
{
    .getchar 		= 0,
    .putchar 		= phantom_console_window_putc,
    .puts       	= cw_puts,
    .set_fg_color       = set_fg,
    .set_bg_color       = set_bg,
};

#define DEBWIN_X 600
#define DEBWIN_Y 10
#define DEBWIN_XS 400
#define DEBWIN_YS 500


static void phantom_debug_window_loop();

void phantom_init_console_window()
{
    console_fg = COLOR_LIGHTGRAY;
    console_bg = COLOR_BLACK;

    int xsize = 620, ysize = 200;
    int cw_x = 50, cw_y = 550;
    if( video_drv->ysize < 600 )
    {
        cw_x = cw_y = 0;
    }

    drv_video_window_t *w = drv_video_window_create(
                        xsize, ysize,
                        cw_x, cw_y, console_bg, "Console" );

    phantom_console_window = w;

    w->owner = GET_CURRENT_THREAD();

    phantom_set_console_ops( &win_ops );
    phantom_console_window_puts("Phantom console window\n");


    phantom_debug_window = drv_video_window_create(
                        DEBWIN_XS, DEBWIN_YS,
                        DEBWIN_X, DEBWIN_Y, console_bg, "Threads" );

    phantom_debug_window_puts("Phantom debug window\n");

    hal_start_kernel_thread(phantom_debug_window_loop);
}


void phantom_stop_console_window()
{
}

//---------------------------------------------------------------------------
// DEBUG window
//---------------------------------------------------------------------------

#define DEBBS 200000

#define PROGRESS_H 4

static void put_progress()
{
    rect_t progress_rect;
    progress_rect.x = 0;
    //progress_rect.y = DEBWIN_YS-PROGRESS_H;
    progress_rect.y = 0;
    progress_rect.ysize = PROGRESS_H;
    progress_rect.xsize = 0;

    extern int vm_map_do_for_percentage;

    progress_rect.xsize = DEBWIN_XS;
    drv_video_window_fill_rect( phantom_debug_window, COLOR_GREEN, progress_rect );

    progress_rect.xsize = (vm_map_do_for_percentage*DEBWIN_XS)/100;
    drv_video_window_fill_rect( phantom_debug_window, COLOR_LIGHTGREEN, progress_rect );
}


static void phantom_debug_window_loop()
{
    static char buf[DEBBS+1];
    int step = 0;

    hal_set_thread_name("Debug Win");

    int wx = 600;

    while(1)
    {
        //hal_sleep_msec(1000);
        hal_sleep_msec(100);
#if 1
        drv_video_window_clear( phantom_debug_window );
        ttyd = 370;
        ttxd = 0;
#endif
        //put_progress();

        void *bp = buf;
        int len = DEBBS;
        int rc;

        time_t sec = uptime();
        int min = sec/60; sec %= 60;
        int hr = min/60; min %= 60;
        int days = hr/24; hr %= 24;

        struct tm *tmp, mt;
        tmp = current_time;
        mt = *tmp;

        rc = snprintf(bp, len, " \x1b[32mStep %d, uptime %d days, %02d:%02d:%02d\x1b[37m\n Today is %02d/%02d/%04d %02d:%02d:%02d\n",
                      step++, days, hr, min, (int)sec,
                      mt.tm_mday, mt.tm_mon, mt.tm_year+1900,
                      mt.tm_hour, mt.tm_min, mt.tm_sec
                     );
        bp += rc;
        len -= rc;

        phantom_dump_threads_buf(bp,len);
        phantom_debug_window_puts(buf);

        if(wx == 600) wx = 620; else wx = 600;
        //drv_video_window_move( phantom_debug_window, wx, 50 );

        put_progress();
        drv_video_winblt( phantom_debug_window );

    }
}


