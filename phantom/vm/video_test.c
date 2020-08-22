/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests for video subsystem.
 *
**/

#define DEBUG_MSG_PREFIX "vm.vtest"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>

#include <video/screen.h>
#include <video/font.h>

#include <unistd.h>
#include <kunix.h>

#include "winhal.h"


#define WXS (240*3)
#define WYS (200*3)


// -----------------------------------------------------------------------
//
// Test truetype fonts and UTF-8
//
// -----------------------------------------------------------------------


void videotest_truetype(void)
{
    drv_video_window_t *w = drv_video_window_create( WXS, WYS, 300, 300, COLOR_BLACK, "TrueType Test Window", WFLAG_WIN_DECORATED );

    drv_video_winblt( w );

    //w_draw_line( w, 0, 0, WXS, WYS, COLOR_RED );
    //w_fill_ellipse( w, 30, 30, 15, 27, COLOR_BLUE );

    w_font_draw_string( w, &drv_video_8x16san_font, "Test font", COLOR_BLACK, COLOR_GREEN, 0, 0 );

    //w_fill_box( w,  40, 32, 33, 10, COLOR_RED );

    /*
    font_handle_t font1 = w_get_tt_font_file( "OpenSans-Regular.ttf", 100 );

    if( font1 == INVALID_POOL_HANDLE )
    {
        printf("\n\nTTF 1 FAIL\n\n");
    }
    else
        w_ttfont_draw_string( w, font1,
                          "TrueType rulez рулит",
                          COLOR_LIGHTRED, COLOR_BLACK,
                          10, 80 );
    */

    font_handle_t font2 = w_get_system_font_ext( 50 );

    if( font2 == INVALID_POOL_HANDLE )
    {
        //w_font_draw_string( w, &drv_video_8x16san_font, "TrueType Font Failed", COLOR_BLACK, COLOR_GREEN, 0, 30 );
        printf("\n\nTTF 2 FAIL\n\n");
    }
    else
        w_ttfont_draw_string( w, font2,
                          "TrueType рулит",
                          COLOR_LIGHTRED, //COLOR_BLACK,
                          10, 50 );


    w_release_tt_font( font2 );

    drv_video_winblt( w );

    (void) getchar();
}










// -----------------------------------------------------------------------
//
// Test truetype fonts and UTF-8
//
// -----------------------------------------------------------------------


void videotest_overlay(void)
{

    font_handle_t font2 = w_get_system_font_ext( 50 );

    if( font2 == INVALID_POOL_HANDLE )
    {
        //w_font_draw_string( w, &drv_video_8x16san_font, "TrueType Font Failed", COLOR_BLACK, COLOR_GREEN, 0, 30 );
        printf("\n\nTTF 2 FAIL\n\n");
        return;
    }

    drv_video_window_t *w_large = drv_video_window_create( WXS,   WYS,   100, 100, COLOR_BLACK, "Large Window", WFLAG_WIN_DECORATED );

    drv_video_winblt( w_large );


    drv_video_window_t *w_small = drv_video_window_create( WXS/2, WYS/2, 150, 150, COLOR_BLACK, "Small Window", WFLAG_WIN_DECORATED );

    drv_video_winblt( w_small );

    int i;
    for( i = 0; i < 100000; i++ )
    {
        w_to_top(w_small);

        drv_video_winblt( w_small );
        drv_video_winblt( w_large );

        w_ttfont_draw_string( w_large, font2, "TrueType рулит", COLOR_LIGHTRED, 130, 50 );

        w_to_top(w_large);
        drv_video_winblt( w_large );
        drv_video_winblt( w_small );

        w_ttfont_draw_string( w_small, font2, "TrueType рулит", COLOR_LIGHTRED, 10, 50 );

    }



}




void videotest_pbm()
{
//void *bmp_pixels;
//int bmpxs, bmpys;

    //int ysize = 60;
    //int xsize = 80;
    //char win[ysize*xsize*3];

    const char *bpm = "../../plib/resources/backgrounds/phantom_dz_large.pbm";

    int fd;
    int rc = k_open( &fd, bpm, 0, 0 );

    if( rc )
        {
            printf("can't open %s\n", bpm );
            exit(33);
        }

        k_seek( 0, fd, 0, SEEK_END );        
        //long size = ftell( f );
        int size;
        k_seek( &size, fd, 0, SEEK_CUR );
        k_seek( 0, fd, 0, SEEK_SET );

        printf("Size of %s is %d\n", bpm, size );

        char *data = malloc(size);

        rc = k_read( 0, fd, data, size );
        k_close(fd);

        drv_video_bitmap_t *bmp;

        //int i;
        int result;

        //for( i = 0; i < 1000; i++ )
            result = bmp_ppm_load( &bmp, data );

        free(data);

        if(result)
            printf("can't parse %s: %d\n", bpm, result );
        else
        {
            drv_video_window_t *w = drv_video_window_create( 900, 600, 10, 10, COLOR_BLACK, "BMP Test Window", WFLAG_WIN_DECORATED );
            drv_video_winblt( w );

            w_draw_bitmap( w, 0, 0, bmp );
            drv_video_winblt( w );

            getchar();
        }


}





