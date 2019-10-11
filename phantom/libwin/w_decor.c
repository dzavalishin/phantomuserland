/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system bodrers and other decorations.
 *
**/

#include <assert.h>
#include <phantom_libc.h>

#include <video/window.h>
#include <video/internal.h>
#include <video/vops.h>
#include <video/font.h>
//#include <video/button.h>
#include <video/control.h>

#define border_0_color { .r = 210, .g = 203, .b = 188, .a = 0xFF } // bgr!
#define border_1_color { .r = 183, .g = 171, .b = 146, .a = 0xFF }
#define border_2_color { .r = 210, .g = 203, .b = 188, .a = 0xFF }

#if VIDEO_VANILLA
static rgba_t brdr[] = { border_0_color, border_1_color, border_2_color };
#else
static rgba_t brdr[] = {
    { 0x40, 0x40, 0x40, 0xFF },
    { 0x80, 0x80, 0x80, 0xFF },
    { 0x40, 0x40, 0x40, 0xFF },
};
#endif
rgba_t title_back_color_focus   = { .r = 213, .g = 172, .b = 101, .a = 0xFF };  // { 122, 230, 251, 0xFF };
rgba_t title_back_color_nofocus = { 214, 227, 231, 0xFF }; // border_1_color;  // { 122, 251, 195, 0xFF };

static const int bordr_size = sizeof(brdr)/sizeof(rgba_t);
static const int title_size = 31; //33; //18;

extern drv_video_bitmap_t vanilla_cream_rollup_bmp;
#define vanilla_cream_rollup_pressed_bmp vanilla_cream_rollup_bmp

extern drv_video_bitmap_t vanilla_cream_close_bmp;
#define vanilla_cream_close_pressed_bmp vanilla_cream_close_bmp


extern drv_video_bitmap_t title_selected_bmp;
extern drv_video_bitmap_t title_deselected_bmp;




void window_basic_border( drv_video_window_t *dest, const struct rgba_t *src, int srcSize )
{
    int stepOff = srcSize;
    w_replicate_ver( dest, 0, stepOff, dest->ysize-(2*stepOff), src, srcSize );
    w_replicate_ver( dest, dest->xsize-srcSize, stepOff, dest->ysize-(2*stepOff), src, srcSize );

    w_replicate_hor( dest, stepOff, 0, dest->xsize-(2*stepOff), src, srcSize );
    w_replicate_hor( dest, stepOff, dest->ysize-srcSize, dest->xsize-(2*stepOff), src, srcSize );

    rect_t r;
    r.xsize = r.ysize = stepOff;

    r.x = r.y = 0;
    w_fill_rect( dest, brdr[1], r );

    r.x = 0; r.y = dest->ysize-(stepOff);
    w_fill_rect( dest, brdr[1], r );

    r.y = 0; r.x = dest->xsize-(stepOff);
    w_fill_rect( dest, brdr[1], r );

    r.x = dest->xsize-(stepOff);
    r.y = dest->ysize-(stepOff);
    w_fill_rect( dest, brdr[1], r );
}







void win_make_decorations(drv_video_window_t *w)
{
    if(!(w->flags & WFLAG_WIN_DECORATED) ) return;

    w_assert_lock();

    int zless = (w->z == 0) ? 0 : (w->z - 1);

    if( 0 == w->w_decor)
    {
#if VIDEO_T_IN_D
        int dysize = w->ysize+bordr_size*3+title_size;
#else
        int dysize = w->ysize+bordr_size*2;
#endif
        drv_video_window_t *wdecor = private_drv_video_window_create(w->xsize+bordr_size*2, dysize );

        //wdecor->flags |= WFLAG_WIN_NOTINALL; // On destroy don't try to remove from allwindows
        wdecor->w_owner = w;
        w->w_decor = wdecor;

#if VIDEO_T_IN_D
        int bmp_y = w->ysize + bordr_size*2 + 2;
        // close button with id=1
        w_add_button( w->w_decor, WBUTTON_SYS_CLOSE, 5, bmp_y, &close_bmp, &close_pressed_bmp, 0 );
#endif

        // hangs?
        w->w_decor->flags &= ~WFLAG_WIN_DECORATED;
        iw_enter_allwq(w->w_decor);
    }

    w->w_decor->x = w->x-bordr_size;
    w->w_decor->y = w->y-bordr_size;
    w->w_decor->z = zless;

    //w->w_decor->bg = w->bg;
    w->w_decor->bg = COLOR_TRANSPARENT; // transparent!

    w_fill( w->w_decor, w->w_decor->bg );


#if !VIDEO_T_IN_D

    if(w->w_title == 0)
    {
        drv_video_window_t *wtitle =
            private_drv_video_window_create(
                                            w->xsize+bordr_size*2,
                                            title_size+bordr_size*2
                                           );

        //wtitle->flags |= WFLAG_WIN_NOTINALL; // On destroy don't try to remove from allwindows
        iw_enter_allwq(wtitle);

        wtitle->inKernelEventProcess = w_titleWindowEventProcessor;

        wtitle->w_owner = w;
        w->w_title = wtitle;

        int bwidth = vanilla_cream_close_bmp.xsize;
        int bxp = w->w_title->xsize - bwidth - 3;
        // close button 
        w_add_button( w->w_title, WBUTTON_SYS_CLOSE, bxp, 3, &vanilla_cream_close_bmp, &vanilla_cream_close_pressed_bmp, 0 );
        bxp -= bwidth + 0;
        // roll up button
        w_add_button( w->w_title, WBUTTON_SYS_ROLLUP, bxp, 3, &vanilla_cream_rollup_bmp, &vanilla_cream_rollup_pressed_bmp, 0 );
    }

    w->w_title->x = w->x-bordr_size;
    w->w_title->y = w->y+w->ysize; //+bordr_size;
    w->w_title->z = zless;

    int focused = w->state & WSTATE_WIN_FOCUSED;
    //if( w->w_title && w->w_title->state & WSTATE_WIN_FOCUSED) focused = 1;
    if( w->w_title->state & WSTATE_WIN_FOCUSED) focused = 1; // w_title can't be null here

    w->w_title->bg = focused ? title_back_color_focus : title_back_color_nofocus;
    w_fill( w->w_title, w->w_title->bg );

    //w_fill( w->w_title, w->w_title->bg );
    /*
    //drv_video_bitmap_t *tbmp = focused ?  &title_brown_bmp : &title_green_bmp;
    drv_video_bitmap_t *tbmp = focused ?  &title_violet_bmp : &title_green_bmp;
    w_replicate_hor( w->w_title, 3, 3, w->w_title->xsize, tbmp->pixel, tbmp->ysize );
    */

    drv_video_bitmap_t *tbmp = focused ?  &title_selected_bmp : &title_deselected_bmp;
    //if( focused )
    w_replicate_hor( w->w_title, 3, 3, w->w_title->xsize, tbmp->pixel, tbmp->ysize );

    window_basic_border( w->w_title, brdr, bordr_size );

    // BUG! It must be +3, not -1 on Y coord!
#if CONF_TRUETYPE
    w_ttfont_draw_string( w->w_title, decorations_title_font,
                                w->title, COLOR_BLACK, //COLOR_TRANSPARENT,
                                bordr_size+7, bordr_size+7 );
#else
    w_font_draw_string( w->w_title, &drv_video_8x16cou_font,
                                w->title, COLOR_BLACK, COLOR_TRANSPARENT,
                                bordr_size+3, bordr_size-1 );
#endif


    if( w->state & WSTATE_WIN_VISIBLE )
        iw_winblt_locked(w->w_title);
    //drv_video_window_free(wtitle);
#else
/*
    w->w_decor->inKernelEventProcess = titleWindowEventProcessor;

    int focused = w->state & WSTATE_WIN_FOCUSED;
    if( w->w_decor && w->w_decor->state & WSTATE_WIN_FOCUSED) focused = 1;

    color_t bg = focused ? title_back_color_focus : title_back_color_nofocus;

    rect_t r;
    r.x = bordr_size;
    r.y = w->ysize + bordr_size*2;
    r.xsize = w->xsize;
    r.ysize = title_size;

    w_fill_rect( w->w_decor, bg, r );


    int bmp_y = w->ysize + bordr_size*2 + 2;

    w_font_draw_string( w->w_decor, &drv_video_8x16cou_font,
                                w->title, COLOR_BLACK, COLOR_TRANSPARENT,
                                bordr_size+3, bmp_y-4 );

    drv_video_window_draw_bitmap( w->w_decor, w->w_decor->xsize - close_bmp.xsize - 5, bmp_y+1, &close_bmp );
    drv_video_window_draw_bitmap( w->w_decor, w->w_decor->xsize - pin_bmp.xsize - 2 - close_bmp.xsize - 5, bmp_y+1, &pin_bmp );


    // nSteps is x size, srcSize is y size
    w_replicate_hor( w->w_decor, 0, w->ysize + bordr_size,
                           w->w_decor->xsize, brdr, bordr_size );
*/
#endif

    w_repaint_controls(w->w_decor);
    w_repaint_controls(w->w_title);

    window_basic_border( w->w_decor, brdr, bordr_size );


    //if( (!(w->state & WSTATE_WIN_ROLLEDUP)) && (w->state & WSTATE_WIN_VISIBLE) )
    if( w->state & WSTATE_WIN_VISIBLE )
        iw_winblt_locked(w->w_decor);


    // replace setting pos here with
    //win_move_decorations(w);

}



void win_draw_decorations(drv_video_window_t *w)
{
    if(!(w->flags & WFLAG_WIN_DECORATED) ) return;

    w_assert_lock();
#if !VIDEO_T_IN_D
    if( (w->state & WSTATE_WIN_VISIBLE) && w->w_title )
        iw_winblt_locked(w->w_title);
#endif
    if( (!(w->state & WSTATE_WIN_ROLLEDUP)) && (w->state & WSTATE_WIN_VISIBLE) && w->w_decor )
        iw_winblt_locked(w->w_decor);
}




void win_move_decorations(drv_video_window_t *w)
{
    if(!(w->flags & WFLAG_WIN_DECORATED) ) return;

    w_assert_lock();

    int zless = (w->z == 0) ? 0 : (w->z - 1);

#if !VIDEO_T_IN_D
    if( 0 != w->w_title )
    {
        w->w_title->x = w->x-bordr_size;
        w->w_title->y = w->y+w->ysize; //+bordr_size;
        w->w_title->z = zless;
    }
#endif

    if( 0 != w->w_decor )
    {
        w->w_decor->x = w->x-bordr_size;
        w->w_decor->y = w->y-bordr_size;
        w->w_decor->z = zless;
    }
}








