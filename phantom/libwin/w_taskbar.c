/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * UI: Task Bar
 * 
 * Special control in the lower part of screen which 
 * displays running programs' icons and lets user to
 * control windows visibility.
 *
**/

#define DEBUG_MSG_PREFIX "ui.ctl"
#include <debug_ext.h>
#define debug_level_flow 1
#define debug_level_error 10
#define debug_level_info 1


#include <phantom_types.h>
#include <phantom_libc.h>
#include <assert.h>

#include <kernel/pool.h>

#include <video/rect.h>
#include <video/window.h>
#include <video/bitmap.h>
#include <video/font.h>
#include <video/internal.h>
#include <video/screen.h>
#include <video/control.h>
#include <video/builtin_bitmaps.h>

#include <dev/key_event.h>

#if NEW_TASK_BAR

// -----------------------------------------------------------------------
// Internals
// -----------------------------------------------------------------------

typedef struct {
    window_handle_t         w;              //< Owning window
    drv_video_bitmap_t     *icon;           //< How to show us

    control_handle_t        c;              //< This is what shown in task bar

    int                     notif_count;    //< Count of notifications - to display small number above the icon
} task_bar_el_t;

// -----------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------

// TODO check to be unique, better bring all pool magics to one header
//#define TASK_BAR_POOL_MAGIC ( ('t' << 8) | 'b' ) // can't be 2 bytes
#define TASK_BAR_POOL_MAGIC ( 't' ^ 'b' )

#define TB_HEIGHT 47
#define TB_START_X 200

#define TB_X_SHIFT (toolbar_icon_calendar_normal_bmp.xsize)


static color_t tb_bg = { .r = 209, .g = 202, .b = 186, .a = 0xFF };
static color_t tb_b2 = { 214, 227, 231, 0xFF };
static color_t tb_b1 = { .r = 183, .g = 171, .b = 146, .a = 0xFF  };

// -----------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------

static pool_t *task_bar;
static window_handle_t task_bar_window = 0;
static window_handle_t start_menu_window = 0;

static int task_bar_next_x = TB_START_X; // TODO hack
static int task_bar_next_y = 3;


// -----------------------------------------------------------------------
// Local prototypes
// -----------------------------------------------------------------------

static void create_start_menu( void );

static void  do_tb_destroy(void *pool_elem);
static void *do_tb_create(void *arg);

static void task_bar_reorder_buttons( void );

static void task_bar_callback(window_handle_t w, struct control *cc);

#endif

// -----------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------

void init_task_bar(void)
{
#if NEW_TASK_BAR
    task_bar = create_pool();
    assert(task_bar != 0 );

    task_bar->flag_autoclean = 0;
    task_bar->flag_autodestroy = 0;

    task_bar->magic = TASK_BAR_POOL_MAGIC;

    task_bar->init    = do_tb_create;
    task_bar->destroy = do_tb_destroy;


    task_bar_window = drv_video_window_create( 
            scr_get_xsize(), TB_HEIGHT,
            0, 0, tb_bg, "Task Bar", WFLAG_WIN_ONTOP|WFLAG_WIN_NOKEYFOCUS );

    w_fill( task_bar_window, tb_bg ); // Crashes with SEGV inusermode on mem fill (rep stos)

    int lb_x = scr_get_xsize();
    lb_x -= slide_switch_alpha_v31_off_bmp.xsize + 5;

    control_handle_t shutdown = w_add_button( task_bar_window, -1, lb_x, 8, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_off_bmp, CONTROL_FLAG_NOBORDER );
    w_control_set_background( task_bar_window, shutdown, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_on_bmp, 0 );
    // TODO callbck for me

    create_start_menu();

    w_draw_line( task_bar_window, 0, TB_HEIGHT-1, scr_get_xsize(), TB_HEIGHT-1, tb_b1 );
    w_draw_line( task_bar_window, 0, TB_HEIGHT-2, scr_get_xsize(), TB_HEIGHT-2, tb_b2 );

    w_update( task_bar_window );
#endif
}

#if NEW_TASK_BAR

// -----------------------------------------------------------------------
// Pool element create/kill
// -----------------------------------------------------------------------

static void  do_tb_destroy(void *pool_elem)
{
    free(pool_elem);
}

static void *do_tb_create(void *arg)
{
    assert(arg == 0);
    return calloc(sizeof(task_bar_el_t), 1);
}

// -----------------------------------------------------------------------
// Task bar button callback
// -----------------------------------------------------------------------


static void task_bar_callback(window_handle_t w, struct control *cc) { 
    (void)w;
    //LOG_FLOW( 0, "toggle %x", cc->callback_arg_p );

    taskbar_handle_t h = (taskbar_handle_t)cc->callback_arg_p;

    task_bar_el_t *e = pool_get_el( task_bar, h );
    if( 0 == e )
    {
        LOG_ERROR( 1, "invalid handle %d", h );
        return;
    }

    // bring on top if not
    if( !iw_is_top(e->w) ) 
    {
        w_set_visible( e->w, 1 );
        //w_to_top( e->w ); // TODO can't - recurse mutex - send message 
        ev_q_put_win( 0, 0, UI_EVENT_WIN_TO_TOP, e->w );
        
        if(cc->state != cs_pressed) // TODO quite a hack? call method? Need internal ictl_set_state( cc, state )?
        {
            cc->state = cs_pressed;
            w_paint_control( cc->w, cc );
        }
    }
    else
        w_set_visible( e->w, cc->state == cs_pressed );

    pool_release_el( task_bar, h );
    }

// -----------------------------------------------------------------------
// Add/remove windows to task bar
// -----------------------------------------------------------------------

taskbar_handle_t w_add_to_task_bar_ext( window_handle_t w, drv_video_bitmap_t *icon,
        drv_video_bitmap_t *n_bmp,
        drv_video_bitmap_t *p_bmp,
        drv_video_bitmap_t *h_bmp
    )
{
    control_handle_t bh;   

    assert( w != 0 );

    // TODO take mutex

    bh = w_add_button( task_bar_window, 0, task_bar_next_x, task_bar_next_y, n_bmp, p_bmp, CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE );
    if( INVALID_POOL_HANDLE == bh )
        return INVALID_POOL_HANDLE;

    w_control_set_background( task_bar_window, bh, n_bmp, p_bmp, h_bmp );
    w_control_set_state( task_bar_window, bh, 1 );
    w_control_set_icon( task_bar_window, bh, icon );

    task_bar_next_x += 5 + TB_X_SHIFT;

    taskbar_handle_t h = pool_create_el( task_bar, 0 );
    if( h == INVALID_POOL_HANDLE )
    {
        w_delete_control( task_bar_window, bh );
        return INVALID_POOL_HANDLE;
    }
    task_bar_el_t *e = pool_get_el( task_bar, h );
    assert( e != 0 );

    e->w = w;
    e->c = bh;

    w_control_set_callback( task_bar_window, bh, task_bar_callback, (void *)h );

    pool_release_el( task_bar, h );

    w->task_bar_h = h;

    return h;
}

taskbar_handle_t w_add_to_task_bar_icon( window_handle_t w, drv_video_bitmap_t *icon )
{
    return w_add_to_task_bar_ext( w, icon,
        &toolbar_icon_calendar_normal_bmp, 
        &toolbar_icon_calendar_pressed_bmp, 
        &toolbar_icon_calendar_hover_bmp    
    );
}

taskbar_handle_t w_add_to_task_bar( window_handle_t w )
{
    return w_add_to_task_bar_icon( w, 0 );
}



errno_t w_remove_from_task_bar( window_handle_t w )
{
    assert( w );
    taskbar_handle_t t = w->task_bar_h;

    task_bar_el_t *e = pool_get_el( task_bar, t );
    assert( e != 0 );

    w_delete_control( task_bar_window, e->c );

    pool_release_el( task_bar, t );
    pool_destroy_el( task_bar, t );

    task_bar_reorder_buttons();
    return 0;
}

// -----------------------------------------------------------------------
// Reorder
// -----------------------------------------------------------------------

static errno_t do_reorder(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) arg;

    task_bar_el_t *e = el;
    assert( e != 0 );

    //w_control_set_visible( task_bar_window, e->c, 0 );
    w_control_set_position( task_bar_window, e->c, task_bar_next_x, task_bar_next_y );
    //w_control_set_visible( task_bar_window, e->c, 1 );

    task_bar_next_x += 5 + TB_X_SHIFT;

    return 0;
}

static void task_bar_reorder_buttons( void )
{
    // TODO take mutex

    task_bar_next_x = TB_START_X;

    // Clear all controls - remaining ones will be redrawn
    w_fill( task_bar_window, tb_bg ); 

    pool_foreach( task_bar, do_reorder, 0 );

    w_update( task_bar_window );

    // Note that task_bar_next_x left with correct value for
    // next icon to add
}

// -----------------------------------------------------------------------
// Getters/setters
// -----------------------------------------------------------------------

void w_set_task_bar_icon( window_handle_t w, drv_video_bitmap_t *bmp )
{
    assert( w );
    taskbar_handle_t t = w->task_bar_h;

    if( t == 0 ) return;

    task_bar_el_t *e = pool_get_el( task_bar, t );
    if( 0 == e )
    {
        LOG_ERROR( 1, "invalid handle %x", t );
        return;
    }

    w_control_set_icon( task_bar_window, e->c, bmp );

    pool_release_el( task_bar, t );
}


void w_add_notification( window_handle_t w, int count_to_add )
{
    assert( w );
    taskbar_handle_t t = w->task_bar_h;

    if( t == 0 ) return;

    task_bar_el_t *e = pool_get_el( task_bar, t );
    if( 0 == e )
    {
        LOG_ERROR( 1, "invalid handle %x", t );
        return;
    }

    e->notif_count += count_to_add;

    // w_is_in_focus is a very cheap check and usually true for topmost ones
    if( w_is_in_focus(w) && w_is_top(w) ) e->notif_count = 0; 

    w_control_set_notify( task_bar_window, e->c, e->notif_count );

    pool_release_el( task_bar, t );
}

void w_reset_notification( window_handle_t w )
{
    assert( w );
    taskbar_handle_t t = w->task_bar_h;

    if( t == 0 ) return;

    task_bar_el_t *e = pool_get_el( task_bar, t );
    if( 0 == e )
    {
        LOG_ERROR( 1, "invalid handle %x", t );
        return;
    }

    e->notif_count = 0;

    w_control_set_notify( task_bar_window, e->c, e->notif_count );

    pool_release_el( task_bar, t );
}


// -----------------------------------------------------------------------
// Start Menu
// -----------------------------------------------------------------------

static void create_start_menu( void )
{
    // -----------------------------
    // Create
    // -----------------------------

    pool_handle_t bh;

    color_t menu_border = (color_t){.r = 0xA0, .g = 0xA0, .b = 0xA0, .a = 255};

    start_menu_window = drv_video_window_create( 
            200, 186,
            9, TB_HEIGHT, COLOR_WHITE, 
            "Menu", WFLAG_WIN_ONTOP|WFLAG_WIN_NOKEYFOCUS );

    window_handle_t lmw = start_menu_window;
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
    w_control_set_background( lmw, bh, &slide_switch_alpha_v31_off_bmp, &slide_switch_alpha_v31_on_bmp, 0 );
    // TODO callback for me

    // -----------------------------
    // Start button
    // -----------------------------


    bh = w_add_button( task_bar_window, -2, 5, 5, &start_button_normal_bmp, &start_button_selected_bmp, CONTROL_FLAG_NOBORDER|CONTROL_FLAG_TOGGLE );
    w_control_set_children( task_bar_window, bh, start_menu_window, 0 );
    //w_control_set_flags( task_bar_window, bh, CONTROL_FLAG_TOGGLE, 0 );
    w_control_set_background( task_bar_window, bh, 
        &start_button_normal_bmp, &start_button_selected_bmp, &start_button_hover_bmp  );

}

#endif
