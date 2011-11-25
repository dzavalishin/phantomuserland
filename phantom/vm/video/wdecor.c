#if 0
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
#include <event.h>
//#include <spinlock.h>

#include "win_local.h"

#include <video/window.h>
#include <video/internal.h>
#include <video/vops.h>
#include <video/font.h>
#include <video/button.h>





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
    w_fill_rect( dest, COLOR_DARKGRAY, r );

    r.x = 0; r.y = dest->ysize-(stepOff);
    w_fill_rect( dest, COLOR_DARKGRAY, r );

    r.y = 0; r.x = dest->xsize-(stepOff);
    w_fill_rect( dest, COLOR_DARKGRAY, r );

    r.x = dest->xsize-(stepOff);
    r.y = dest->ysize-(stepOff);
    w_fill_rect( dest, COLOR_DARKGRAY, r );
}




static rgba_t brdr[] = {
    { 0x40, 0x40, 0x40, 0xFF },
    { 0x80, 0x80, 0x80, 0xFF },
    { 0x40, 0x40, 0x40, 0xFF },
};

rgba_t title_back_color_focus   = { 122, 230, 251, 0xFF };
rgba_t title_back_color_nofocus = { 122, 251, 195, 0xFF };

static const int bordr_size = sizeof(brdr)/sizeof(rgba_t);
static const int title_size = 18;


//static int titleWindowEventProcessor( drv_video_window_t *w, struct ui_event *e );


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
        drv_video_window_t *w2 = private_drv_video_window_create(w->xsize+bordr_size*2, dysize );

        w2->flags |= WFLAG_WIN_NOTINALL; // On destroy don't try to remove from allwindows
        w2->w_owner = w;
        w->w_decor = w2;

#if VIDEO_T_IN_D
        int bmp_y = w->ysize + bordr_size*2 + 2;
        // close button with id=1
        w_add_button( w->w_decor, 1, 5, bmp_y, &close_bmp, &close_pressed_bmp, 0 );
#endif
    }

    w->w_decor->x = w->x-bordr_size;
    w->w_decor->y = w->y-bordr_size;
    w->w_decor->z = zless;

    w->w_decor->bg = w->bg;

    w_fill( w->w_decor, w->w_decor->bg );


#if !VIDEO_T_IN_D

    if(w->w_title == 0)
    {
        drv_video_window_t *w3 =
            private_drv_video_window_create(
                                            w->xsize+bordr_size*2,
                                            title_size+bordr_size*2
                                           );

        //w3->flags |= WFLAG_WIN_NOTINALL; // On destroy don't try to remove from allwindows
        drv_video_window_enter_allwq(w3);


        w3->inKernelEventProcess = w_titleWindowEventProcessor;

        w3->w_owner = w;
        w->w_title = w3;

        int bwidth = close_bmp.xsize;
        int bxp = w->w_title->xsize - bwidth - 5;
        // close button with id=1
        w_add_button( w->w_title, WBUTTON_SYS_CLOSE, bxp, 5, &close_bmp, &close_pressed_bmp, 0 );
        bxp -= bwidth + 2;
        // roll up button
        w_add_button( w->w_title, WBUTTON_SYS_ROLLUP, bxp, 5, &rollup_bmp, &rollup_pressed_bmp, 0 );
    }

    w->w_title->x = w->x-bordr_size;
    w->w_title->y = w->y+w->ysize; //+bordr_size;
    w->w_title->z = zless;

    int focused = w->state & WSTATE_WIN_FOCUSED;
    if( w->w_title && w->w_title->state & WSTATE_WIN_FOCUSED) focused = 1;

    w->w_title->bg = focused ? title_back_color_focus : title_back_color_nofocus;

    //w_fill( w->w_title, w->w_title->bg );

    //drv_video_bitmap_t *tbmp = focused ?  &title_brown_bmp : &title_green_bmp;
    drv_video_bitmap_t *tbmp = focused ?  &title_violet_bmp : &title_green_bmp;
    w_replicate_hor( w->w_title, 3, 3, w->w_title->xsize, tbmp->pixel, tbmp->ysize );

    window_basic_border( w->w_title, brdr, bordr_size );

    // BUG! It must be +3, not -1 on Y coord!
    w_font_draw_string( w->w_title, &drv_video_8x16cou_font,
                                w->title, COLOR_BLACK, COLOR_TRANSPARENT,
                                bordr_size+3, bordr_size-1 );

    //drv_video_window_draw_bitmap( w->w_title, w->w_title->xsize - close_bmp.xsize - 5, 5, &close_bmp );
    //drv_video_window_draw_bitmap( w->w_title, w->w_title->xsize - pin_bmp.xsize - 2 - close_bmp.xsize - 5, 5, &pin_bmp );

    if( w->state & WSTATE_WIN_VISIBLE )
        _drv_video_winblt_locked(w->w_title);
    //drv_video_window_free(w3);
#else
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

    drv_video_window_draw_bitmap( w->w_decor, w->w_decor->xsize - close_bmp.xsize - 5, bmp_y, &close_bmp );
    drv_video_window_draw_bitmap( w->w_decor, w->w_decor->xsize - pin_bmp.xsize - 2 - close_bmp.xsize - 5, bmp_y, &pin_bmp );


    // nSteps is x size, srcSize is y size
    w_replicate_hor( w->w_decor, 0, w->ysize + bordr_size,
                           w->w_decor->xsize, brdr, bordr_size );

#endif

    w_repaint_buttons(w->w_decor);
    w_repaint_buttons(w->w_title);

    //drv_video_window_draw_bitmap( w->w_decor, 5, bmp_y, &close_bmp );


    window_basic_border( w->w_decor, brdr, bordr_size );

    if( (!(w->state & WSTATE_WIN_ROLLEDUP)) && (w->state & WSTATE_WIN_VISIBLE) )
        _drv_video_winblt_locked(w->w_decor);


    // replace setting pos here with
    //win_move_decorations(w);

}



void win_draw_decorations(drv_video_window_t *w)
{
    if(!(w->flags & WFLAG_WIN_DECORATED) ) return;

    w_assert_lock();
#if !VIDEO_T_IN_D
    if( w->state & WSTATE_WIN_VISIBLE )
        _drv_video_winblt_locked(w->w_title);
#endif
    if( (!(w->state & WSTATE_WIN_ROLLEDUP)) && (w->state & WSTATE_WIN_VISIBLE) )
        _drv_video_winblt_locked(w->w_decor);
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








#endif
