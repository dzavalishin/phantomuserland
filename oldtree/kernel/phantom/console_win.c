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
#include <video/control.h>
#include <video/builtin_bitmaps.h>

#include "console.h"
#include "misc.h"

#include <threads.h>
#include <kernel/timedcall.h>
#include <kernel/debug.h>
#include <kernel/debug_graphical.h>
#include <kernel/stats.h>
#include <kernel/profile.h>
#include <kernel/init.h>

#include <kernel/json.h>

#if NET_TIMED_FLUSH
#include <kernel/net_timer.h>
#endif


void create_settings_window( void ); // tmp
static window_handle_t make_debug_w_context_menu(void);


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
drv_video_window_t *phantom_settings_window = 0;
#endif

#if !NEW_TASK_BAR
static window_handle_t phantom_launcher_window = 0;
static int phantom_launcher_event_process( window_handle_t w, ui_event_t *e);
drv_video_window_t *phantom_launcher_menu_window = 0;
#endif

int volatile debug_mode_selector = 's';


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

    w_add_notification( phantom_console_window, 1 );

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

#if !NEW_TASK_BAR
#define MAX_LAUNCH_BUTTONS 3
static pool_handle_t taskbuttons[MAX_LAUNCH_BUTTONS];
#endif

static void phantom_debug_window_loop();
//static void phantom_launcher_window_loop();

//extern drv_video_bitmap_t vanilla_task_button_bmp;
//extern drv_video_bitmap_t slide_switch_on_bmp;
//extern drv_video_bitmap_t slide_switch_off_bmp;
static void cc_arg_win_OnOff(window_handle_t w, struct control *cc) { 
    (void)w;
    LOG_FLOW( 0, "toggle %x", cc->callback_arg_p );
    int on = cc->state == cs_pressed;
    w_set_visible( cc->callback_arg_p, on );    
    }


static void debug_mode(window_handle_t w, struct control *cc) { 
    (void)w;
    debug_mode_selector = (char)((int)cc->callback_arg);
    LOG_FLOW( 0, "set mode '%c'", debug_mode_selector );
    }

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


#if !NEW_TASK_BAR
    // -------------------------------------------------------------------
    // Launcher window
    // -------------------------------------------------------------------

    //color_t la_bg = { 214, 227, 231, 0xFF };
    //color_t la_b1 = { 214, 227, 231, 0xFF };
    //color_t la_b2 = { .r = 183, .g = 171, .b = 146, .a = 0xFF  };

    color_t la_b2 = { 214, 227, 231, 0xFF };
    color_t la_b1 = { .r = 183, .g = 171, .b = 146, .a = 0xFF  };
    //color_t la_bg = la_b1;
    //color_t la_bg = { .r = 210, .g = 204, .b = 189, .a = 0xFF };
    color_t la_bg = { .r = 209, .g = 202, .b = 186, .a = 0xFF };

    //color_t la_txt = { 0x11, 0xd5, 0xff, 0xFF };
    //color_t la_txt = { 0x0, 0x0, 0x0, 0xFF };

#define LW_HEIGHT 47 // was 45

    phantom_launcher_window = drv_video_window_create( scr_get_xsize(), LW_HEIGHT,
                                                       0, 0, la_bg, "Launcher", WFLAG_WIN_ONTOP|WFLAG_WIN_NOKEYFOCUS );
    window_handle_t plw = phantom_launcher_window;

    phantom_launcher_window->inKernelEventProcess = phantom_launcher_event_process;
    w_fill( phantom_launcher_window, la_bg );

    // -----------------------------
    // Start Menu
    // -----------------------------

    pool_handle_t bh;

    color_t menu_border = (color_t){.r = 0xA0, .g = 0xA0, .b = 0xA0, .a = 255};

    phantom_launcher_menu_window = drv_video_window_create( 200, 186 /*+32*/,
                                                       9, LW_HEIGHT, COLOR_WHITE, 
                                                       "Menu", WFLAG_WIN_ONTOP|WFLAG_WIN_NOKEYFOCUS );
    window_handle_t lmw = phantom_launcher_menu_window;
    w_set_visible( lmw, 0 );

    //w_set_bg_color( lmw, COLOR_WHITE );
    w_fill_box(lmw, 0, 0, 200, 200, COLOR_WHITE );
    w_draw_box( lmw, 0, 0, 200, 200, menu_border );
    int menu_xsize = lmw->xsize-2;
    
    bh = w_add_menu_item( lmw, 0, 1, 1+31*5, menu_xsize, "Shell", COLOR_BLACK );
    w_control_set_icon( lmw, bh, &icon_home_bmp );

    bh = w_add_menu_item( lmw, 0, 1, 1+31*4, menu_xsize, "Settings", COLOR_BLACK );
    w_control_set_icon( lmw, bh, &icon_settings_bmp );

    bh = w_add_menu_item( lmw, 0, 1, 1+31*3, menu_xsize, "Kernel stats", COLOR_BLACK );
    w_control_set_icon( lmw, bh, &icon_key_bmp );
    
    bh = w_add_menu_item( lmw, 0, 1, 1+31*2, menu_xsize, "Weather", COLOR_BLACK );
    w_control_set_icon( lmw, bh, &icon_heart_bmp );

    w_add_menu_item( lmw, 0, 1, 1+31*1, menu_xsize, "Clock", COLOR_BLACK );

    // before slide for it to paint over us
    w_add_label( lmw, 1, 1, 200, 32, "Fast Snap", COLOR_BLACK );

    bh = w_add_button( lmw, 0, 128, 2+31*0, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_on_bmp, CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE );
    w_control_set_background( w, bh, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_on_bmp, 0 );
    //w_ttfont_draw_string( lmw, decorations_title_font, "Fast Snap", COLOR_BLACK, 10, 8 );    
    //w_add_label( lmw, 10, 8, lmw->xsize, 32, "Fast Snap", COLOR_BLACK );
    //w_ttfont_draw_string( lmw, w_get_system_font_ext(20), "Fast Snap", COLOR_BLACK, 10, 8 );
    //w_control_set_text( lmw, bh, "Fast Snap", COLOR_BLACK );

    //bh = w_add_text_field( lmw, 1, 1+31*6, 170, 31, "Hell", COLOR_BLACK );

#endif
    create_settings_window();

#if NEW_TASK_BAR

    w_add_to_task_bar_icon( phantom_console_window, &icon_screen_bmp );
    w_add_to_task_bar_icon( phantom_debug_window, &icon_text_bmp );
    w_add_to_task_bar_icon( phantom_settings_window, &icon_settings_bmp );

    w_set_task_bar_menu( phantom_debug_window, make_debug_w_context_menu() );

#else
    // -----------------------------
    // Buttons
    // -----------------------------

    int lb_x = scr_get_xsize();
    int lb_y = 0;

    lb_x -= slide_switch_on_bmp.xsize + 5;
    //lb_y += slide_switch_on_bmp.ysize + 5;
    lb_y += 8;

    w_add_button( phantom_launcher_window, -1, lb_x, lb_y, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_off_bmp, CONTROL_FLAG_NOBORDER );
    w_control_set_background( w, bh, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_on_bmp, 0 );

    //bh = w_add_menu_item( phantom_launcher_window, -2, 5, 3, 39, 0, COLOR_BLACK );
    bh = w_add_button( phantom_launcher_window, -2, 5, 5, &start_button_normal_bmp, &start_button_selected_bmp, CONTROL_FLAG_NOBORDER );
    w_control_set_children( phantom_launcher_window, bh, phantom_launcher_menu_window, 0 );
    w_control_set_flags( phantom_launcher_window, bh, CONTROL_FLAG_TOGGLE, 0 );
    w_control_set_background( phantom_launcher_window, bh, 
        &start_button_normal_bmp, &start_button_selected_bmp, &start_button_hover_bmp  );




    lb_x = 300;
#if 0
    int nwin = 0;
    for( nwin = 0; nwin < MAX_LAUNCH_BUTTONS; nwin++ )
    {
        char wname[10] = "Window 1";
        wname[7] = '0' + nwin;

        bh = w_add_button( phantom_launcher_window, nwin, lb_x, 0, &vanilla_task_button_bmp, &vanilla_task_button_bmp, CONTROL_FLAG_NOBORDER );
        w_control_set_text( phantom_launcher_window, bh, wname, BTEXT_COLOR );
        lb_x += 5+vanilla_task_button_bmp.xsize;

        taskbuttons[nwin] = bh;
    }
#else
        // Launcher task icons
        lb_y = 3;

        bh = w_add_button( plw, 'a', lb_x, lb_y, &toolbar_icon_shell_normal_bmp, &toolbar_icon_shell_pressed_bmp, CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE );
        w_control_set_background( plw, bh, &toolbar_icon_shell_normal_bmp, &toolbar_icon_shell_pressed_bmp, &toolbar_icon_shell_hover_bmp );
        lb_x += 5+toolbar_icon_shell_normal_bmp.xsize;
        w_control_set_callback( plw, bh, cc_arg_win_OnOff, phantom_console_window );
        w_control_set_state( plw, bh, 1 );


        bh = w_add_button( plw, 'b', lb_x, lb_y, &toolbar_icon_debug_normal_bmp, &toolbar_icon_debug_pressed_bmp, CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE );
        w_control_set_background( plw, bh, &toolbar_icon_debug_normal_bmp, &toolbar_icon_debug_pressed_bmp, &toolbar_icon_debug_hover_bmp );
        lb_x += 5+toolbar_icon_debug_normal_bmp.xsize;
        w_control_set_callback( plw, bh, cc_arg_win_OnOff, phantom_debug_window );
        w_control_set_state( plw, bh, 1 );

        bh = w_add_button( plw, 'b', lb_x, lb_y, &toolbar_icon_settings_normal_bmp, &toolbar_icon_settings_pressed_bmp, CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE );
        w_control_set_background( plw, bh, &toolbar_icon_settings_normal_bmp, &toolbar_icon_settings_pressed_bmp, &toolbar_icon_settings_hover_bmp );
        lb_x += 5+toolbar_icon_settings_normal_bmp.xsize;
        w_control_set_callback( plw, bh, cc_arg_win_OnOff, phantom_settings_window );
        w_control_set_state( plw, bh, 1 );

#endif

    w_draw_line( phantom_launcher_window, 0, LW_HEIGHT-1, scr_get_xsize(), LW_HEIGHT-1, la_b1 );
    w_draw_line( phantom_launcher_window, 0, LW_HEIGHT-2, scr_get_xsize(), LW_HEIGHT-2, la_b2 );

    w_update( phantom_launcher_window );
#endif    
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
static void paint_persistent_map(window_handle_t w);
static void paint_object_map(window_handle_t w);


static void phantom_debug_window_loop()
{
    static char buf[DEBBS+1];
    int step = 0;


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
                       "w\t- show windows list\n"     "t\t- show threads list\n"
                       "s\t- show stats\n"            "p\t- show profiler\n"
                       "d\t- dump threads to JSON\n"  "m\t- show physical memory map\n"
                       "v\t- show virtual address space map\n" "o\t- show objects (persistent) memory map\n"
                       "a\t- show object arenas memory map\n" 
                      );
                break;
            case 'p':            case 't':
            case 'w':            case 's':
            case 'm':            case 'v':
            case 'o':            case 'a':
                debug_mode_selector = c;
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
            static char old_debug_mode_selector = 0;
            if( old_debug_mode_selector != debug_mode_selector )
            {
                old_debug_mode_selector = debug_mode_selector;
                switch(debug_mode_selector)
                {
                case 't': w_set_title( phantom_debug_window,  "Threads" );         break;
                case 'w': w_set_title( phantom_debug_window,  "Windows" );         break;
                case 's': w_set_title( phantom_debug_window,  "Stats" );           break;
                case 'p': w_set_title( phantom_debug_window,  "Profiler" );        break;
                case 'm': w_set_title( phantom_debug_window,  "Physical memory" ); break;
                case 'v': w_set_title( phantom_debug_window,  "Virtual address space" ); break;
                case 'o': w_set_title( phantom_debug_window,  "Objects memory" );  break;                
                case 'a': w_set_title( phantom_debug_window,  "Objects allocator" );  break;                
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

        switch(debug_mode_selector)
        {
        case 't':
        default:            phantom_dump_threads_buf(bp,len);            break;
        case 'w':           phantom_dump_windows_buf(bp,len);            break;
        case 's':           phantom_dump_stats_buf(bp,len);              break;
        case 'p':           phantom_dump_profiler_buf(bp,len);           break;
        case 'm':           paint_memory_map(phantom_debug_window);      break;
        case 'v':           paint_vaspace_map(phantom_debug_window);     break;
        case 'a':           paint_object_map(phantom_debug_window);      break;
        case 'o':           paint_persistent_map(phantom_debug_window);  break;
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

    case UI_EVENT_WIN_BUTTON_ON:
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


static void paint_persistent_map(window_handle_t w)
{
    memory_map_rect.ysize = w->ysize - HEAD_ROOM;
    memory_map_rect.xsize = w->xsize - (memory_map_rect.x * 2);

    paint_persistent_memory_map( w, &memory_map_rect );
}


static void paint_object_map(window_handle_t w)
{
    memory_map_rect.ysize = w->ysize - HEAD_ROOM;
    memory_map_rect.xsize = w->xsize - (memory_map_rect.x * 2);

    vm_lock_persistent_memory();
    paint_object_memory_map( w, &memory_map_rect );
    vm_unlock_persistent_memory();
}



# if 0
static void consoleOnOff(window_handle_t w, struct control *c) { 
    (void)w;
    int on = c->state == cs_pressed;
    w_set_visible( phantom_console_window, on );    
    }

static void debugOnOff(window_handle_t w, struct control *c) { 
    (void)w;
    int on = c->state == cs_pressed;
    w_set_visible( phantom_debug_window, on );    
    }
#else
#endif


void create_settings_window( void )
{
    pool_handle_t bh;
    int cc_y = 0;
    int cc_x = 0;

    //color_t menu_border = (color_t){.r = 0xA0, .g = 0xA0, .b = 0xA0, .a = 255};

    window_handle_t w = drv_video_window_create( 400, 350, 20, 500, COLOR_WHITE, "Controls", WFLAG_WIN_DECORATED );
    phantom_settings_window = w;
    
    //w_set_visible( lmw, 0 );

    w_draw_bitmap( w, 0, 0, &vanilla_background_bmp );
    
    //w_fill_box( w, 0, 0, WXY, WXY, COLOR_WHITE );
    //w_draw_box( w, 0, 0, WXY, WXY, menu_border );

    

    bh = w_add_menu_item( w, '1', 20, 300, 200, "Settings", COLOR_BLACK );
    w_control_set_icon( w, bh, &icon_settings_bmp );

    // slide must paint over
    //w_add_label( w, 20, 250, 200, 32, "Fast Snap", COLOR_BLACK );
    w_add_label_transparent( w, 20, 250, 200, 32, "Fast Snap", COLOR_BLACK );

    bh = w_add_button( w, '2', 138, 250, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_on_bmp, CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE );
    w_control_set_background( w, bh, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_on_bmp, 0 );

    bh = w_add_text_field( w, 20, 200, 200, 31, "Hell", COLOR_BLACK );


    //bh = w_add_button( w, '3', 350, 300, &checkbox_square_off_a_x30_bmp, &checkbox_square_on_a_x30_bmp, CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE );
    //w_control_set_background( w, bh, &checkbox_square_off_a_x30_bmp, &checkbox_square_on_a_x30_bmp, 0 );
    bh = w_add_checkbox( w, 350, 300 );
    w_control_set_callback( w, bh, cc_arg_win_OnOff, phantom_console_window );
    w_control_set_state( w, bh, 1 );

    w_add_label_transparent( w, 240, 300, 50, 32, "Console", COLOR_BLACK );



    //bh = w_add_button( w, '4', 350, 250, &checkbox_square_off_a_x30_bmp, &checkbox_square_on_a_x30_bmp, CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE );
    //w_control_set_background( w, bh, &checkbox_square_off_a_x30_bmp, &checkbox_square_on_a_x30_bmp, 0 );
    bh = w_add_checkbox( w, 350, 250 );
    w_control_set_callback( w, bh, cc_arg_win_OnOff, phantom_debug_window );
    w_control_set_state( w, bh, 1 );

    w_add_label_transparent( w, 240, 250, 50, 32, "Debug", COLOR_BLACK );        

    // -------------------------------------------------------------------
    // Radio
    // -------------------------------------------------------------------

#define RADIO_SHIFT 35    
    cc_y = 150;
    cc_x = 240;

    bh = w_add_radio_button( w, 'x', 'g', cc_x+110, cc_y );
    w_control_set_callback( w, bh, debug_mode, (void *)'t' );
    w_add_label_transparent( w, cc_x, cc_y, 50, 32, "Threads", COLOR_BLACK );        

    cc_y -= RADIO_SHIFT;

    bh = w_add_radio_button( w, 'x', 'g', cc_x+110, cc_y );
    w_control_set_callback( w, bh, debug_mode, (void *)'s' );
    w_control_set_state( w, bh, 1 );
    w_add_label_transparent( w, cc_x, cc_y, 50, 32, "Stats", COLOR_BLACK );        

    cc_y -= RADIO_SHIFT;

    bh = w_add_radio_button( w, 'x', 'g', cc_x+110, cc_y );
    w_control_set_callback( w, bh, debug_mode, (void *)'p' );
    w_add_label_transparent( w, cc_x, cc_y, 50, 32, "Profiler", COLOR_BLACK );        




    cc_y = 150;
    cc_x = 20;

    bh = w_add_radio_button( w, 'x', 'g', cc_x+110, cc_y );
    w_control_set_callback( w, bh, debug_mode, (void *)'m' );
    w_add_label_transparent( w, cc_x, cc_y, 50, 32, "PhysMem", COLOR_BLACK );        

    cc_y -= RADIO_SHIFT;

    bh = w_add_radio_button( w, 'x', 'g', cc_x+110, cc_y );
    w_control_set_callback( w, bh, debug_mode, (void *)'v' );
    w_add_label_transparent( w, cc_x, cc_y, 50, 32, "VAddr", COLOR_BLACK );        

    cc_y -= RADIO_SHIFT;

    bh = w_add_radio_button( w, 'x', 'g', cc_x+110, cc_y );
    w_control_set_callback( w, bh, debug_mode, (void *)'o' );
    w_add_label_transparent( w, cc_x, cc_y, 50, 32, "ObjMem", COLOR_BLACK );        




    // -------------------------------------------------------------------
    // Ok/Cancel
    // -------------------------------------------------------------------



    bh = w_add_button( w, 'o', 20, 20, &button_normal_alpha_x98_bmp, &button_pressed_alpha_x98_bmp, 0 );
    w_control_set_background( w, bh, &button_normal_alpha_x98_bmp, &button_pressed_alpha_x98_bmp, &button_hover_alpha_x98_bmp );
    w_control_set_text( w, bh, "Ok", COLOR_BLACK );

    bh = w_add_button( w, '0', 220, 20, &button_normal_alpha_x98_bmp, &button_pressed_alpha_x98_bmp, 0 );
    w_control_set_background( w, bh, &button_normal_alpha_x98_bmp, &button_pressed_alpha_x98_bmp, &button_hover_alpha_x98_bmp );
    w_control_set_text( w, bh, "Cancel", COLOR_BLACK );
}

#if 1
static window_handle_t make_debug_w_context_menu(void)
{
    // -----------------------------
    // Start Menu
    // -----------------------------

    pool_handle_t bh;

    color_t menu_border = (color_t){.r = 0xA0, .g = 0xA0, .b = 0xA0, .a = 255};

    window_handle_t ctx_menu = drv_video_window_create( 200, 186 /*+32*/,
                                                       9, 47, COLOR_WHITE, 
                                                       "Menu", WFLAG_WIN_ONTOP|WFLAG_WIN_NOKEYFOCUS|WFLAG_WIN_HIDE_ON_FOCUS_LOSS );
    window_handle_t lmw = ctx_menu;
    w_set_visible( lmw, 0 );

    //w_set_bg_color( lmw, COLOR_WHITE );
    w_fill_box(lmw, 0, 0, 200, 200, COLOR_WHITE );
    w_draw_box( lmw, 0, 0, 200, 200, menu_border );
    int menu_xsize = lmw->xsize-2;
    
    bh = w_add_menu_item( lmw, 0, 1, 1+31*5, menu_xsize, "Threads", COLOR_BLACK );
    //w_control_set_icon( lmw, bh, &icon_home_bmp );
    w_control_set_callback( lmw, bh, debug_mode, (void *)'t' );

    bh = w_add_menu_item( lmw, 0, 1, 1+31*4, menu_xsize, "Windows", COLOR_BLACK );
    //w_control_set_icon( lmw, bh, &icon_settings_bmp );
    w_control_set_callback( lmw, bh, debug_mode, (void *)'w' );

    bh = w_add_menu_item( lmw, 0, 1, 1+31*3, menu_xsize, "Stats", COLOR_BLACK );
    //w_control_set_icon( lmw, bh, &icon_key_bmp );
    w_control_set_callback( lmw, bh, debug_mode, (void *)'s' );
    
    bh = w_add_menu_item( lmw, 0, 1, 1+31*2, menu_xsize, "Phys mem", COLOR_BLACK );
    //w_control_set_icon( lmw, bh, &icon_heart_bmp );
    w_control_set_callback( lmw, bh, debug_mode, (void *)'m' );

    bh = w_add_menu_item( lmw, 0, 1, 1+31*1, menu_xsize, "Virt addr", COLOR_BLACK );
    w_control_set_callback( lmw, bh, debug_mode, (void *)'v' );

    // before slide for it to paint over us
    w_add_label( lmw, 1, 1, 200, 32, "Fast Snap", COLOR_BLACK );

    bh = w_add_button( lmw, 0, 128, 2+31*0, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_on_bmp, CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE );
    w_control_set_background( lmw, bh, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_on_bmp, 0 );

    return ctx_menu;
}

#endif






