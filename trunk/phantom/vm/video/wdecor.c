/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system bodrers and other decorations.
 *
 *
**/

#include <drv_video_screen.h>
#include <assert.h>
#include <phantom_libc.h>
#include <event.h>
#include <spinlock.h>

#include "win_local.h"





void replicate2window_ver( drv_video_window_t *dest, int destX, int destY,
                           int nSteps, const struct rgba_t *src, int srcSize
                         )
{
    int srcShift = 0;
    if( destX < 0 )
    {
        srcShift = -destX;
        destX = 0;
    }

    if( destX+srcSize >= dest->xsize )
        srcSize = dest->xsize-destX;

    if( srcSize <= 0 ) return;

    if( destY < 0 )
    {
        nSteps += destY;
        destY = 0;
    }

    if( destY+nSteps >= dest->ysize )
        nSteps = dest->ysize - destY;

    src += srcShift;

    int vcnt;
    for(vcnt = destY; vcnt < destY+nSteps; vcnt++ )
    {
        struct rgba_t *dstPtr = dest->pixel + vcnt*dest->xsize + destX;
        rgba2rgba_move( dstPtr, src, srcSize );
    }
}


// nSteps is x size, srcSize is y size
void replicate2window_hor( drv_video_window_t *dest, int destX, int destY,
                           int nSteps, const struct rgba_t *src, int srcSize
                         )
{
    int srcShift = 0;
    if( destY < 0 )
    {
        srcShift = -destY;
        destY = 0;
    }

    if( destY+srcSize >= dest->ysize )
        srcSize = dest->ysize - destY;

    if( srcSize <= 0 ) return;

    if( destX < 0 )
    {
        nSteps += destX;
        destX = 0;
    }

    if( destX+nSteps >= dest->xsize )
        nSteps = dest->xsize-destX;

    src += srcShift;

    int vcnt;
    for(vcnt = destY; vcnt < destY+srcSize; vcnt++ )
    {
        struct rgba_t *dstPtr = dest->pixel + vcnt*dest->xsize + destX;
        rgba2rgba_replicate( dstPtr, src, nSteps );
        src++;
    }
}



void window_basic_border( drv_video_window_t *dest, const struct rgba_t *src, int srcSize )
{
    int stepOff = srcSize;
    replicate2window_ver( dest, 0, stepOff, dest->ysize-(2*stepOff), src, srcSize );
    replicate2window_ver( dest, dest->xsize-srcSize, stepOff, dest->ysize-(2*stepOff), src, srcSize );

    replicate2window_hor( dest, stepOff, 0, dest->xsize-(2*stepOff), src, srcSize );
    replicate2window_hor( dest, stepOff, dest->ysize-srcSize, dest->xsize-(2*stepOff), src, srcSize );

    rect_t r;
    r.xsize = r.ysize = stepOff;

    r.x = r.y = 0;
    drv_video_window_fill_rect( dest, COLOR_DARKGRAY, r );

    r.x = 0; r.y = dest->ysize-(stepOff);
    drv_video_window_fill_rect( dest, COLOR_DARKGRAY, r );

    r.y = 0; r.x = dest->xsize-(stepOff);
    drv_video_window_fill_rect( dest, COLOR_DARKGRAY, r );

    r.x = dest->xsize-(stepOff);
    r.y = dest->ysize-(stepOff);
    drv_video_window_fill_rect( dest, COLOR_DARKGRAY, r );

}





static rgba_t brdr[] = {
    { 0x40, 0x40, 0x40, 0xFF },
    { 0x80, 0x80, 0x80, 0xFF },
    { 0x40, 0x40, 0x40, 0xFF },
};

rgba_t title_back_color_focus   = { 122, 230, 251, 0xFF };
rgba_t title_back_color_nofocus = { 122, 251, 195, 0xFF };




void win_make_decorations(drv_video_window_t *w)
{
    int bordr_size = sizeof(brdr)/sizeof(rgba_t);
#if 1
    int title_size = 18;

    int zless = (w->z == 0) ? 0 : (w->z - 1);

    drv_video_window_t *w3 =
        private_drv_video_window_create(
                                        w->xsize+bordr_size*2,
                                        title_size+bordr_size*2
                                       );

    w3->flags |= WFLAG_WIN_NOTINALL; // On destroy don't try to remove from allwindows

    w3->x = w->x-bordr_size; w3->y = w->y+w->ysize; //+bordr_size;
    w3->z = zless;

    w3->bg = (w->state & WSTATE_WIN_FOCUSED) ? title_back_color_focus : title_back_color_nofocus;

    drv_video_window_fill( w3, w3->bg );
    window_basic_border( w3, brdr, bordr_size );

    // BUG! It must be +3, not -1 on Y coord!
    drv_video_font_draw_string( w3, &drv_video_8x16cou_font,
                                w->title, COLOR_BLACK,
                                bordr_size+3, bordr_size-1 );

    drv_video_window_draw_bitmap( w3, w3->xsize - close_bmp.xsize - 5, 5, &close_bmp );
    drv_video_window_draw_bitmap( w3, w3->xsize - pin_bmp.xsize - 2 - close_bmp.xsize - 5, 5, &pin_bmp );


    drv_video_winblt(w3);
    drv_video_window_free(w3);
#endif

    //int bordr_size = sizeof(brdr)/sizeof(rgba_t);
    drv_video_window_t *w2 = private_drv_video_window_create(w->xsize+bordr_size*2, w->ysize+bordr_size*2);

    w2->flags |= WFLAG_WIN_NOTINALL; // On destroy don't try to remove from allwindows
    //hal_spin_lock( &allw_lock );
    //queue_enter(&allwindows, w2, drv_video_window_t *, chain);
    //hal_spin_unlock( &allw_lock );

    w2->x = w->x-bordr_size; w2->y = w->y-bordr_size;
    w2->z = zless;

    w2->bg = w->bg;

    drv_video_window_fill( w2, w2->bg );
    window_basic_border( w2, brdr, bordr_size );

    drv_video_winblt(w2);
    drv_video_window_free(w2);
}

