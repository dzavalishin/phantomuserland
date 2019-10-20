/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal (native) classes implementation: VM TTY window 
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/


#define DEBUG_MSG_PREFIX "vm.sysc.tty"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>

#include "vm/object.h"
#include "vm/internal.h"
#include "vm/internal_da.h"
#include "vm/syscall.h"
#include "vm/root.h"
#include "vm/p2c.h"
#include "vm/alloc.h"

#include <console.h>

#include <video/screen.h>
#include <video/font.h>

#include <kernel/snap_sync.h>

static int debug_print = 0;

#define tty_font &drv_video_8x16san_font

#define BS 1024


//---------------------------------------------------------------------------
// Display
//---------------------------------------------------------------------------

/*
    static void validate_xy()
    {
    if( curr_xy.X < 0 ) curr_xy.X = 0;
    if( curr_xy.Y < 0 ) curr_xy.Y = 0;
    if( curr_xy.X >= consoleInfo.dwSize.X ) curr_xy.X = consoleInfo.dwSize.X-1;
    if( curr_xy.Y >= consoleInfo.dwSize.Y ) curr_xy.Y = consoleInfo.dwSize.Y-1;

    SetConsoleCursorPosition( hNewScreenBuffer, curr_xy );
    }

    static void advance_cursor(int written)
    {
    curr_xy.Y += written / consoleInfo.dwSize.X;
    curr_xy.X += written % consoleInfo.dwSize.X;

    // Need scrolling here?
    validate_xy();
    }
*/


static int tostring_5( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    DEBUG_INFO;
    SYSCALL_RETURN( pvm_create_string_object( "tty window" ));
}

static int putws_17( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;

    struct data_area_4_tty      *da = pvm_data_area( me, tty );

    CHECK_PARAM_COUNT(1);

    pvm_object_t _text = args[0];
    ASSERT_STRING(_text);

    int len = pvm_get_str_len( _text );
    const char * data = (const char *)pvm_get_str_data(_text);

    char buf[BS+2];

    if( len > BS ) len = BS;
    strncpy( buf, data, len );
    //buf[len] = '\n';
    buf[len] = 0;

    SYS_FREE_O(_text);

    //printf("tty print: '%s' at %d,%d\n", buf, da->x, da->y );

    struct rgba_t fg = da->fg;
    struct rgba_t bg = da->bg;

    // TODO w_font_tty_string_n( &(da->w), tty_font, data, len, ...)
    w_font_tty_string( &(da->w), tty_font, buf, fg, bg, &(da->x), &(da->y) );
    w_update( &(da->w) );

    SYSCALL_RETURN_NOTHING;
}

static int getwc_16( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    DEBUG_INFO;
    char c[1];

    // wtty_t *tty;
    // t_get_ctty( get_current_tid(), &tty );
    // wtty_getc( tty );

    vm_unlock_persistent_memory();
    
    //c[0] = phantom_dev_keyboard_getc(); // TODO need to read from local window? wtty?
    //#warning fix me - phantom_window_getc() ?

    //c[0] = phantom_window_getc();
    c[0] = 0;
    vm_lock_persistent_memory();

    SYSCALL_RETURN( pvm_create_string_object_binary( c, 1 ));
}


static int debug_18( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    DEBUG_INFO;

    CHECK_PARAM_COUNT(1);

    pvm_object_t o = args[0];

    printf("\n\nobj dump: ");
    dumpo((addr_t)(o));
    printf("\n\n");

    SYS_FREE_O(o);

    SYSCALL_RETURN_NOTHING;
}


static int gotoxy_19( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_tty      *da = pvm_data_area( me, tty );

    DEBUG_INFO;

    CHECK_PARAM_COUNT(2);

    int goy = AS_INT(args[1]);
    int gox = AS_INT(args[0]);

    da->x = da->font_width * gox;
    da->y = da->font_height * goy;

    SYSCALL_RETURN_NOTHING;
}

static int clear_20( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_tty      *da = pvm_data_area( me, tty );

    DEBUG_INFO;

    da->x = da->y = 0;

    w_fill( &(da->w), da->bg );
    w_update( &(da->w) );

    SYSCALL_RETURN_NOTHING;
}

static int setcolor_21( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    //struct data_area_4_tty      *da = pvm_data_area( me, tty );

    DEBUG_INFO;

    CHECK_PARAM_COUNT(1);

    int color = AS_INT(args[0]);
    (void) color;
    //int attr = (short)color;

    // TODO colors from attrs
    //printf("setcolor  font %d,%d\n", da->font_width, da->font_height );

    SYSCALL_RETURN_NOTHING;
}

static int fill_22( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    DEBUG_INFO;
    SYSCALL_THROW_STRING( "not implemented" );
}

static int putblock_23( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    DEBUG_INFO;
    SYSCALL_THROW_STRING( "not implemented" );
}



static int tty_setWinPos_24( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_tty      *da = pvm_data_area( me, tty );

    CHECK_PARAM_COUNT(2);

    int y = AS_INT(args[1]);
    int x = AS_INT(args[0]);

    w_move( &(da->w), x, y );

    SYSCALL_RETURN_NOTHING;
}

static int tty_setWinTitle_25( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_tty      *da = pvm_data_area( me, tty );

    CHECK_PARAM_COUNT(1);

    pvm_object_t _text = args[0];
    ASSERT_STRING(_text);

    int len = pvm_get_str_len( _text );
    const char * data = (const char *)pvm_get_str_data(_text);

    if( len > PVM_MAX_TTY_TITLE-1 ) len = PVM_MAX_TTY_TITLE-1 ;
    strlcpy( da->title, data, len+1 );
    //buf[len] = 0;

    SYS_FREE_O(_text);

    w_set_title( &(da->w), da->w.title );

    SYSCALL_RETURN_NOTHING;
}


static int setbgcolor_26( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    struct data_area_4_tty      *da = pvm_data_area( me, tty );

    DEBUG_INFO;

    CHECK_PARAM_COUNT(1);

    int color = AS_INT(args[0]);
    INT32_TO_RGBA(da->bg, color);

    SYSCALL_RETURN_NOTHING;
}


syscall_func_t  syscall_table_4_tty[32] =
{
    &si_void_0_construct,               &si_void_1_destruct,
    &si_void_2_class,                   &si_void_3_clone,
    &si_void_4_equals,                  &tostring_5,
    &si_void_6_toXML,                   &si_void_7_fromXML,
    // 8
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &si_void_15_hashcode,
    // 16
    &getwc_16,                          &putws_17,
    &debug_18,                          &gotoxy_19,
    &clear_20,                          &setcolor_21,
    &fill_22,                           &putblock_23,
    // 24
    &tty_setWinPos_24,                  &tty_setWinTitle_25,
    &setbgcolor_26,                     &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall,

};
DECLARE_SIZE(tty);


/*
pvm_object_t get_text_display_class()
{
    static pvm_object_t tdc;
    static int got_it = 0;

    if( got_it ) return tdc;
    { BLOCK_LOCK
    if( got_it ) return tdc;

    tdc = struct pvm_object_storage::
        create_internal_class(
            struct pvm_object_storage::create_string(".internal.io.tty"),
            get_void_class(), 0, 0,
            n_syscall_table_4_text_display,
            syscall_table_4_text_display );
    return tdc;
    }
}
*/


void pvm_internal_init_tty( pvm_object_t  ttyos )
{
    struct data_area_4_tty      *tty = (struct data_area_4_tty *)ttyos->da;

    tty->font_height = 16;
    tty->font_width = 8;
    tty->x = 0;
    tty->y = 0;
    tty->xsize = tty->w.xsize/tty->font_width;
    tty->ysize = tty->w.ysize/tty->font_height;
    tty->fg = COLOR_BLACK;
    tty->bg = COLOR_WHITE;

    strlcpy( tty->title, "VM TTY Window", sizeof(tty->title) );

    pvm_object_t bin = pvm_create_binary_object( drv_video_window_bytes( PVM_DEF_TTY_XSIZE, PVM_DEF_TTY_YSIZE ) + sizeof(drv_video_window_t), 0 );
    tty->o_pixels = bin;

    struct data_area_4_binary *bda = (struct data_area_4_binary *)bin->da;

    void *pixels = &bda->data;

    //void *pixels = &(tty->w) + sizeof(drv_video_window_t);

    drv_video_window_init( &(tty->w), pixels, PVM_DEF_TTY_XSIZE, PVM_DEF_TTY_YSIZE, 100, 100, tty->bg, WFLAG_WIN_DECORATED, tty->title );

    //lprintf("pvm_internal_init_tty %p pix %p", &(tty->w), pixels );


    w_clear( &(tty->w) );
    //w_update( &(tty->w) );

    pvm_add_object_to_restart_list( ttyos );
}

void pvm_gc_iter_tty(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    (void) func;
    (void) os;
    (void) arg;
    // Empty
}

void pvm_gc_finalizer_tty( pvm_object_t os )
{
    struct data_area_4_tty      *tty = (struct data_area_4_tty *)os->da;
    drv_video_window_destroy(&(tty->w));
}


void pvm_restart_tty( pvm_object_t o )
{
    pvm_add_object_to_restart_list( o ); // Again!

    struct data_area_4_tty *tty = pvm_object_da( o, tty );

    //lprintf( "restart TTY %p\n", tty );

    void *pixels = ((void*)&(tty->w)) + sizeof(drv_video_window_t);

    w_restart_init( &tty->w, pixels );

    tty->w.title = tty->title; // need? must be correct in snap

    // TODO BUG! How do we fill owner? We must have object ref here
    //tty->w.owner = -1;

    //tty->w.buttons = 0; // ? TODO how do we resetup 'em?

    w_restart_attach( &tty->w );
}

