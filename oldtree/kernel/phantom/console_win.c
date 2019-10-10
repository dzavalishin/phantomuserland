/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Console window - mostly debug.
 *
**/

// crashes :(
#define TIMED_FLUSH 0
// looses characters :(
#define NET_TIMED_FLUSH 1


#define DEBUG_MSG_PREFIX "console"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <phantom_libc.h>
#include <hal.h>
#include <wtty.h>
#include <time.h>
#include <event.h> // get_n_events_in_q()


#include <video/window.h>
#include <video/font.h>
#include <video/screen.h>
#include <video/button.h>

#include "console.h"
#include "misc.h"

#include <threads.h>
#include <kernel/timedcall.h>
#include <kernel/debug.h>
#include <kernel/stats.h>
#include <kernel/profile.h>
#include <kernel/init.h>

#include <kernel/json.h>

#if NET_TIMED_FLUSH
#include <kernel/net_timer.h>
#endif


#define CON_FONT drv_video_8x16san_font
#define DEB_FONT drv_video_8x16cou_font

#define BUFS 128

static font_handle_t ttfont;

#if NEW_WINDOWS
window_handle_t phantom_console_window = 0;
window_handle_t phantom_debug_window = 0;
#else
drv_video_window_t *phantom_console_window = 0;
drv_video_window_t *phantom_debug_window = 0;
#endif

static window_handle_t phantom_launcher_window = 0;
static int phantom_launcher_event_process( window_handle_t w, ui_event_t *e);


static rgba_t console_fg;
static rgba_t console_bg;


int 	set_fg(struct rgba_t c) { console_fg = c; return 0; }
int 	set_bg(struct rgba_t c) { console_bg = c; return 0; }


static int ttx = 0, tty = 0;
static int phantom_console_window_puts(const char *s)
{
    if(phantom_console_window == 0)
        return 0;

    w_font_tty_string( phantom_console_window, &CON_FONT,
    	s, console_fg, console_bg, &ttx, &tty );

    w_update( phantom_console_window );
    return 0;
}



static char cbuf[BUFS+1];
static int cbufpos = 0;
static hal_mutex_t buf_mutex;


static void flush_stdout(void * arg)
{
    char text[BUFS + 1];
    (void) arg;

    hal_mutex_lock( &buf_mutex );
    if( cbufpos >= BUFS)
        cbufpos = BUFS;
    cbuf[cbufpos] = '\0';
    memcpy(text, cbuf, cbufpos + 1);
    cbufpos = 0;
    hal_mutex_unlock( &buf_mutex );
    phantom_console_window_puts(text);
}

static void put_buf( char c )
{
    hal_mutex_lock( &buf_mutex );
    cbuf[cbufpos++] = c;
    hal_mutex_unlock( &buf_mutex );
}



#if TIMED_FLUSH
static timedcall_t cons_timer =
{
    (void *)flush_stdout,
    0, 100,
    0, 0, { 0, 0 }, 0
};
#endif

#if NET_TIMED_FLUSH
static net_timer_event cons_upd_timer;
#endif


int phantom_console_window_putc(int c)
{
#if TIMED_FLUSH
    phantom_undo_timed_call( &cons_timer );
#endif

#if NET_TIMED_FLUSH
    cancel_net_timer(&cons_upd_timer);
#endif

    switch(c)
    {
    case '\b':
        if(cbufpos > 0) cbufpos--;
        goto noflush;
        //return c;

    case '\t':
        while(cbufpos % 8)
        {
            if(cbufpos >= BUFS)
                break;
            put_buf(' ');
        }
        goto noflush;
        //return c;

    case '\n':
    case '\r':
        put_buf( c );
        goto flush;

    default:
        put_buf( c );
        if( cbufpos >= BUFS )
            goto flush;

noflush:
#if TIMED_FLUSH
        phantom_request_timed_call( &cons_timer, 0 );
#endif

#if NET_TIMED_FLUSH
        set_net_timer(&cons_upd_timer, 100, flush_stdout, 0, 0 );
#endif

        return c;
    }

flush:
    flush_stdout(0);
    return c;
}



static int ttxd = 0, ttyd = 0;
int phantom_debug_window_puts(const char *s)
{
    if(phantom_debug_window == 0)        return 0;

    w_font_tty_string( phantom_debug_window, &DEB_FONT,
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
        //phantom_console_window_puts(dbuf);
        phantom_debug_window_puts(dbuf);
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

#define DEBWIN_X 580
#define DEBWIN_Y 32
#define DEBWIN_XS 416
#define DEBWIN_YS 600

//#define MAX_LAUNCH_BUTTONS 64
#define MAX_LAUNCH_BUTTONS 5

static pool_handle_t taskbuttons[MAX_LAUNCH_BUTTONS];

static void phantom_debug_window_loop();
//static void phantom_launcher_window_loop();

extern drv_video_bitmap_t vanilla_task_button_bmp;
extern drv_video_bitmap_t slide_switch_on_bmp;
extern drv_video_bitmap_t slide_switch_off_bmp;

void phantom_init_console_window()
{
    hal_mutex_init( &buf_mutex, "console" );

    console_fg = COLOR_LIGHTGRAY;
    console_bg = COLOR_BLACK;

    int xsize = 620, ysize = 300;
    int cw_x = 50, cw_y = 450;
    if( scr_get_ysize() < 600 )
    {
        cw_x = cw_y = 0;
    }


    ttfont = w_get_system_font_ext( 16 );

    drv_video_window_t *w = drv_video_window_create( xsize, ysize,
                        cw_x, cw_y, console_bg, "Console", WFLAG_WIN_DECORATED|WFLAG_WIN_DOUBLEBUF );

    phantom_console_window = w;

    w->owner = get_current_tid();

    phantom_set_console_ops( &win_ops );
    phantom_console_window_puts("Phantom console window\n");


    phantom_debug_window = drv_video_window_create(
                                                   DEBWIN_XS, DEBWIN_YS,
                                                   DEBWIN_X, DEBWIN_Y, console_bg, "Threads", WFLAG_WIN_DECORATED|WFLAG_WIN_DOUBLEBUF|WFLAG_WIN_FULLPAINT );

    //phantom_debug_window->flags |= WFLAG_WIN_DOUBLEBUF|WFLAG_WIN_FULLPAINT;
    //w_update( phantom_debug_window ); // For dbl buf flags to start working ok

    phantom_debug_window_puts("Phantom debug window\n\nt - threads\nw - windows\ns - stats\np - profiler\n");
    w_update( phantom_debug_window );
    //hal_sleep_msec(4000);

    hal_start_kernel_thread(phantom_debug_window_loop);


    // -------------------------------------------------------------------
    // Launcher window
    // -------------------------------------------------------------------

    //    color_t la_bg = { 0x19, 0x19, 0x19, 0xFF };
    //color_t la_b1 = { 68, 66, 62, 0xFF  };
    //color_t la_b2 = { 88, 84, 79, 0xFF  };

    color_t la_bg = { 214, 227, 231, 0xFF };

    color_t la_b1 = { 214, 227, 231, 0xFF };
    color_t la_b2 = { .r = 183, .g = 171, .b = 146, .a = 0xFF  };

    //color_t la_txt = { 0x11, 0xd5, 0xff, 0xFF };
    color_t la_txt = { 0x0, 0x0, 0x0, 0xFF };
//#define BTEXT_COLOR COLOR_YELLOW
#define BTEXT_COLOR la_txt

    phantom_launcher_window = drv_video_window_create( scr_get_xsize(), 45,
                                                       0, 0, la_bg, "Launcher", WFLAG_WIN_ONTOP|WFLAG_WIN_NOKEYFOCUS );

    phantom_launcher_window->inKernelEventProcess = phantom_launcher_event_process;
    w_fill( phantom_launcher_window, la_bg );

    int lb_x = scr_get_xsize();
    int lb_y = 0;

    lb_x -= slide_switch_on_bmp.xsize + 5;
    //lb_y += slide_switch_on_bmp.ysize + 5;
    lb_y += 8;

    w_add_button( phantom_launcher_window, -1, lb_x, lb_y, &slide_switch_on_bmp, &slide_switch_off_bmp, BUTTON_FLAG_NOBORDER );

    pool_handle_t bh;

    lb_x = 5;

    int nwin = 0;
    for( nwin = 0; nwin < MAX_LAUNCH_BUTTONS; nwin++ )
    {
        char wname[10] = "Window 1";

        // crashes in some configurations??
        wname[7] = '0' + nwin;

        bh = w_add_button( phantom_launcher_window, nwin, lb_x, 0, &vanilla_task_button_bmp, &vanilla_task_button_bmp, BUTTON_FLAG_NOBORDER );
        w_button_set_text( phantom_launcher_window, bh, wname, BTEXT_COLOR );
        lb_x += 5+vanilla_task_button_bmp.xsize;

        taskbuttons[nwin] = bh;
    }


    w_draw_line( phantom_launcher_window, 0, 44, scr_get_xsize(), 44, la_b1 );
    w_draw_line( phantom_launcher_window, 0, 43, scr_get_xsize(), 43, la_b2 );

    w_update( phantom_launcher_window );
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
    //progress_rect.xsize = 0;

    extern int vm_map_do_for_percentage;

    progress_rect.xsize = DEBWIN_XS;
    w_fill_rect( phantom_debug_window, COLOR_GREEN, progress_rect );

    progress_rect.xsize = (vm_map_do_for_percentage*DEBWIN_XS)/100;
    w_fill_rect( phantom_debug_window, COLOR_LIGHTGREEN, progress_rect );
}

static void paint_memory_map(window_handle_t w);
static void paint_vaspace_map(window_handle_t w);

static void phantom_debug_window_loop()
{
    static char buf[DEBBS+1];
    int step = 0;

    int show = 's';

    t_current_set_name("Debug Win");
    // Which thread will receive typein for this window
    phantom_debug_window->owner = get_current_tid();

    int wx = 600;

#if CONF_NEW_CTTY
	if( t_new_ctty( get_current_tid() ) )
		panic("console t_new_ctty");
#else
    // Need separate ctty
    t_set_ctty( get_current_tid(), wtty_init( WTTY_SMALL_BUF ) );
#endif
    // TODO HACK! Need ioctl to check num of bytes?
    wtty_t *tty;
    t_get_ctty( get_current_tid(), &tty );


    while(1)
    {
        if(tty && !wtty_is_empty(tty))
        {
            char c = wtty_getc( tty );
            switch(c)
            {
            case '?':
            case'h':
                printf(
                       "Commands:\n"
                       "---------\n"
                       "w\t- show windows list\n"
                       "t\t- show threads list\n"
                       "s\t- show stats\n"
                       "p\t- show profiler\n"
                       "d\t- dump threads to JSON\n"
                       "m\t- show physical memory map\n"
                       "v\t- show virtual address space map\n"
                      );
                break;
            case 'p': // profiler
            case 't':
            case 'w':
            case 's':
            case 'm':
            case 'v':
                show = c;
                break;

            case 'd':
                {
                    json_output jo = { 0 };

                    json_start( &jo );
                    json_dump_threads( &jo );
                    json_stop( &jo );

                }
                break;
            }
        }


        {
            static char old_show = 0;
            if( old_show != show )
            {
                old_show = show;
                switch(show)
                {
                case 't': w_set_title( phantom_debug_window,  "Threads" );         break;
                case 'w': w_set_title( phantom_debug_window,  "Windows" );         break;
                case 's': w_set_title( phantom_debug_window,  "Stats" );           break;
                case 'p': w_set_title( phantom_debug_window,  "Profiler" );        break;
                case 'm': w_set_title( phantom_debug_window,  "Physical memory" ); break;
                case 'v': w_set_title( phantom_debug_window,  "Virtual address space" ); break;
                }
            }
        }


        //hal_sleep_msec(1000);
        hal_sleep_msec(100);
#if 1
#if 1
        w_clear( phantom_debug_window );
        ttyd = DEBWIN_YS-20;
        ttxd = 0;
#endif
        void *bp = buf;
        int len = DEBBS;
        int rc;

        time_t sec = uptime();
        int min = sec/60; sec %= 60;
        int hr = min/60; min %= 60;
        int days = hr/24; hr %= 24;

        struct tm mt = *current_time;

        rc = snprintf(bp, len, " View: \x1b[32mt\x1b[37mhreads \x1b[32ms\x1b[37mtats \x1b[32mw\x1b[37mindows \x1b[32mp\x1b[37mrofile       \x1b[32m?\x1b[37m - help\n \x1b[32mStep %d, uptime %dd, %02d:%02d:%02d\x1b[37m, %d events\n Time %04d/%02d/%02d %02d:%02d:%02d GMT, CPU 0 %2d%% idle\n",
                      step++, days, hr, min, (int)sec,
                      ev_get_n_events_in_q(),
                      mt.tm_year + 1900, mt.tm_mon, mt.tm_mday,
                      mt.tm_hour, mt.tm_min, mt.tm_sec, 100-percpu_cpu_load[0]
                     );
        bp += rc;
        len -= rc;

        switch(show)
        {
        case 't':
        default:            phantom_dump_threads_buf(bp,len);            break;
        case 'w':           phantom_dump_windows_buf(bp,len);            break;
        case 's':           phantom_dump_stats_buf(bp,len);              break;
        case 'p':           phantom_dump_profiler_buf(bp,len);           break;
        case 'm':           paint_memory_map(phantom_debug_window);      break;
        case 'v':           paint_vaspace_map(phantom_debug_window);     break;
        }

        phantom_debug_window_puts(buf);

        if(wx == 600) wx = 620; else wx = 600;
        //w_move( phantom_debug_window, wx, 50 );
#endif
        put_progress();
        w_update( phantom_debug_window );

    }
}

/*
static void phantom_launcher_window_loop()
{
    t_current_set_name("Debug Win");
    // Which thread will receive typein for this window
    phantom_launcher_window->owner = get_current_tid();

    // Need separate ctty
    t_set_ctty( get_current_tid(), wtty_init() );

    phantom_launcher_window->inKernelEventProcess = phantom_launcher_event_process;


}
*/


static int phantom_launcher_event_process( window_handle_t w, ui_event_t *e)
{

    switch(e->w.info)
    {
    default:
        return defaultWindowEventProcessor( w, e );

    case UI_EVENT_WIN_BUTTON:
        printf("launcher button %x\n", e->extra );
        {
            switch(e->extra)
            {
            case -1:
                phantom_shutdown(0);
                break;
            }
        }
    break;


    }

    return 1;
}


rect_t memory_map_rect = (rect_t){ 5, 10, 100, 100 };

#define HEAD_ROOM 64

static void paint_memory_map(window_handle_t w)
{
    memory_map_rect.ysize = w->ysize - HEAD_ROOM;
    memory_map_rect.xsize = w->xsize - (memory_map_rect.x * 2);

    paint_physmem_allocator_memory_map( w, &memory_map_rect );
}


static void paint_vaspace_map(window_handle_t w)
{
    memory_map_rect.ysize = w->ysize - HEAD_ROOM;
    memory_map_rect.xsize = w->xsize - (memory_map_rect.x * 2);

    paint_vaspace_allocator_memory_map( w, &memory_map_rect );
}



