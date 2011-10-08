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
#include <video/vops.h>

static int debug_print = 0;


// --------- window -------------------------------------------------------


static int si_window_5_tostring(struct pvm_object o, struct data_area_4_thread *tc )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(window)" ));
}



static int win_getXSize(struct pvm_object o, struct data_area_4_thread *tc )
{
    struct data_area_4_window      *da = pvm_data_area( o, window );
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_int_object( da->w.xsize ));
}

static int win_getYSize(struct pvm_object o, struct data_area_4_thread *tc )
{
    struct data_area_4_window      *da = pvm_data_area( o, window );
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_int_object( da->w.ysize ));
}

static int win_getX(struct pvm_object o, struct data_area_4_thread *tc )
{
    struct data_area_4_window      *da = pvm_data_area( o, window );
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_int_object( da->x ));
}

static int win_getY(struct pvm_object o, struct data_area_4_thread *tc )
{
    struct data_area_4_window      *da = pvm_data_area( o, window );
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_int_object( da->y ));
}





static int win_clear_20(struct pvm_object me , struct data_area_4_thread *tc )
{
    struct data_area_4_window      *da = pvm_data_area( me, window );

    DEBUG_INFO;

    da->x = da->y = 0;

    drv_video_window_fill( &(da->w), da->bg );
    drv_video_window_update( &(da->w) );

    SYSCALL_RETURN_NOTHING;
}

static int win_fill_21(struct pvm_object me , struct data_area_4_thread *tc )
{
    struct data_area_4_window      *da = pvm_data_area( me, window );
    DEBUG_INFO;

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 1);
    int color = POP_INT();

    rgba_t c;

    /*
    c.r = color >> 16;
    c.r = color >> 8;
    c.b = color >> 0;
    c.a = 0xFF;
    */

    INT32_TO_RGBA(c, color);

    drv_video_window_fill( &(da->w), c );

    //SYSCALL_THROW_STRING( "not implemented" );
    SYSCALL_RETURN_NOTHING;
}


static int win_setFGcolor_22(struct pvm_object me , struct data_area_4_thread *tc )
{
    struct data_area_4_window      *da = pvm_data_area( me, window );

    DEBUG_INFO;
    int n_param = POP_ISTACK;

    CHECK_PARAM_COUNT(n_param, 1);

    int color = POP_INT();
    //da->fg = color;

    /*
    da->fg.r = color >> 16;
    da->fg.g = color >> 8;
    da->fg.b = color >> 0;
    da->fg.a = 0xFF;
    */

    INT32_TO_RGBA(da->fg, color);

    SYSCALL_RETURN_NOTHING;
}

static int win_setBGcolor_23(struct pvm_object me , struct data_area_4_thread *tc )
{
    struct data_area_4_window      *da = pvm_data_area( me, window );

    DEBUG_INFO;
    int n_param = POP_ISTACK;

    CHECK_PARAM_COUNT(n_param, 1);

    int color = POP_INT();

    /*
    da->bg.r = color >> 16;
    da->bg.r = color >> 8;
    da->bg.b = color >> 0;
    da->bg.a = 0xFF;
    */

    INT32_TO_RGBA(da->bg, color);

    SYSCALL_RETURN_NOTHING;
}


// TODO need current font var for win and font set method?
#define tty_font &drv_video_8x16san_font


static int win_putString_24(struct pvm_object me , struct data_area_4_thread *tc )
{
    DEBUG_INFO;

    struct data_area_4_tty      *da = pvm_data_area( me, tty );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 3);

    struct pvm_object _text = POP_ARG;
    ASSERT_STRING(_text);

    int x = POP_INT();
    int y = POP_INT();


    int len = pvm_get_str_len( _text );
    const char * data = (const char *)pvm_get_str_data(_text);

#define BS 1024
    char buf[BS+2];

    if( len > BS ) len = BS;
    strncpy( buf, data, len );
    buf[len] = 0;

    SYS_FREE_O(_text);

    //printf("tty print: '%s' at %d,%d\n", buf, da->x, da->y );

    struct rgba_t fg = da->fg;
    struct rgba_t bg = da->bg;

    // TODO make a version of drv_video_font_tty_string that accepts non-zero terminated strings with len
    drv_video_font_tty_string( &(da->w), tty_font, buf, fg, bg, &x, &y );
    drv_video_window_update( &(da->w) );

    SYSCALL_RETURN_NOTHING;
}




static int win_putImage_25(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_window      *da = pvm_data_area( me, window );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 3);

    struct pvm_object _img = POP_ARG;
    int y = POP_INT();
    int x = POP_INT();

    // TODO check class!
    struct data_area_4_bitmap *_bmp = pvm_object_da( _img, bitmap );
    //struct data_area_4_tty *tty = pvm_object_da( _tty, tty );
    struct data_area_4_binary *pixels = pvm_object_da( _bmp->image, binary );

    bitmap2bitmap(
    		da->pixel, da->w.xsize, da->w.ysize, x, y,
    		(rgba_t *)pixels, _bmp->xsize, _bmp->ysize, 0, 0,
    		_bmp->xsize, _bmp->ysize
    );
    //drv_video_winblt( &(tty->w), tty->w.x, tty->w.y);
    drv_video_window_update( &(da->w) );

    SYS_FREE_O(_img);

    SYSCALL_RETURN_NOTHING;
}


static int win_setSize_26(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_window      *da = pvm_data_area( me, window );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 2);

    int y = POP_INT();
    int x = POP_INT();

    if(x*y > PVM_MAX_WIN_PIXELS)
        SYSCALL_THROW_STRING( "new win size > PVM_MAX_WIN_PIXELS" );

    drv_video_window_resize( &(da->w), x, y );

    SYSCALL_RETURN_NOTHING;
}

static int win_setPos_27(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_window      *da = pvm_data_area( me, window );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 2);

    int y = POP_INT();
    int x = POP_INT();

    drv_video_window_move( &(da->w), x, y );

    SYSCALL_RETURN_NOTHING;
}

static int win_drawLine_28(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_window      *da = pvm_data_area( me, window );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 4);

    int ys = POP_INT();
    int xs = POP_INT();
    int y = POP_INT();
    int x = POP_INT();

    drv_video_window_draw_line( &(da->w),
                                 x, y, x+xs, y+ys, da->fg );


    SYSCALL_RETURN_NOTHING;
}

// void	drawBox( var x : int, var y : int, var xsize : int, var ysize : int ) [26] {}

static int win_drawBox_29(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_window      *da = pvm_data_area( me, window );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 4);

    int ys = POP_INT();
    int xs = POP_INT();
    int y = POP_INT();
    int x = POP_INT();

    drv_video_window_draw_box( &(da->w), x, y, xs, ys, da->fg );

    SYSCALL_RETURN_NOTHING;
}

// void	fillBox( var x : int, var y : int, var xsize : int, var ysize : int ) [30] {}

static int win_fillBox_30(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_window      *da = pvm_data_area( me, window );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 4);

    int ys = POP_INT();
    int xs = POP_INT();
    int y = POP_INT();
    int x = POP_INT();

    drv_video_window_fill_box( &(da->w), x, y, xs, ys, da->fg );

    SYSCALL_RETURN_NOTHING;
}


// void	fillEllipse( var x : int, var y : int, var xsize : int, var ysize : int ) [31] {}

static int win_fillEllipse_31(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_window      *da = pvm_data_area( me, window );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 4);

    int ys = POP_INT();
    int xs = POP_INT();
    int y = POP_INT();
    int x = POP_INT();

    drv_video_window_fill_ellipse( &(da->w), x, y, xs, ys, da->fg );

    SYSCALL_RETURN_NOTHING;
}

// void	setEventHandler( var handler : .ru.dz.phantom.handler );

static int win_setHandler_32(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_window      *da = pvm_data_area( me, window );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 1);

    struct pvm_object handler = POP_ARG;

    // TODO check class!

    da->event_handler = handler;

    //SYS_FREE_O(_img);

    SYSCALL_RETURN_NOTHING;
}

static int win_setTitle_33(struct pvm_object me , struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_window      *da = pvm_data_area( me, window );


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

    drv_video_window_set_title( &(da->w), da->w.title );

    SYSCALL_RETURN_NOTHING;
}


syscall_func_t	syscall_table_4_window[32+8] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_window_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &invalid_syscall,    	    &invalid_syscall,
    &invalid_syscall, 	    	    &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode,
    // 16
    &win_getXSize, 	    	    &win_getYSize,
    &win_getX, 	    	    	    &win_getY,
    &win_clear_20,                  &win_fill_21,
    &win_setFGcolor_22,             &win_setBGcolor_23,
    // 24
    &win_putString_24,		    &win_putImage_25,
    &win_setSize_26,    	    &win_setPos_27,
    &win_drawLine_28, 	    	    &win_drawBox_29,
    &win_fillBox_30,                &win_fillEllipse_31,
    // 32
    &win_setHandler_32,  	    &win_setTitle_33,
    &invalid_syscall, 	    	    &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,

};
DECLARE_SIZE(window);
