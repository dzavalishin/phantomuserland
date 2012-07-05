/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/

#include <phantom_libc.h>


//#include "drv_video_screen.h"
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


static int tostring_5(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    DEBUG_INFO;
    SYSCALL_RETURN( pvm_create_string_object( "tty window" ));
}

static int putws_17(struct pvm_object me , struct data_area_4_thread *tc )
{
    DEBUG_INFO;

    struct data_area_4_tty      *da = pvm_data_area( me, tty );

    //printf("putws font %d,%d\n", da->font_width, da->font_height );


    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 1);

    struct pvm_object _text = POP_ARG;
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

    w_font_tty_string( &(da->w), tty_font, buf, fg, bg, &(da->x), &(da->y) );
    w_update( &(da->w) );

    SYSCALL_RETURN_NOTHING;
}

static int getwc_16(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    DEBUG_INFO;
    char c[1];

    // TODO XXX syscall blocks!
    c[0] = phantom_dev_keyboard_getc();

    SYSCALL_RETURN( pvm_create_string_object_binary( c, 1 ));
}


static int debug_18(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    DEBUG_INFO;

    //struct data_area_4_tty      *da = pvm_data_area( me, tty );


    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 1);

    struct pvm_object o = POP_ARG;

    //pvm_object_print( o );
    printf("\n\nobj dump: ");
    dumpo((addr_t)(o.data));
    printf("\n\n");

    SYS_FREE_O(o);

    SYSCALL_RETURN_NOTHING;
}


static int gotoxy_19(struct pvm_object me , struct data_area_4_thread *tc )
{
    struct data_area_4_tty      *da = pvm_data_area( me, tty );

    DEBUG_INFO;
    int n_param = POP_ISTACK;

    CHECK_PARAM_COUNT(n_param, 2);

    int goy = POP_INT();
    int gox = POP_INT();

    da->x = da->font_width * gox;
    da->y = da->font_height * goy;

    SYSCALL_RETURN_NOTHING;
}

static int clear_20(struct pvm_object me , struct data_area_4_thread *tc )
{
    struct data_area_4_tty      *da = pvm_data_area( me, tty );

    DEBUG_INFO;

    da->x = da->y = 0;

    w_fill( &(da->w), da->bg );
    w_update( &(da->w) );

    SYSCALL_RETURN_NOTHING;
}

static int setcolor_21(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    //struct data_area_4_tty      *da = pvm_data_area( me, tty );

    DEBUG_INFO;
    int n_param = POP_ISTACK;

    CHECK_PARAM_COUNT(n_param, 1);

    int color = POP_INT();
    (void) color;
    //int attr = (short)color;

    // TODO colors from attrs
    //printf("setcolor  font %d,%d\n", da->font_width, da->font_height );

    SYSCALL_RETURN_NOTHING;
}

static int fill_22(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    DEBUG_INFO;
    SYSCALL_THROW_STRING( "not implemented" );
}

static int putblock_23(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    DEBUG_INFO;
    SYSCALL_THROW_STRING( "not implemented" );
}



static int tty_setWinPos_24(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_tty      *da = pvm_data_area( me, tty );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 2);

    int y = POP_INT();
    int x = POP_INT();

    w_move( &(da->w), x, y );

    SYSCALL_RETURN_NOTHING;
}

static int tty_setWinTitle_25(struct pvm_object me , struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_tty      *da = pvm_data_area( me, tty );


    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 1);

    struct pvm_object _text = POP_ARG;
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



syscall_func_t	syscall_table_4_tty[32] =
{
    &si_void_0_construct,           	&si_void_1_destruct,
    &si_void_2_class,               	&si_void_3_clone,
    &si_void_4_equals,              	&tostring_5,
    &si_void_6_toXML,               	&si_void_7_fromXML,
    // 8
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&si_void_15_hashcode,
    // 16
    &getwc_16,    			&putws_17,
    &debug_18,               		&gotoxy_19,
    &clear_20,    			&setcolor_21,
    &fill_22,     			&putblock_23,
    // 24
    &tty_setWinPos_24,               	&tty_setWinTitle_25,
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&invalid_syscall,

};
DECLARE_SIZE(tty);


/*
struct pvm_object get_text_display_class()
{
    static struct pvm_object tdc;
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


void pvm_internal_init_tty( struct pvm_object_storage * ttyos )
{
    struct data_area_4_tty      *tty = (struct data_area_4_tty *)ttyos->da;

    /*
    tty->w.xsize = PVM_DEF_TTY_XSIZE;
    tty->w.ysize = PVM_DEF_TTY_YSIZE;
    tty->w.x = 100;
    tty->w.y = 100;
    */

    tty->font_height = 16;
    tty->font_width = 8;
    tty->x = 0;
    tty->y = 0;
    tty->xsize = tty->w.xsize/tty->font_width;
    tty->ysize = tty->w.ysize/tty->font_height;
    tty->fg = COLOR_BLACK;
    tty->bg = COLOR_WHITE;

    strlcpy( tty->title, "VM TTY Window", sizeof(tty->title) );

    drv_video_window_init( &(tty->w), PVM_DEF_TTY_XSIZE, PVM_DEF_TTY_YSIZE, 100, 100, tty->bg, WFLAG_WIN_DECORATED, tty->title );
    w_clear( &(tty->w) );
    //w_update( &(tty->w) );


    {
    pvm_object_t o;
    o.data = ttyos;
    o.interface = pvm_get_default_interface(ttyos).data;

    // This object needs OS attention at restart
    // TODO do it by class flag in create fixed or earlier?
    pvm_add_object_to_restart_list( o );
    }
}

void pvm_gc_iter_tty(gc_iterator_call_t func, struct pvm_object_storage * os, void *arg)
{
    (void) func;
    (void) os;
    (void) arg;
    // Empty
}

void pvm_gc_finalizer_tty( struct pvm_object_storage * os )
{
    struct data_area_4_tty      *tty = (struct data_area_4_tty *)os->da;
    drv_video_window_destroy(&(tty->w));
}


void pvm_restart_tty( pvm_object_t o )
{
    pvm_add_object_to_restart_list( o ); // Again!

    struct data_area_4_tty *tty = pvm_object_da( o, tty );

    printf( "restart TTY %p\n", tty );

    w_restart_init( &tty->w );

    tty->w.title = tty->title; // need? must be correct in snap

    // BUG! How do we fill owner? We must have object ref here
    //tty->w.owner = -1;

    //tty->w.buttons = 0; // ? TODO how do we resetup 'em?

    w_restart_attach( &tty->w );
}

